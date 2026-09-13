#include "entity_cache.hpp"
#include "sdk.hpp"
#include "../security/obfuscator.hpp"
#include "../render/overlay.hpp"

    #include <array>
    #include <chrono>
    #include <functional>
    #include <unordered_set>

namespace cache {
    static std::shared_ptr<const snapshot> current_snapshot = std::make_shared<snapshot>();
    static std::mutex mtx;

    std::shared_ptr<const snapshot> get_snapshot() {
        std::lock_guard<std::mutex> lock(mtx);
        return current_snapshot;
    }
}

namespace {
    static constexpr uintptr_t PRIM_READ_BASE = offsets::Primitive::CFrame;           // 0xC0
    static constexpr size_t    PRIM_READ_SIZE = (offsets::Primitive::Size + sizeof(Vec3)) - PRIM_READ_BASE; // 252

    inline void read_primitive_bulk(uintptr_t primitive, CFrame& cf, Vec3& pos, Vec3& sz) {
        alignas(16) uint8_t buf[PRIM_READ_SIZE];
        if (!mem.read_buf(primitive + PRIM_READ_BASE, buf, PRIM_READ_SIZE)) return;
        memcpy(&cf,  buf,                                                   sizeof(CFrame));
        memcpy(&pos, buf + (offsets::Primitive::Position - PRIM_READ_BASE), sizeof(Vec3));
        memcpy(&sz,  buf + (offsets::Primitive::Size     - PRIM_READ_BASE), sizeof(Vec3));
    }

    auto refresh_player_transforms(player_entry& entry) -> void {
        for (int i = 0; i < BONE_COUNT; ++i) {
            auto& bone = entry.bones[i];
            if (bone.valid && bone.primitive) {
                read_primitive_bulk(bone.primitive, bone.cframe, bone.position, bone.size);
            }
        }
        if (entry.root_primitive) {
            entry.velocity = mem.read<Vec3>(entry.root_primitive + offsets::Primitive::AssemblyLinearVelocity);
        }
    }

}

// [cache::refresh_live_transforms]
auto cache::refresh_live_transforms(player_entry& entry) -> void {
    refresh_player_transforms(entry);
}

// [cache::update]
auto cache::update() -> void {
    auto start_time = std::chrono::steady_clock::now();

    std::vector<player_entry> temp_list;

    static int64_t last_place_id = 0;
    int64_t current_place_id = game.get_placeid();
    if (current_place_id != last_place_id && current_place_id != 0) {
        render::add_notification(HIDE_STR("Game: ") + std::to_string(current_place_id), 5.0f);
        last_place_id = current_place_id;
    }

    const auto players_service = game.players();
    if (!players_service) return;

    const auto local_player_opt = game.get_local_player();
    static uintptr_t cached_local_player = 0;
    if (local_player_opt.has_value() && local_player_opt.value() != 0) {
        cached_local_player = local_player_opt.value();
    }
    const auto local_player = cached_local_player;

    const auto children = game.get_children(players_service);

    static std::unordered_map<uintptr_t, std::chrono::steady_clock::time_point> last_deep_update;
    static std::unordered_map<uintptr_t, std::string> name_cache;
    static std::unordered_map<uintptr_t, std::array<mesh_data, BONE_COUNT>> mesh_cache;
    static std::unordered_map<uintptr_t, bool> teammate_cache;
    static std::unordered_map<uintptr_t, uintptr_t> last_character;
    static std::unordered_map<uintptr_t, std::array<bone_info, BONE_COUNT>> bone_cache;
    static std::unordered_map<uintptr_t, uintptr_t> root_primitive_cache;
    static std::unordered_map<uintptr_t, std::string> weapon_cache;
    static std::unordered_map<uintptr_t, uintptr_t> humanoid_cache;
    auto now = std::chrono::steady_clock::now();

    std::vector<uintptr_t> current_ids;
    for (const auto& player_inst : children) 
    {
        current_ids.push_back(player_inst);
        player_entry entry;
        entry.instance = player_inst;
        entry.is_local = (player_inst == local_player);

        entry.character = mem.read<uintptr_t>(player_inst + offsets::Player::Character);

        bool character_changed = (last_character.find(player_inst) != last_character.end() && last_character[player_inst] != entry.character);
        bool deep_update = false;
        if (character_changed || last_deep_update.find(player_inst) == last_deep_update.end() || 
            std::chrono::duration_cast<std::chrono::seconds>(now - last_deep_update[player_inst]).count() >= 2) {
            deep_update = true;
            last_deep_update[player_inst] = now;
            last_character[player_inst] = entry.character;
        }

        if (deep_update) {
            entry.name = game.get_name(player_inst);
            name_cache[player_inst] = entry.name;
            entry.is_teammate = game.is_teammate(player_inst);
            teammate_cache[player_inst] = entry.is_teammate;
        } else {
            entry.name = name_cache[player_inst];
            entry.is_teammate = teammate_cache[player_inst];
        }

        // Use cached humanoid pointer to avoid triple find_first_child("Humanoid")
        // Invalidate on deep update (character changed or first time) so stale pointers aren't reused
        uintptr_t humanoid_ptr = deep_update ? 0 : humanoid_cache[player_inst];
        if (!humanoid_ptr && entry.character) {
            auto h = game.find_first_child(entry.character, HIDE_STR("Humanoid"));
            if (h) { humanoid_ptr = *h; humanoid_cache[player_inst] = humanoid_ptr; }
        }
        float health = humanoid_ptr ? mem.read<float>(humanoid_ptr + offsets::Humanoid::Health) : 0.0f;

        if (entry.character && health > 0.1f)
        {
            entry.alive = true;
            entry.hp = health;
            entry.max_hp = humanoid_ptr ? mem.read<float>(humanoid_ptr + offsets::Humanoid::MaxHealth) : 100.0f;
            
            if (deep_update) {
                const auto char_children = game.get_children(entry.character);
                entry.root_part = 0;
                entry.root_primitive = 0;
                entry.humanoid = 0;

                for (int i = 0; i < BONE_COUNT; ++i) entry.bones[i].valid = false;

                for (const auto& child : char_children) {
                    const auto child_name = game.get_name(child);
                    const auto class_name = game.get_class_name(child);

                    if (class_name == HIDE_STR("Tool")) {
                        entry.weapon = child_name;
                    }

                    bone_index b_idx = BONE_COUNT;
                    if (child_name == HIDE_STR("Head")) b_idx = BONE_HEAD;
                    else if (child_name == HIDE_STR("Torso")) b_idx = BONE_TORSO;
                    else if (child_name == HIDE_STR("UpperTorso")) b_idx = BONE_UPPER_TORSO;
                    else if (child_name == HIDE_STR("LowerTorso")) b_idx = BONE_LOWER_TORSO;
                    else if (child_name == HIDE_STR("Humanoid")) {
                        entry.humanoid = child;
                        humanoid_cache[player_inst] = child;
                    }
                    else if (child_name == HIDE_STR("HumanoidRootPart")) {
                        b_idx = BONE_ROOT;
                        entry.root_part = child;
                        entry.root_primitive = mem.read<uintptr_t>(child + offsets::BasePart::Primitive);
                    }
                    else if (child_name == HIDE_STR("Left Arm")) b_idx = BONE_LEFT_UPPER_ARM;
                    else if (child_name == HIDE_STR("Right Arm")) b_idx = BONE_RIGHT_UPPER_ARM;
                    else if (child_name == HIDE_STR("Left Leg")) b_idx = BONE_LEFT_UPPER_LEG;
                    else if (child_name == HIDE_STR("Right Leg")) b_idx = BONE_RIGHT_UPPER_LEG;
                    else if (child_name == HIDE_STR("LeftUpperArm")) b_idx = BONE_LEFT_UPPER_ARM;
                    else if (child_name == HIDE_STR("LeftLowerArm")) b_idx = BONE_LEFT_LOWER_ARM;
                    else if (child_name == HIDE_STR("LeftHand")) b_idx = BONE_LEFT_HAND;
                    else if (child_name == HIDE_STR("RightUpperArm")) b_idx = BONE_RIGHT_UPPER_ARM;
                    else if (child_name == HIDE_STR("RightLowerArm")) b_idx = BONE_RIGHT_LOWER_ARM;
                    else if (child_name == HIDE_STR("RightHand")) b_idx = BONE_RIGHT_HAND;
                    else if (child_name == HIDE_STR("LeftUpperLeg")) b_idx = BONE_LEFT_UPPER_LEG;
                    else if (child_name == HIDE_STR("LeftLowerLeg")) b_idx = BONE_LEFT_LOWER_LEG;
                    else if (child_name == HIDE_STR("LeftFoot")) b_idx = BONE_LEFT_FOOT;
                    else if (child_name == HIDE_STR("RightUpperLeg")) b_idx = BONE_RIGHT_UPPER_LEG;
                    else if (child_name == HIDE_STR("RightLowerLeg")) b_idx = BONE_RIGHT_LOWER_LEG;
                    else if (child_name == HIDE_STR("RightFoot")) b_idx = BONE_RIGHT_FOOT;

                    if (b_idx != BONE_COUNT)
                    {
                        bone_info& info = entry.bones[b_idx];
                        uintptr_t primitive = mem.read<uintptr_t>(child + offsets::BasePart::Primitive);
                        
                        if (!primitive && b_idx == BONE_HEAD) {
                            for (const auto& head_child : game.get_children(child)) {
                                primitive = mem.read<uintptr_t>(head_child + offsets::BasePart::Primitive);
                                if (primitive) break;
                            }
                        }

                        if (!primitive) continue;

                        info.valid = true;
                        info.instance = child;
                        info.primitive = primitive;
                        read_primitive_bulk(primitive, info.cframe, info.position, info.size);

                        if (class_name == HIDE_STR("MeshPart")) {
                            info.mesh.is_mesh_part = true;
                            std::string url = game.read_roblox_string(child + offsets::MeshPart::MeshId);
                            info.mesh.asset_id = game.extract_asset_id(url);
                        } else {
                            auto special_mesh = game.find_first_class(child, HIDE_STR("SpecialMesh"));
                            if (special_mesh) {
                                info.mesh.special_mesh_ptr = *special_mesh;
                                std::string url = game.read_roblox_string(*special_mesh + offsets::SpecialMesh::MeshId);
                                info.mesh.asset_id = game.extract_asset_id(url);
                                info.mesh.mesh_scale = mem.read<Vec3>(*special_mesh + offsets::SpecialMesh::Scale);
                            } else info.mesh.asset_id = 0;
                        }
                    }
                }
                bone_cache[player_inst] = entry.bones;
                root_primitive_cache[player_inst] = entry.root_primitive;

                if (current_place_id == 17625359962) {
                    static uintptr_t vmf = 0;
                    static int64_t vmp = 0;
                    if (vmp != current_place_id) {
                        auto dm = game.get_datamodel();
                        if (dm) {
                            auto rs = game.find_first_class(*dm, HIDE_STR("ReplicatedStorage"));
                            if (rs) {
                                auto as = game.find_first_child(*rs, HIDE_STR("Assets"));
                                if (as) {
                                    auto tp = game.find_first_child(*as, HIDE_STR("Temp"));
                                    if (tp) vmf = game.find_first_child(*tp, HIDE_STR("ViewModels")).value_or(0);
                                }
                            }
                        }
                        vmp = current_place_id;
                    }
                    if (vmf) {
                        std::string pfx = entry.name + HIDE_STR(" - ");
                        for (auto v : game.get_children(vmf)) {
                            std::string vn = game.get_name(v);
                            if (vn.find(pfx) == 0) {
                                size_t s = vn.find(HIDE_STR(" - "), pfx.length());
                                entry.weapon = (s != std::string::npos) ? vn.substr(pfx.length(), s - pfx.length()) : vn.substr(pfx.length());
                                break;
                            }
                        }
                    }
                }
                weapon_cache[player_inst] = entry.weapon;
            } else {
                entry.bones = bone_cache[player_inst];
                entry.root_primitive = root_primitive_cache[player_inst];
                entry.weapon = weapon_cache[player_inst];
                entry.humanoid = humanoid_cache[player_inst];
            }
            refresh_player_transforms(entry);
            bone_cache[player_inst] = entry.bones;
        }
        else
        {
            entry.alive = false;
            entry.hp = 0.0f;
            entry.max_hp = 100.0f;
            entry.velocity = { 0.0f, 0.0f, 0.0f };
            entry.root_part = 0;
            entry.root_primitive = 0;
            for (int i = 0; i < BONE_COUNT; ++i) entry.bones[i].valid = false;
        }

        temp_list.push_back(entry);

    }

    std::vector<model_entry> temp_models;
    std::vector<part_entry> temp_parts;

    if (globals::visuals::visual_check.load()) {
        const auto dm = game.get_datamodel();
        if (dm) {
            const auto workspace = game.find_first_class(*dm, HIDE_STR("Workspace"));
            if (workspace) {
                std::function<void(uintptr_t, int, bool)> scan_recursive;
                scan_recursive = [&](uintptr_t parent, int depth, bool in_character) {
                    if (depth > 5) return; // limit depth to prevent infinite loops or extreme lag

                    for (const auto& child : game.get_children(parent)) {
                        std::string class_name = game.get_class_name(child);
                        bool is_character = in_character || game.find_first_child(child, HIDE_STR("Humanoid")).has_value();
                        
                        if (class_name == HIDE_STR("Model")) {
                            if (!is_character) {
                                model_entry me;
                                me.instance = child;
                                me.name = game.get_name(child);
                                
                                bool found_pos = false;
                                for (const auto& model_child : game.get_children(child)) {
                                    std::string mc_class = game.get_class_name(model_child);
                                    if (mc_class.find(HIDE_STR("Part")) != std::string::npos) {
                                        uintptr_t prim = mem.read<uintptr_t>(model_child + offsets::BasePart::Primitive);
                                        if (prim) {
                                            read_primitive_bulk(prim, me.cframe, me.position, me.size);
                                            found_pos = true;
                                            break;
                                        }
                                    }
                                }
                                if (found_pos) temp_models.push_back(me);
                            }
                            scan_recursive(child, depth + 1, is_character);
                        }
                        else if (class_name.find(HIDE_STR("Part")) != std::string::npos) {
                            if (!is_character) {
                                part_entry pe;
                                pe.instance = child;
                                pe.name = game.get_name(child);
                                uintptr_t prim = mem.read<uintptr_t>(child + offsets::BasePart::Primitive);
                                if (prim) {
                                    read_primitive_bulk(prim, pe.cframe, pe.position, pe.size);
                                    temp_parts.push_back(pe);
                                }
                            }
                        }
                        else if (class_name == HIDE_STR("Folder")) {
                            scan_recursive(child, depth + 1, in_character);
                        }
                    }
                };
                scan_recursive(*workspace, 0, false);
            }
        }
    }

    // cleanup static caches (B1 fix) -- use set for O(1) lookup instead of O(n) std::find
    std::unordered_set<uintptr_t> current_id_set(current_ids.begin(), current_ids.end());
    auto cleanup = [&](auto& map) {
        for (auto it = map.begin(); it != map.end(); ) {
            if (current_id_set.find(it->first) == current_id_set.end()) {
                it = map.erase(it);
            } else {
                ++it;
            }
        }
    };

    cleanup(last_deep_update);
    cleanup(name_cache);
    cleanup(mesh_cache);
    cleanup(teammate_cache);
    cleanup(last_character);
    cleanup(bone_cache);
    cleanup(root_primitive_cache);
    cleanup(weapon_cache);
    cleanup(humanoid_cache);

    auto new_snapshot = std::make_shared<snapshot>();
    new_snapshot->players = std::move(temp_list);
    new_snapshot->models = std::move(temp_models);
    new_snapshot->parts = std::move(temp_parts);

    {
        std::lock_guard<std::mutex> lock(mtx);
        current_snapshot = std::move(new_snapshot);
    }

    auto end_time = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float, std::milli>(end_time - start_time).count();
    globals::settings::player_cache_delay.store(elapsed);
}
