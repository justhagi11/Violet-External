#pragma once


#include "../io/memory/memory.hpp"
#include "../io/network/request.hpp"
#include "../security/obfuscator.hpp"
#include "offsets.hpp"

#include <string>
#include <string_view>
#include <optional>
#include <vector>

#include "../math/types.hpp"
#include "../core/globals.hpp"

class sdk {
public:
	// [sdk::get_datamodel]
	auto get_datamodel() const -> std::optional<uintptr_t> {
		const auto fake_dm = mem.read<uintptr_t>(mem.base_address + offsets::FakeDataModel::Pointer);

		return mem.read<uintptr_t>(fake_dm + offsets::FakeDataModel::RealDataModel);
	}

	// [sdk::get_children]
	auto get_children(uintptr_t instance) const -> std::vector<uintptr_t> {

		std::vector<uintptr_t> children_list;

		const auto children_ptr = mem.read<uintptr_t>(instance + offsets::Instance::ChildrenStart);
		if (!children_ptr) return {};

		const auto start = mem.read<uintptr_t>(children_ptr);
		const auto end = mem.read<uintptr_t>(children_ptr + offsets::Instance::ChildrenEnd);

		for (auto it = start; it < end; it += 16) {
			const auto child = mem.read<uintptr_t>(it);
			if (child) children_list.push_back(child);
		}

		return children_list;

	}

	// [sdk::find_first_child]
	auto find_first_child(uintptr_t parent,std::string_view target_name) const -> std::optional<uintptr_t> {

		for (const auto& child : get_children(parent)) {
			if (get_name(child) == target_name) return child;
		}

		return std::nullopt;

	}

	// [sdk::read_roblox_string]
	auto read_roblox_string(uintptr_t address) const -> std::string {

		if (!address) return HIDE_STR("???");

		const auto length = mem.read<size_t>(address + 0x10);
		if (length <= 0 || length > 200) return HIDE_STR("read_roblox_string: string is not valid");
		const auto data_addr = (length > 15) ? mem.read<uintptr_t>(address) : address;

		std::vector<char> buffer(length + 1);
		auto h = mem.process_handle;
		if (!h) return HIDE_STR("read_roblox_string: not attached");
		Hagi_ReadVirtualMemory(h.get(), reinterpret_cast<PVOID>(data_addr), buffer.data(), length, nullptr);
		buffer[length] = '\0';

		return std::string(buffer.data());
	}

	// [sdk::write_roblox_string]
	auto write_roblox_string(uintptr_t address, std::string_view new_text) const -> bool {

		if (!address) return false;

		const auto capacity = mem.read<size_t>(address + 0x18);
		const auto new_len = new_text.length();

		if (new_len >= capacity) {
			return false;
		}

		uintptr_t target_buffer = address;
		if (capacity >= 16) {
			target_buffer = mem.read<uintptr_t>(address);
		}

		if (!target_buffer) return false;
		for (size_t i = 0; i < new_len; i++) {
			mem.write<char>(target_buffer + i, new_text[i]);
		}

		mem.write<char>(target_buffer + new_len, '\0');
		mem.write<size_t>(address + 0x10, new_len);

		return true;
	}
	
	// [sdk::get_class_name]
	auto get_class_name(uintptr_t instance) const -> std::string {

		const auto descriptor = mem.read<uintptr_t>(instance + offsets::Instance::ClassDescriptor);
		if (!descriptor) return HIDE_STR("???");

		const auto class_name_ptr = mem.read<uintptr_t>(descriptor + offsets::Instance::ClassName);
		return read_roblox_string(class_name_ptr);

	}

	// [sdk::get_name]
	auto get_name(uintptr_t instance) const -> std::string {

		const auto name_ptr = mem.read<uintptr_t>(instance + offsets::Instance::Name);
		return read_roblox_string(name_ptr);

	}

	// [sdk::find_first_class]
	auto find_first_class(uintptr_t parent, std::string_view target_class) const -> std::optional<uintptr_t> {

		for (const auto& child : get_children(parent)) {
			if (get_class_name(child) == target_class) {
				return child;
			}
		}

		return std::nullopt;

	}

	// [sdk::get_local_player]
	auto get_local_player() const -> std::optional<uintptr_t> {

		const auto datamodel = get_datamodel();
		if (!datamodel) return std::nullopt;

		const auto players = find_first_class(*datamodel, HIDE_STR("Players"));
		if (!players) return std::nullopt;

		const auto lp = mem.read<uintptr_t>(*players + offsets::Players::LocalPlayer);
		if (!lp) return std::nullopt;

		return lp;

	}

	// [sdk::get_character]
	auto get_character(uintptr_t player) const -> std::optional<uintptr_t> {

		const auto character = mem.read<uintptr_t>(player + offsets::Player::Character);
		if (!character) return std::nullopt;
		return character;

	}

	// [sdk::get_root_part]
	auto get_root_part(uintptr_t character) const -> std::optional<uintptr_t> {
		
		return find_first_child(character, HIDE_STR("HumanoidRootPart"));

	}

	// [sdk::get_players_service]
	auto get_players_service() const -> std::optional<uintptr_t> {

		const auto dm = get_datamodel();
		if (!dm) return std::nullopt;

		return find_first_class(*dm, HIDE_STR("Players"));

	}

	// [sdk::players]
	auto players() const -> uintptr_t {
		return get_players_service().value_or(0);
	}

	// [sdk::get_health]
	auto get_health(uintptr_t character) const -> float {

		const auto humanoid = find_first_child(character, HIDE_STR("Humanoid"));
		if (!humanoid) return 0.0f;

		return mem.read<float>(*humanoid + offsets::Humanoid::Health);
	}

	// [sdk::hp]
	auto hp(uintptr_t character) const -> float {
		return get_health(character);
	}

	// [sdk::get_max_health]
	auto get_max_health(uintptr_t character) const -> float {
		const auto humanoid = find_first_child(character, HIDE_STR("Humanoid"));
		if (!humanoid) return 100.0f;

		return mem.read<float>(*humanoid + offsets::Humanoid::MaxHealth);
	}

	// [sdk::max_hp]
	auto max_hp(uintptr_t character) const -> float {
		return get_max_health(character);
	}


	// [sdk::is_alive]
	auto is_alive(uintptr_t character) const -> bool {

		return get_health(character) > 0.1f;
	}

	// [sdk::is_teammate]
	auto is_teammate(uintptr_t player_inst) const -> bool {

		if (!player_inst) return false;

		const auto character = mem.read<uintptr_t>(player_inst + offsets::Player::Character);
		if (!character) return false;

		const auto children = get_children(character);
		for (const auto& child : children) {
			if (get_name(child) == HIDE_STR("HumanoidRootPart")) {
				const auto hrp_children = get_children(child);
				for (const auto& hrp_child : hrp_children) {
					if (get_name(hrp_child) == HIDE_STR("TeammateLabel")) {
						return true;
					}
				}
			}
		}
		return false;
	}


	// [sdk::refresh_render]
	auto refresh_render() const -> uintptr_t {
		const auto visual_engine = mem.read<uintptr_t>(mem.base_address + offsets::VisualEngine::Pointer);
		const auto render_view = mem.read<uintptr_t>(visual_engine + offsets::VisualEngine::RenderView);
		mem.write<uint8_t>(render_view + offsets::RenderView::LightingValid, 0);

		return 0;
	}

	// [sdk::set_ambient]
	auto set_ambient(float r, float g, float b) const -> void {

		const auto datamodel = get_datamodel();
		if (!datamodel) return;

		const auto lighting = find_first_class(*datamodel, HIDE_STR("Lighting"));
		if (!lighting) return;

		for (const auto& child : get_children(*lighting)) {
			if (get_class_name(child) == HIDE_STR("Atmosphere")) {
				mem.write<uintptr_t>(child + offsets::Instance::Parent, 0);
			}
		}

		struct color3 { float r, g, b; };
		const color3 target_color = { r, g, b };
		mem.write<color3>(*lighting + offsets::Lighting::Ambient, target_color);
		mem.write<color3>(*lighting + offsets::Lighting::OutdoorAmbient, target_color);
		mem.write<double>(*lighting + offsets::Lighting::ClockTime, 0.0);
		const auto visual_engine = mem.read<uintptr_t>(mem.base_address + offsets::VisualEngine::Pointer);
		if (visual_engine) {
			const auto render_view = mem.read<uintptr_t>(visual_engine + offsets::VisualEngine::RenderView);
			if (render_view) {
				mem.write<uint8_t>(render_view + offsets::RenderView::LightingValid, 0);
			}
		}
	}

	// [sdk::set_bloom]
	auto set_bloom(float intensity, float size, float threshold) const -> void {

		const auto datamodel = get_datamodel();
		if (!datamodel) return;

		const auto lighting = find_first_class(*datamodel, HIDE_STR("Lighting"));
		if (!lighting) return;

		const auto bloom = find_first_class(*lighting, HIDE_STR("BloomEffect"));
		if (!bloom) return;

		mem.write<float>(*bloom + offsets::BloomEffect::Intensity, intensity);
		mem.write<float>(*bloom + offsets::BloomEffect::Size, size);
		mem.write<float>(*bloom + offsets::BloomEffect::Threshold, threshold);

		const auto visual_engine = mem.read<uintptr_t>(mem.base_address + offsets::VisualEngine::Pointer);
		if (visual_engine) {
			const auto render_view = mem.read<uintptr_t>(visual_engine + offsets::VisualEngine::RenderView);
			if (render_view) {
				mem.write<uint8_t>(render_view + offsets::RenderView::LightingValid, 0);
			}
		}

	}

	// [sdk::set_fov]
	auto set_fov(float value) const -> void {

		const auto datamodel = get_datamodel();
		if (!datamodel) return;

		const auto workspace = find_first_class(*datamodel, HIDE_STR("Workspace"));
		if (!workspace) return;

		const auto camera = mem.read<uintptr_t>(*workspace + offsets::Workspace::CurrentCamera);
		if (!camera) return;
		mem.write<float>(camera + offsets::Camera::FieldOfView, value);
	}

	// [sdk::set_camera_offset]
	auto set_camera_offset(uintptr_t humanoid, Vec3 offset) const -> void {
        if (!humanoid) return;
        mem.write<Vec3>(humanoid + offsets::Humanoid::CameraOffset, offset);
	}

	// [sdk::set_viewport_int16]
	auto set_viewport_int16(Vector2Int16 val) const -> void {
		const auto datamodel = get_datamodel();
		if (!datamodel) return;

		const auto workspace = find_first_class(*datamodel, HIDE_STR("Workspace"));
		if (!workspace) return;

		const auto camera = mem.read<uintptr_t>(*workspace + offsets::Workspace::CurrentCamera);
		if (!camera) return;

		mem.write<Vector2Int16>(camera + offsets::Camera::ViewportInt16, val);
	}

	// [sdk::set_stretched_resolution]
	auto set_stretched_resolution(float res_factor, float vx, float vy) const -> void {
		const auto datamodel = get_datamodel();
		if (!datamodel) return;

		const auto workspace = find_first_class(*datamodel, HIDE_STR("Workspace"));
		if (!workspace) return;

		const auto camera = mem.read<uintptr_t>(*workspace + offsets::Workspace::CurrentCamera);
		if (!camera) return;

		CFrame current = mem.read<CFrame>(camera + offsets::Camera::CFrame);
        current.scale_y_axis(res_factor); 
		mem.write<CFrame>(camera + offsets::Camera::CFrame, current);
        
        struct vec2_t { float x, y; };
		mem.write<vec2_t>(camera + offsets::Camera::ViewportSize, { vx, vy });
	}

	// [sdk::get_jobid]
	auto get_jobid() const -> std::string {

		const auto dm = get_datamodel();

		if (!dm.has_value()) {
			return HIDE_STR("DM Not Found.");
		}

		uintptr_t jobadr = dm.value() + offsets::DataModel::JobId;

		return read_roblox_string(jobadr);
	}

	// [sdk::get_gameid]
	auto get_gameid() const -> std::string {

		const auto dm = get_datamodel();

		if (!dm.has_value()) {
			return HIDE_STR("DM Not Found.");
		}

		uintptr_t gameid_ptr = dm.value() + offsets::DataModel::GameId;
		int64_t game_id = mem.read<int64_t>(gameid_ptr);
		return std::to_string(game_id);
	}

	// [sdk::get_placeid]
	auto get_placeid() const -> int64_t {

		const auto dm = get_datamodel();

		uintptr_t placeid_ptr = dm.value() + offsets::DataModel::PlaceId;
		int64_t place_id = mem.read<int64_t>(placeid_ptr);
		return int64_t(place_id);
	}

	// [sdk::get_ping]
	auto get_ping() const -> float {
		const auto dm = get_datamodel();
		if (!dm) return 0.0f;

		// 1. Find Stats Service
		uintptr_t stats = 0;
		for (const auto& child : get_children(*dm)) {
			if (get_class_name(child) == HIDE_STR("Stats")) {
				stats = child;
				break;
			}
		}
		if (!stats) return 0.0f;

		// 2. Find Network child under Stats
		uintptr_t network = 0;
		for (const auto& child : get_children(stats)) {
			if (get_class_name(child) == HIDE_STR("Network")) {
				network = child;
				break;
			}
		}
		if (!network) return 0.0f;

		// 3. Find ServerStatsItem under Network
		uintptr_t server_stats = 0;
		for (const auto& child : get_children(network)) {
			std::string name = get_name(child);
			if (name == HIDE_STR("ServerStatsItem") || name == HIDE_STR("ServerStats") || get_class_name(child).find(HIDE_STR("StatsItem")) != std::string::npos) {
				server_stats = child;
				break;
			}
		}
		if (!server_stats) return 0.0f;

		// Try standard offsets for modern Roblox 64-bit builds
		float ping = mem.read<float>(server_stats + 0x228); 
		if (ping <= 1.0f || ping > 3000.0f) {
			ping = mem.read<float>(server_stats + 0x238);
		}
		if (ping <= 1.0f || ping > 3000.0f) {
			ping = mem.read<float>(server_stats + 0x24C);
		}

		return ping;
	}


	// [sdk::get_game_name]
	auto get_game_name() const -> std::string {

		int64_t place_id = get_placeid();

		if (place_id == 0) return HIDE_STR("Unknown Game");
		std::string url = HIDE_STR("https://games.roblox.com/v1/games/multiget-place-details?placeIds=") + std::to_string(place_id);
		std::string json_response = network::send_request(url);
        std::vector<std::string> keys = { HIDE_STR("\"name\":\""), HIDE_STR("\"name\" :\""), HIDE_STR("\"name\": \"") }; 
        size_t name_pos = std::string::npos;
        size_t key_len = 0;

        for (const auto& k : keys) {
            name_pos = json_response.find(k);
            if (name_pos != std::string::npos) {
                key_len = k.length();
                break;
            }
        }

		if (name_pos != std::string::npos) {
			name_pos += key_len;
			size_t end_pos = json_response.find("\"", name_pos);
			if (end_pos != std::string::npos) {
				return json_response.substr(name_pos, end_pos - name_pos);
			}
		}
        
		return HIDE_STR("Unknown Game");

	}

	// [sdk::safe_stoull]
    static inline uint64_t safe_stoull(const std::string& str) {
        if (str.empty()) return 0;
        char* endptr = nullptr;
        return std::strtoull(str.c_str(), &endptr, 10);
    }

	// [sdk::extract_asset_id]
    auto extract_asset_id(const std::string& url) const -> uint64_t {
        if (url.empty()) return 0;
        
        size_t pos = url.find(HIDE_STR("id="));
        if (pos != std::string::npos) {
            return safe_stoull(url.substr(pos + 3));
        }
        
        pos = url.find(HIDE_STR("://"));
        if (pos != std::string::npos) {
            return safe_stoull(url.substr(pos + 3));
        }
        
        return safe_stoull(url);
    }

	/*
    auto update_skybox() const -> void {
        const auto dm = get_datamodel();
        if (!dm) return;

        const auto lighting = find_first_class(*dm, HIDE_STR("Lighting"));
        if (!lighting) return;

        const auto sky = find_first_class(*lighting, HIDE_STR("Sky"));
        if (!sky) return;

        write_roblox_string(*sky + offsets::Sky::SkyboxBk, globals::world::skybox_bk);
        write_roblox_string(*sky + offsets::Sky::SkyboxDn, globals::world::skybox_dn);
        write_roblox_string(*sky + offsets::Sky::SkyboxFt, globals::world::skybox_ft);
        write_roblox_string(*sky + offsets::Sky::SkyboxLf, globals::world::skybox_lf);
        write_roblox_string(*sky + offsets::Sky::SkyboxRt, globals::world::skybox_rt);
        write_roblox_string(*sky + offsets::Sky::SkyboxUp, globals::world::skybox_up);

        const auto visual_engine = mem.read<uintptr_t>(mem.base_address + offsets::VisualEngine::Pointer);
        if (visual_engine) {
            const auto render_view = mem.read<uintptr_t>(visual_engine + offsets::VisualEngine::RenderView);
            if (render_view) {
                mem.write<bool>(render_view + offsets::RenderView::SkyboxValid, false);
                mem.write<bool>(render_view + offsets::RenderView::LightingValid, false);
            }
        }
    }
	*/
};

inline auto game = sdk{};
