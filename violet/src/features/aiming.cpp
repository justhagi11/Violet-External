#include "aiming.hpp"
#include "../core/detail.hpp"
#include "../math/perlin.hpp"
#include "../game/entity_cache.hpp"
#include <random>
#include <algorithm>

// [aiming::reset_silent_aim]
void aiming::reset_silent_aim() {
    if (!is_silent_active) return;

    Vector2 screen_size = math::get_viewport_size();
    game.set_viewport_int16({ static_cast<int16_t>(screen_size.x), static_cast<int16_t>(screen_size.y) });

    is_silent_active = false;
}

// [aiming::get_target_bone]
int aiming::get_target_bone(int setting, int& random_storage) {
    if (setting == 17) return random_storage; // random

    switch (setting) {
        case 0:  return BONE_HEAD;
        case 1:  return BONE_TORSO;
        case 2:  return BONE_UPPER_TORSO;
        case 3:  return BONE_LOWER_TORSO;
        case 4:  return BONE_ROOT;
        case 5:  return BONE_LEFT_UPPER_ARM;
        case 6:  return BONE_LEFT_LOWER_ARM;
        case 7:  return BONE_LEFT_HAND;
        case 8:  return BONE_RIGHT_UPPER_ARM;
        case 9:  return BONE_RIGHT_LOWER_ARM;
        case 10: return BONE_RIGHT_HAND;
        case 11: return BONE_LEFT_UPPER_LEG;
        case 12: return BONE_LEFT_LOWER_LEG;
        case 13: return BONE_LEFT_FOOT;
        case 14: return BONE_RIGHT_UPPER_LEG;
        case 15: return BONE_RIGHT_LOWER_LEG;
        case 16: return BONE_RIGHT_FOOT;
        default: return BONE_HEAD;
    }
}

// [aiming::is_visible]
bool is_visible(const Vec3& start, const Vec3& end, const std::vector<model_entry>& models, const std::vector<part_entry>& parts) {
    Vec3 direction = end - start;
    float length = std::sqrtf(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
    if (length < 0.1f) return true;

    Vec3 dir_norm = { direction.x / length, direction.y / length, direction.z / length };

    auto check_obb = [&](const CFrame& cf, const Vec3& sz) {
        float max_dim = (std::max)({ sz.x, sz.y, sz.z });
        Vec3 cf_pos = { cf.x, cf.y, cf.z };
        if (start.dist(cf_pos) > length + max_dim) return false;

        Vec3 local_origin = cf.point_to_object(start);
        Vec3 local_dir = cf.vector_to_object(dir_norm);

        float t_min = 0.0f;
        float t_max = length;

        float half_sz[3] = { sz.x * 0.5f, sz.y * 0.5f, sz.z * 0.5f };
        float l_origin[3] = { local_origin.x, local_origin.y, local_origin.z };
        float l_dir[3] = { local_dir.x, local_dir.y, local_dir.z };

        for (int i = 0; i < 3; ++i) {
            if (std::abs(l_dir[i]) < 0.0001f) {
                if (std::abs(l_origin[i]) > half_sz[i]) return false;
            } else {
                float ood = 1.0f / l_dir[i];
                float t1 = (-half_sz[i] - l_origin[i]) * ood;
                float t2 = (half_sz[i] - l_origin[i]) * ood;

                if (t1 > t2) std::swap(t1, t2);
                if (t1 > t_min) t_min = t1;
                if (t2 < t_max) t_max = t2;
                if (t_min > t_max) return false;
            }
        }
        return true;
    };

    for (const auto& m : models) {
        if (check_obb(m.cframe, m.size)) return false;
    }
    for (const auto& p : parts) {
        if (check_obb(p.cframe, p.size)) return false;
    }

    return true;
}

// [aiming::tick]
void aiming::tick() {
    auto start_time = std::chrono::steady_clock::now();

    // js ut return if nothing is enabled, no need to waste cpu cycles on checking for targets and stuff if nothing is enabled
    if (!globals::aim::aimbot_enabled && !globals::aim::silentaim_enabled && !globals::triggerbot::triggerbot_enabled) {
        CurrentTarget = 0;
        return;
    }

    auto snap = cache::get_snapshot();
    if (snap->players.size() <= 1) {
        CurrentTarget = 0;
        return;
    }
    const auto& local_players = snap->players;
    const auto& local_models = snap->models;
    const auto& local_parts = snap->parts;

    const auto datamodel = game.get_datamodel();
    const auto workspace = datamodel ? game.find_first_class(*datamodel, HIDE_STR("Workspace")) : std::nullopt;
    uintptr_t camera = 0;
    Vec3 camera_pos = { 0, 0, 0 };
    if (workspace) {
        camera = mem.read<uintptr_t>(*workspace + offsets::Workspace::CurrentCamera);
        if (camera) camera_pos = mem.read<Vec3>(camera + offsets::Camera::Position);
    }

    auto view_matrix = math::get_view_matrix();
    auto viewport = math::get_viewport_size();
    
    Vector2 screen_center = { viewport.x / 2.0f, viewport.y / 2.0f };
    static HWND rbx_hwnd = nullptr;
    if (!rbx_hwnd || !IsWindow(rbx_hwnd)) {
        rbx_hwnd = FindWindowW(NULL, L"Roblox");
    }
    
    if (rbx_hwnd) {
        POINT pt;
        if (GetCursorPos(&pt)) {
            ScreenToClient(rbx_hwnd, &pt);
            screen_center = { static_cast<float>(pt.x), static_cast<float>(pt.y) };
        }
    }

    // rcs
    static float last_pitch = 0.0f;
    static float last_yaw = 0.0f;
    float current_pitch = 0.0f;
    float current_yaw = 0.0f;

    {
        // extracting angles from view matrix
        Vec3 forward = { -view_matrix.data[2], -view_matrix.data[6], -view_matrix.data[10] };
        current_yaw = std::atan2(forward.x, forward.z);
        current_pitch = std::asin(std::clamp(forward.y, -1.0f, 1.0f));
    }

    float rcs_dx = 0.0f;
    float rcs_dy = 0.0f;
    float rcs_scale = globals::aim::aimbot_rcs_scale.load();

    if (last_pitch != 0.0f && rcs_scale > 0.01f) {
        float dp = current_pitch - last_pitch;
        float dy = current_yaw - last_yaw;
        if (dy > PI) dy -= PI_2;
        if (dy < -PI) dy += PI_2;

        float k = (viewport.y / DEG2RAD(120.0f)); // offload to 120.
        rcs_dx = -(dy * k) * rcs_scale; 
        rcs_dy = (dp * k) * rcs_scale;
    }
    last_pitch = current_pitch;
    last_yaw = current_yaw;

    uintptr_t best_aim_target = 0;
    float best_aim_score = FLT_MAX;
    Vector2 best_aim_screen_pos = { 0, 0 };
    Vec3 best_aim_world_pos = { 0, 0, 0 };
    Vec3 best_aim_velocity = { 0, 0, 0 };
    Vec3 local_velocity = { 0, 0, 0 };

    bool allow_aimbot = globals::aim::aimbot_enabled && detail::is_keybind_active(
        globals::aim::aimbot_bind_mode,
        globals::aim::aimbot_bind1,
        globals::aim::aimbot_bind2
    );

    // delay logic
    static double start_press_time = 0;
    static bool was_holding = false;
    if (allow_aimbot) {
        if (!was_holding) {
            start_press_time = detail::now_sec();
            was_holding = true;
        }

        float start_delay = globals::aim::aimbot_start_delay.load();
        if (start_delay > 0.001f && (detail::now_sec() - start_press_time) < static_cast<double>(start_delay)) {
            allow_aimbot = false;
        }
    } else {
        was_holding = false;
    }

    // focus logic
    float focus = 0.0f;
    float focus_cfg = globals::aim::aimbot_focus_in_seconds.load();
    if (focus_cfg > 0.01f && was_holding) {
        double elapsed = detail::now_sec() - start_press_time;
        focus = std::clamp(static_cast<float>(elapsed / (double)focus_cfg), 0.0f, 1.0f);
    }

    // edge detection
    static bool was_aimbot_active = false;
    static bool was_silent_active_kb = false;

    bool is_aimbot_active = detail::is_keybind_active(globals::aim::aimbot_bind_mode, globals::aim::aimbot_bind1, globals::aim::aimbot_bind2);
    bool is_silent_active_kb = detail::is_keybind_active(globals::aim::silentaim_bind_mode, globals::aim::silentaim_bind1, globals::aim::silentaim_bind2);

    if (is_aimbot_active && !was_aimbot_active) {
        static std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, BONE_COUNT - 1);
        current_random_bone = dist(gen);
    }
    if (is_silent_active_kb && !was_silent_active_kb) {
        static std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, BONE_COUNT - 1);
        current_random_silent_bone = dist(gen);
    }
    was_aimbot_active = is_aimbot_active;
    was_silent_active_kb = is_silent_active_kb;

    float current_fov = globals::aim::aimbot_fov.load();
    if (focus > 0.01f) {
        current_fov *= (1.0f - (focus * 0.5f)); // narrow fov
    }

    // stop on kill
    static double kill_time = 0;
    static uintptr_t last_target = 0;
    static bool was_target_alive = false;

    if (globals::aim::aimbot_stop_on_kill.load()) {
        if (detail::now_sec() < kill_time) {
            allow_aimbot = false;
        }
    }

    {
        // check if last target died
        if (globals::aim::aimbot_stop_on_kill.load() && last_target != 0 && was_target_alive) {
            bool found = false;
            for (const auto& p : local_players) {
                if (p.instance == last_target) {
                    found = true;
                    if (!p.alive) {
                        kill_time = detail::now_sec() + 0.35; // 350ms delay
                    }
                    break;
                }
            }
            if (!found) { // assumed dead
                kill_time = detail::now_sec() + 0.35;
            }
        }

        bool target_alive = false;

        for (const auto& player : local_players) {
            if (player.is_local) {
                local_velocity = player.velocity;
            }

            if (!player.alive || player.is_local) continue;

            // aimbot loop
            if (allow_aimbot) {

                if (globals::aim::aimbot_team_check && player.is_teammate) continue;

                int target_bone_index = -1;
                uint32_t blacklist = globals::aim::aimbot_hitpart_blacklist.load();

                // hitpart retrieval via switch logic
                int preferred = get_target_bone(globals::aim::aimbot_prefered_hitpart.load(), current_random_bone);

                // try preferred first
                if (!(blacklist & (1 << preferred)) && player.bones[preferred].valid) {
                    target_bone_index = preferred;
                } else {
                    static const bone_index fallbacks[] = { BONE_HEAD, BONE_UPPER_TORSO, BONE_TORSO, BONE_LOWER_TORSO, BONE_ROOT };
                    for (auto b : fallbacks) {
                        if (!(blacklist & (1 << b)) && player.bones[b].valid) {
                            target_bone_index = b;
                            break;
                        }
                    }
                }

                if (target_bone_index == -1) continue;

                
                Vec3 bone_pos = mem.read<Vec3>(player.bones[target_bone_index].primitive + offsets::Primitive::Position); // i wanted to reuse the visuals but that would cause approx 5ms delay.
                Vector2 screen_pos;
                if (math::world_to_screen(bone_pos, view_matrix, viewport.x, viewport.y, screen_pos)) {

                    float dx = screen_pos.x - screen_center.x;
                    float dy = screen_pos.y - screen_center.y;
                    float dist = std::sqrtf(dx * dx + dy * dy);
                    float depth = (bone_pos.x * view_matrix.data[12]) + (bone_pos.y * view_matrix.data[13]) + (bone_pos.z * view_matrix.data[14]) + view_matrix.data[15];

                    // dynamic fov calculation
                    float fov_to_check = current_fov;
                    if (globals::aim::aimbot_dynamic_fov.load()) {
                        fov_to_check = (current_fov * 60.0f) / std::max(1.0f, depth);
                    }

                    if (dist < fov_to_check) {

                        if (globals::aim::aimbot_wall_check.load()) {
                            if (camera) {
                                if (!is_visible(camera_pos, bone_pos, local_models, local_parts)) continue;
                            }
                        }

                        float current_score = dist;
                        bool is_sticky = (globals::aim::aimbot_sticky_aim.load() && player.instance == last_target);

                        if (is_sticky) {
                            current_score = -FLT_MAX;
                        } else {
                            int priority = globals::aim::aimbot_target_priority.load();
                            if (priority == 2) current_score = player.hp;
                            else if (priority == 3) current_score = depth;
                        }

                        if (current_score < best_aim_score) {
                            best_aim_score = current_score;
                            best_aim_target = player.instance;
                            best_aim_screen_pos = screen_pos;
                            best_aim_world_pos = bone_pos;
                            best_aim_velocity = player.velocity;
                            target_alive = player.alive;
                        }
                    }

                }
            }


        }
        
        was_target_alive = target_alive;
    }

    static double target_state_time = 0;
    static Vector2 overshoot_offset = { 0, 0 };
    static uintptr_t last_aim_target = 0;

    if (best_aim_target != 0) {
        if (best_aim_target != last_aim_target) {
            target_state_time = detail::now_sec();
            last_aim_target = best_aim_target;

            float over_cfg = globals::aim::aimbot_overshoot.load();
            if (over_cfg > 0.01f) {
                float dx = best_aim_screen_pos.x - screen_center.x;
                float dy = best_aim_screen_pos.y - screen_center.y;
                float mag = std::sqrtf(dx * dx + dy * dy);
                if (mag > 5.0f) {
                    float scale = over_cfg / 10.0f; 
                    overshoot_offset = { (dx / mag) * (mag * scale), (dy / mag) * (mag * scale) };
                }
            } else {
                overshoot_offset = { 0, 0 };
            }
        }
    } else {
        last_aim_target = 0;
        overshoot_offset = { 0, 0 };
    }

    if (allow_aimbot && best_aim_target != 0) {
        int aim_mode = globals::aim::aimbot_mode.load();

        if (aim_mode == 1) { // mouse
            float dx = best_aim_screen_pos.x - screen_center.x;
            float dy = best_aim_screen_pos.y - screen_center.y;

            // apply decaying overshoot
            double elapsed = detail::now_sec() - target_state_time;
            if (elapsed < 0.25) { // overshoot lasts 250ms
                float scatter_factor = 1.0f - static_cast<float>(elapsed / 0.25);
                dx += overshoot_offset.x * scatter_factor;
                dy += overshoot_offset.y * scatter_factor;
            }

            float smooth_x = globals::aim::aimbot_smoothing_x.load();
            float smooth_y = globals::aim::aimbot_smoothing_y.load();
            float smooth_xy = globals::aim::aimbot_smoothing_xy.load();
            float smooth_xz = globals::aim::aimbot_smoothing_xz.load();

            // jumping check
            if (globals::aim::aimbot_check_jumping.load()) {
                bool local_airborne = (std::abs(local_velocity.y) > 5.0f);
                bool target_airborne = (std::abs(best_aim_velocity.y) > 5.0f);

                if (local_airborne || target_airborne) {
                    smooth_x *= 2.0f;
                    smooth_y *= 2.0f;
                }
            }

            // sigma velo
            // more velocity more struggle :3
            Vec3 rel_v = best_aim_velocity - local_velocity;
            float sigma_v = std::sqrtf(rel_v.x * rel_v.x + rel_v.z * rel_v.z);
            float sigma_factor = 1.0f + (sigma_v / 65.0f);

            smooth_x *= sigma_factor;
            smooth_y *= sigma_factor;

            if (smooth_x <= 0.0f) smooth_x = 1.0f;
            if (smooth_y <= 0.0f) smooth_y = 1.0f;
            if (smooth_xy <= 0.0f) smooth_xy = 1.0f;
            if (smooth_xz <= 0.0f) smooth_xz = 1.0f;

            float t_x = 1.0f / (smooth_x * smooth_xy);
            float t_y = 1.0f / (smooth_y * smooth_xz);
            t_x = std::clamp(t_x, 0.001f, 1.0f);
            t_y = std::clamp(t_y, 0.001f, 1.0f);

            // focus tightening
            if (focus > 0.01f) {
                t_x = detail::lerp(t_x, 1.0f, focus * 0.75f); // tighten by up to 75% precision
                t_y = detail::lerp(t_y, 1.0f, focus * 0.75f);
            }

            int curve_type = globals::aim::aimbot_curve_type.load();
            if (curve_type == 1) { // sine
                dx *= detail::ease_in_out_sine(t_x);
                dy *= detail::ease_in_out_sine(t_y);
            } else if (curve_type == 2) { // cubic
                dx *= detail::ease_in_out_cubic(t_x);
                dy *= detail::ease_in_out_cubic(t_y);
            } else { // linear
                dx *= t_x;
                dy *= t_y;
            }

            if (globals::aim::aimbot_humanizer.load()) {
                float time_f = static_cast<float>(detail::now_sec());
                float sigma_noise = 2.5f * sigma_factor; // amplify noise based on velocity

                // decay noise when we are close to the target center
                float current_dist = std::sqrtf(dx * dx + dy * dy);
                if (current_dist < 20.0f) {
                    sigma_noise *= (current_dist / 20.0f);
                }

                dx += perlin::noise2d(time_f * 2.5f, 0.0f) * sigma_noise;
                dy += perlin::noise2d(time_f * 2.5f, 42.0f) * sigma_noise; // using perlin for humanization, looks fucking legit.
            }

            float sensitivity = globals::aim::aimbot_sensitivity.load();
            if (sensitivity > 0.001f && std::abs(sensitivity - 1.0f) > 0.001f) {
                dx *= sensitivity;
                dy *= sensitivity;
            }

            // apply rcs correction
            dx += rcs_dx;
            dy += rcs_dy;

            // fractional remainder tracking to prevent truncation stopping
            static float remainder_x = 0.0f;
            static float remainder_y = 0.0f;

            if (best_aim_target == 0) {
                remainder_x = 0;
                remainder_y = 0;
            }

            dx += remainder_x;
            dy += remainder_y;

            float move_x = std::trunc(dx);
            float move_y = std::trunc(dy);

            remainder_x = dx - move_x;
            remainder_y = dy - move_y;

            INPUT input = { 0 };
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_MOVE;
            input.mi.dx = static_cast<LONG>(move_x);
            input.mi.dy = static_cast<LONG>(move_y);
            
            SendInput(1, &input, sizeof(INPUT));
        }
        else if (aim_mode == 2) { // memory
            if (!camera) return;
            if (game.get_placeid() == 17625359962LL) {
                Vector2 screen_size = viewport;
                Vector2 target_screen_pos;

                if (math::world_to_screen(best_aim_world_pos, view_matrix, screen_size.x, screen_size.y, target_screen_pos)) {
                    Vector2Int16 silent_vp = math::calculate_silent_viewport(target_screen_pos, screen_size);
                    game.set_viewport_int16(silent_vp);
                    is_silent_active = true;
                }
            } else {
                Matrix3x3 target_rot = math::calculate_look_at(camera_pos, best_aim_world_pos);
                mem.write<Matrix3x3>(camera + offsets::Camera::Rotation, target_rot);
            }
        }
    } else {
        reset_silent_aim();
    }

    last_target = best_aim_target;

    bool lbutton = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    
    bool allow_silent = globals::aim::silentaim_enabled && detail::is_keybind_active(
        globals::aim::silentaim_bind_mode.load(),
        globals::aim::silentaim_bind1.load(),
        globals::aim::silentaim_bind2.load()
    );

    if (allow_silent && lbutton) {
        uintptr_t best_silent_target = 0;
        Vec3 target_pos = { 0, 0, 0 };
        float best_silent_score = FLT_MAX;

        const auto& s_view_matrix = view_matrix;
        const auto& s_viewport = viewport;
        Vector2 s_center = { s_viewport.x / 2.0f, s_viewport.y / 2.0f };

        for (const auto& player : local_players) {
            if (!player.alive || player.is_local) continue;
            if (globals::aim::silentaim_team_check && player.is_teammate) continue;

            // switch logic implementation for silent aim
            int bone_idx = get_target_bone(globals::aim::silentaim_hitpart.load(), current_random_silent_bone);
            if (!player.bones[bone_idx].valid) bone_idx = BONE_HEAD; // fallback
            if (!player.bones[bone_idx].valid) continue;

            Vec3 pos = mem.read<Vec3>(player.bones[bone_idx].primitive + offsets::Primitive::Position);
            Vector2 screen_pos;
            if (math::world_to_screen(pos, s_view_matrix, s_viewport.x, s_viewport.y, screen_pos)) {
                float dx = screen_pos.x - s_center.x;
                float dy = screen_pos.y - s_center.y;
                float dist = std::sqrtf(dx * dx + dy * dy);
                float depth = (pos.x * s_view_matrix.data[12]) + (pos.y * s_view_matrix.data[13]) + (pos.z * s_view_matrix.data[14]) + s_view_matrix.data[15];

                if (dist < globals::aim::silentaim_fov.load()) {
                    if (globals::aim::silentaim_wall_check.load()) {
                        if (camera) {
                            if (!is_visible(camera_pos, pos, local_models, local_parts)) continue;
                        }
                    }
                    float score = dist; // default closest to crosshair
                    if (globals::aim::silentaim_target_priority.load() == 2) score = player.hp;
                    else if (globals::aim::silentaim_target_priority.load() == 3) score = depth;

                    if (score < best_silent_score) {
                        best_silent_score = score;
                        best_silent_target = player.instance;
                        target_pos = pos;
                    }
                }
            }
        }

        if (best_silent_target != 0) {
            Vector2 screen_size = viewport;
            Vector2 target_screen_pos;

            if (math::world_to_screen(target_pos, view_matrix, screen_size.x, screen_size.y, target_screen_pos)) {
                Vector2Int16 silent_vp = math::calculate_silent_viewport(target_screen_pos, screen_size);
                game.set_viewport_int16(silent_vp);
                is_silent_active = true;
            }
        } else {
            reset_silent_aim();
        }
    } else {
        reset_silent_aim();
    }
    

    auto end_time = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float, std::milli>(end_time - start_time).count();
    globals::settings::aim_thread_delay.store(elapsed);
}
