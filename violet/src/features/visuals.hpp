#pragma once

#include "../game/sdk.hpp"
#include "../game/entity_cache.hpp"
#include "../core/globals.hpp"
#include "../render/overlay.hpp"
#include "../mesh/parse.hpp"
#include <algorithm>
#include <sstream>
#include <array>

namespace visuals
{
	// [visuals::draw_bone_line]
	inline auto draw_bone_line(const bone_info* bones,
		bone_index from, bone_index to,
		const ViewMatrix& vm, int w, int h, ImU32 color) -> void
	{
		const auto& b_from = bones[from];
		const auto& b_to = bones[to];
		if (!b_from.valid || !b_to.valid || !b_from.primitive || !b_to.primitive) return;

		Vec3 pos_from = b_from.position;
		Vec3 pos_to = b_to.position;

		Vector2 screen_from, screen_to;
		if (math::world_to_screen(pos_from, vm, w, h, screen_from) &&
			math::world_to_screen(pos_to, vm, w, h, screen_to))
		{
			if (!math::is_on_screen(screen_from, w, h) && !math::is_on_screen(screen_to, w, h)) return;

			float thickness = globals::visuals::skeleton_thickness.load();
			if (globals::visuals::skeleton_shadow.load()) {
				render::line(screen_from, screen_to, render::to_color(0, 0, 0, (color >> IM_COL32_A_SHIFT) & 0xFF), thickness + 2.0f);
			}
			render::line(screen_from, screen_to, color, thickness);
		}
	}

	// [visuals::draw_skeleton]
	inline auto draw_skeleton(const player_entry& entry, const ViewMatrix& vm, int w, int h) -> void {
		const auto& col = globals::visuals::skel_color;
		ImU32 color = render::to_color(
			static_cast<int>(col.r * 255),
			static_cast<int>(col.g * 255),
			static_cast<int>(col.b * 255),
			static_cast<int>(col.a * 255)
		);

		const bone_info* bones = &entry.bones[0];
		bool is_r15 = bones[BONE_UPPER_TORSO].valid;

		if (is_r15) {
			draw_bone_line(bones, BONE_HEAD, BONE_UPPER_TORSO, vm, w, h, color);
			draw_bone_line(bones, BONE_UPPER_TORSO, BONE_LOWER_TORSO, vm, w, h, color);
			draw_bone_line(bones, BONE_LOWER_TORSO, BONE_ROOT, vm, w, h, color);
			draw_bone_line(bones, BONE_UPPER_TORSO, BONE_LEFT_UPPER_ARM, vm, w, h, color);
			draw_bone_line(bones, BONE_LEFT_UPPER_ARM, BONE_LEFT_LOWER_ARM, vm, w, h, color);
			draw_bone_line(bones, BONE_LEFT_LOWER_ARM, BONE_LEFT_HAND, vm, w, h, color);
			draw_bone_line(bones, BONE_UPPER_TORSO, BONE_RIGHT_UPPER_ARM, vm, w, h, color);
			draw_bone_line(bones, BONE_RIGHT_UPPER_ARM, BONE_RIGHT_LOWER_ARM, vm, w, h, color);
			draw_bone_line(bones, BONE_RIGHT_LOWER_ARM, BONE_RIGHT_HAND, vm, w, h, color);
			draw_bone_line(bones, BONE_LOWER_TORSO, BONE_LEFT_UPPER_LEG, vm, w, h, color);
			draw_bone_line(bones, BONE_LEFT_UPPER_LEG, BONE_LEFT_LOWER_LEG, vm, w, h, color);
			draw_bone_line(bones, BONE_LEFT_LOWER_LEG, BONE_LEFT_FOOT, vm, w, h, color);
			draw_bone_line(bones, BONE_LOWER_TORSO, BONE_RIGHT_UPPER_LEG, vm, w, h, color);
			draw_bone_line(bones, BONE_RIGHT_UPPER_LEG, BONE_RIGHT_LOWER_LEG, vm, w, h, color);
			draw_bone_line(bones, BONE_RIGHT_LOWER_LEG, BONE_RIGHT_FOOT, vm, w, h, color);
		}
		else {
			draw_bone_line(bones, BONE_HEAD, BONE_TORSO, vm, w, h, color);
			draw_bone_line(bones, BONE_TORSO, BONE_LEFT_UPPER_ARM, vm, w, h, color);
			draw_bone_line(bones, BONE_TORSO, BONE_RIGHT_UPPER_ARM, vm, w, h, color);
			draw_bone_line(bones, BONE_TORSO, BONE_LEFT_UPPER_LEG, vm, w, h, color);
			draw_bone_line(bones, BONE_TORSO, BONE_RIGHT_UPPER_LEG, vm, w, h, color);
		}
	}

	// [visuals::draw_chams]
	inline auto draw_chams(const player_entry& entry, const ViewMatrix& vm, int w, int h) -> void {
		const auto& col = globals::visuals::chams_color;
		ImU32 face_color = render::to_color(
			static_cast<int>(col.r * 255),
			static_cast<int>(col.g * 255),
			static_cast<int>(col.b * 255),
			static_cast<int>(col.a * 255)
		);
		ImU32 outline_color = render::to_color(
			static_cast<int>(col.r * 255),
			static_cast<int>(col.g * 255),
			static_cast<int>(col.b * 255),
			255
		);

		bool fill = globals::visuals::chams_fill.load();
		bool wireframe = globals::visuals::chams_wireframe.load();
		if (!fill && !wireframe) return;

		static const std::vector<mesh_parser::vertex_t> cube_vertices = {
			{ {-0.5f, -0.5f, -0.5f} }, { { 0.5f, -0.5f, -0.5f} }, { { 0.5f,  0.5f, -0.5f} }, { {-0.5f,  0.5f, -0.5f} },
			{ {-0.5f, -0.5f,  0.5f} }, { { 0.5f, -0.5f,  0.5f} }, { { 0.5f,  0.5f,  0.5f} }, { {-0.5f,  0.5f,  0.5f} }
		};
		static const std::vector<mesh_parser::face_t> cube_faces = {
			{0, 1, 2}, {0, 2, 3}, {1, 5, 6}, {1, 6, 2}, {5, 4, 7}, {5, 7, 6},
			{4, 0, 3}, {4, 3, 7}, {3, 2, 6}, {3, 6, 7}, {4, 5, 1}, {4, 1, 0}
		};

		bool is_r15 = entry.bones[BONE_UPPER_TORSO].valid;
		for (int i = 0; i < BONE_COUNT; ++i) {
			const auto& b_info = entry.bones[i];
			if (!b_info.valid || !b_info.primitive) continue;

			if (i == BONE_ROOT || (is_r15 && i == BONE_TORSO)) continue;

			CFrame cframe = b_info.cframe;
			Vec3 part_size = b_info.size;
			
			const std::vector<mesh_parser::vertex_t>* vertices_ptr = nullptr;
			const std::vector<mesh_parser::face_t>* faces_ptr = nullptr;
			Vec3 scale = { 1.0f, 1.0f, 1.0f };

			uint64_t asset_id = b_info.mesh.asset_id;
			if (asset_id != 0) {
				auto mesh = mesh_parser::get_mesh(asset_id);
				if (mesh && !mesh->faces.empty()) {
					vertices_ptr = &mesh->vertices;
					faces_ptr = &mesh->faces;

					if (b_info.mesh.is_mesh_part) {
						if (mesh->bounds.is_valid && mesh->bounds.size.x > 0.01f) {
							scale.x = part_size.x / mesh->bounds.size.x;
							scale.y = part_size.y / mesh->bounds.size.y;
							scale.z = part_size.z / mesh->bounds.size.z;
						} else scale = part_size;
					} else if (b_info.mesh.special_mesh_ptr) {
						Vec3 mesh_scale = b_info.mesh.mesh_scale;
						scale = { part_size.x * mesh_scale.x, part_size.y * mesh_scale.y, part_size.z * mesh_scale.z };
						if (mesh->bounds.is_valid && mesh->bounds.size.x > 0.01f) {
							scale.x /= mesh->bounds.size.x; scale.y /= mesh->bounds.size.y; scale.z /= mesh->bounds.size.z;
						}
					}
				}
			}

			if (!vertices_ptr) {
				vertices_ptr = &cube_vertices;
				faces_ptr = &cube_faces;
				scale = (i == BONE_HEAD) ? Vec3{ 1.15f, 1.15f, 1.15f } : part_size;
			}

			if (fill) render::draw_chams_mesh(*vertices_ptr, *faces_ptr, vm, cframe, scale, face_color, false);
			if (wireframe) render::draw_chams_mesh(*vertices_ptr, *faces_ptr, vm, cframe, scale, outline_color, true);
		}
	}

	// [visuals::draw_esp]
	inline auto draw_esp(const player_entry& entry, const ViewMatrix& vm, int w, int h, const Vec3& local_pos, float dist_to_camera) -> void {
		float scale = (std::max)(0.65f, (std::min)(1.0f, 150.0f / dist_to_camera));
		Vector2 box_pos = {0, 0};
		Vector2 box_size = {0, 0};
		bool should_draw = false;

		int type = globals::visuals::box_type.load();
		bool is_r15 = entry.bones[BONE_UPPER_TORSO].valid;

		if (type == 0 || type == 2) {
			Vector2 min_bounds = { FLT_MAX, FLT_MAX };
			Vector2 max_bounds = { -FLT_MAX, -FLT_MAX };
			int on_screen_count = 0;

			for (int i = 0; i < BONE_COUNT; ++i) {
				const auto& b_info = entry.bones[i];
				if (!b_info.valid || !b_info.primitive) continue;
				Vec3 pos = b_info.position;

				if (i == BONE_HEAD) pos.y += 0.5f;

				Vector2 screen_pos;
				if (math::world_to_screen(pos, vm, w, h, screen_pos)) {
					if (screen_pos.x < min_bounds.x) min_bounds.x = screen_pos.x;
					if (screen_pos.y < min_bounds.y) min_bounds.y = screen_pos.y;
					if (screen_pos.x > max_bounds.x) max_bounds.x = screen_pos.x;
					if (screen_pos.y > max_bounds.y) max_bounds.y = screen_pos.y;
					on_screen_count++;
				}
			}

			if (on_screen_count > 0) {
				float padding = (max_bounds.y - min_bounds.y) * 0.05f;
				min_bounds.x -= padding; max_bounds.x += padding;
				min_bounds.y -= padding; max_bounds.y += padding;

				float width = max_bounds.x - min_bounds.x;
				float height = max_bounds.y - min_bounds.y;

				if (width <= w * 3.0f && height <= h * 3.0f && width > 0 && height > 0) {
					box_pos = { min_bounds.x + 3.0f, min_bounds.y + 3.0f };
					box_size = { width, height };
					should_draw = true;
				}
			}
		} else if (type == 1 || type == 3) {
			Vec3 root_pos = { 0, 0, 0 };
			bool has_root_pos = false;
			if (entry.bones[BONE_ROOT].valid) { root_pos = entry.bones[BONE_ROOT].position; has_root_pos = true; }
			else if (is_r15 && entry.bones[BONE_LOWER_TORSO].valid) { root_pos = entry.bones[BONE_LOWER_TORSO].position; has_root_pos = true; }
			else if (entry.bones[BONE_TORSO].valid) { root_pos = entry.bones[BONE_TORSO].position; has_root_pos = true; }

			if (has_root_pos) {
				
				Vector2 s_pos;
				if (math::world_to_screen(root_pos, vm, w, h, s_pos)) {
					float dist = (root_pos.x * vm.data[12]) + (root_pos.y * vm.data[13]) + (root_pos.z * vm.data[14]) + vm.data[15];
					if (dist < 1.0f) dist = 1.0f;

					float height = (2000.0f / dist);
					float width = height * 0.6f;

					if (width <= w * 3.0f && height <= h * 3.0f) {
						box_pos = { (s_pos.x - (width * 0.5f)) + 3.0f, (s_pos.y - (height * 0.5f)) + 3.0f };
						box_size = { width, height };
						should_draw = true;
					}
				}
			}
		}

		if (!should_draw) return;

		if (globals::visuals::box.load()) {
			const auto& col = globals::visuals::box_color;
			ImU32 color = render::to_color(
				static_cast<int>(col.r * 255), static_cast<int>(col.g * 255),
				static_cast<int>(col.b * 255), static_cast<int>(col.a * 255)
			);
			ImU32 outline_col = render::to_color(0, 0, 0, static_cast<int>(col.a * 255));

			const auto& f_col = globals::visuals::box_filled_color;
			const auto& f_col2 = globals::visuals::box_filled_color2;
			ImU32 fill_color = render::to_color(
				(int)(f_col.r * 255.0f), (int)(f_col.g * 255.0f),
				(int)(f_col.b * 255.0f), (int)(f_col.a * 255.0f)
			);
			ImU32 fill_color2 = render::to_color(
				(int)(f_col2.r * 255.0f), (int)(f_col2.g * 255.0f),
				(int)(f_col2.b * 255.0f), (int)(f_col2.a * 255.0f)
			);

			bool outline = globals::visuals::box_outline.load();
			float rounding = globals::visuals::box_rounding.load();
			bool filled = globals::visuals::box_filled.load();
			bool gradient = globals::visuals::box_gradient.load();

			if (filled) {
				if (gradient) render::gradient_vertical(box_pos, box_size, fill_color, fill_color2);
				else render::rect_filled(box_pos, box_size, fill_color, rounding);
			}

			if (type == 2 || type == 3) {
				float line_w = box_size.x / 4.0f;
				float line_h = box_size.y / 4.0f;
				
				auto draw_corner = [&](Vector2 p1, Vector2 p2, Vector2 p3) {
					if (outline) {
						render::line({p1.x, p1.y}, {p2.x, p2.y}, outline_col, 3.0f);
						render::line({p2.x, p2.y}, {p3.x, p3.y}, outline_col, 3.0f);
					}
					render::line({p1.x, p1.y}, {p2.x, p2.y}, color, 1.0f);
					render::line({p2.x, p2.y}, {p3.x, p3.y}, color, 1.0f);
				};

				draw_corner({box_pos.x, box_pos.y + line_h}, {box_pos.x, box_pos.y}, {box_pos.x + line_w, box_pos.y});
				draw_corner({box_pos.x + box_size.x - line_w, box_pos.y}, {box_pos.x + box_size.x, box_pos.y}, {box_pos.x + box_size.x, box_pos.y + line_h});
				draw_corner({box_pos.x, box_pos.y + box_size.y - line_h}, {box_pos.x, box_pos.y + box_size.y}, {box_pos.x + line_w, box_pos.y + box_size.y});
				draw_corner({box_pos.x + box_size.x - line_w, box_pos.y + box_size.y}, {box_pos.x + box_size.x, box_pos.y + box_size.y}, {box_pos.x + box_size.x, box_pos.y + box_size.y - line_h});
			} else {
				if (outline) {
					render::rect({ box_pos.x - 1, box_pos.y - 1 }, { box_size.x + 2, box_size.y + 2 }, outline_col, rounding, 1.0f);
					render::rect({ box_pos.x + 1, box_pos.y + 1 }, { box_size.x - 2, box_size.y - 2 }, outline_col, rounding, 1.0f);
				}
				render::rect(box_pos, box_size, color, rounding, 1.0f);
			}
		}

		if (globals::visuals::hp_bar.load()) {
			float hp = entry.hp;
			float max_hp = entry.max_hp > 0.0f ? entry.max_hp : 100.0f;
			if (hp > max_hp) hp = max_hp;
			
			float hp_perc = hp / max_hp;
			float bar_width = globals::visuals::hp_bar_width.load();
			float padding = 4.0f;
			
			Vector2 bar_pos = { box_pos.x - bar_width - padding, box_pos.y };
			Vector2 bar_size = { bar_width, box_size.y };

			if (globals::visuals::hp_bar_outline.load()) {
				render::rect_filled({ bar_pos.x - 1.0f, bar_pos.y - 1.0f }, { bar_size.x + 2.0f, bar_size.y + 2.0f }, render::to_color(0, 0, 0, 180));
			}

			float active_height = (std::max)(0.0f, bar_size.y * hp_perc);
			Vector2 active_pos = { bar_pos.x, bar_pos.y + bar_size.y - active_height };
			Vector2 active_size = { bar_size.x, active_height };

			const auto& c1 = globals::visuals::hp_bar_color1;
			const auto& c2 = globals::visuals::hp_bar_color2;
			ImU32 col1 = render::to_color(static_cast<int>(c1.r * 255), static_cast<int>(c1.g * 255), static_cast<int>(c1.b * 255), 255);
			ImU32 col2 = render::to_color(static_cast<int>(c2.r * 255), static_cast<int>(c2.g * 255), static_cast<int>(c2.b * 255), 255);

			if (globals::visuals::hp_bar_gradient.load()) {
				render::gradient_vertical(active_pos, active_size, col1, col2);
			} else {
				ImU32 hp_color = render::to_color(
					(int)((c2.r + (c1.r - c2.r) * hp_perc) * 255.0f),
					(int)((c2.g + (c1.g - c2.g) * hp_perc) * 255.0f),
					(int)((c2.b + (c1.b - c2.b) * hp_perc) * 255.0f),
					255
				);
				render::rect_filled(active_pos, active_size, hp_color);
			}

			if (globals::visuals::display_health_value.load() && hp < max_hp) {
				std::string hp_str = std::to_string(static_cast<int>(hp));
				ImVec2 ts = ImGui::CalcTextSize(hp_str.c_str());
				
				float hp_text_gap = 3.0f * scale; 
				Vector2 text_pos = { bar_pos.x - (ts.x * scale) - hp_text_gap, active_pos.y - ((ts.y * scale) * 0.5f) };
				
				render::text(text_pos, hp_str, render::to_color(255, 255, 255, 255), false, scale);
			}
		}

		if (globals::visuals::display_name.load()) {
			std::string name = entry.name;
			if (name.empty()) name = HIDE_STR("Unknown");
			
			Vector2 name_pos = { box_pos.x + box_size.x * 0.5f, box_pos.y - (14.0f * scale) };
			render::text(name_pos, name, render::to_color(255, 255, 255, 255), true, scale);
		}

		std::vector<std::pair<std::string, ImU32>> bottom_text;
		
		if (globals::visuals::display_tool.load() && !entry.weapon.empty()) {
			bottom_text.push_back({ entry.weapon, render::to_color(255, 255, 255, 255) });
		}

		if (globals::visuals::display_dist.load()) {
			float dist = 0.0f;
			if (local_pos.x != 0.0f) {
				Vec3 root_pos = { 0, 0, 0 };
				if (entry.bones[BONE_ROOT].valid) root_pos = entry.bones[BONE_ROOT].position;
				else if (entry.bones[BONE_LOWER_TORSO].valid) root_pos = entry.bones[BONE_LOWER_TORSO].position;
				else if (entry.bones[BONE_TORSO].valid) root_pos = entry.bones[BONE_TORSO].position;
				if (root_pos.x != 0.0f || root_pos.y != 0.0f || root_pos.z != 0.0f)
					dist = local_pos.dist(root_pos);
			}

			if (dist > 0.0f) {
				bottom_text.push_back({ std::to_string(static_cast<int>(dist)) + HIDE_STR(" studs"), render::to_color(200, 200, 200, 255) });
			}
		}

		float text_y_offset = box_pos.y + box_size.y + (6.0f * scale);
		for (const auto& item : bottom_text) {
			Vector2 t_pos = { box_pos.x + box_size.x * 0.5f, text_y_offset };
			render::text(t_pos, item.first, item.second, true, scale);
			text_y_offset += (12.0f * scale);
		}
	}

	// [visuals::draw_3d_box]
	inline auto draw_3d_box(const CFrame& cframe, const Vec3& size, const ViewMatrix& vm, int w, int h, ImU32 color) -> void {
		static const Vec3 corners[8] = {
			{-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f},
			{-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}
		};

		Vector2 screen_corners[8];
		bool visible[8];

		for (int i = 0; i < 8; ++i) {
			Vec3 world_pos = cframe.point_to_world({corners[i].x * size.x, corners[i].y * size.y, corners[i].z * size.z});
			visible[i] = math::world_to_screen(world_pos, vm, w, h, screen_corners[i]);
		}

		auto draw_edge = [&](int i, int j) {
			if (visible[i] && visible[j]) {
				render::line(screen_corners[i], screen_corners[j], color, 1.0f);
			}
		};

		draw_edge(0, 1); draw_edge(1, 2); draw_edge(2, 3); draw_edge(3, 0);
		draw_edge(4, 5); draw_edge(5, 6); draw_edge(6, 7); draw_edge(7, 4);
		draw_edge(0, 4); draw_edge(1, 5); draw_edge(2, 6); draw_edge(3, 7);
	}

	// [visuals::run]
	inline auto run() -> void {
		uintptr_t local_humanoid = 0;
		Vec3 local_pos = { 0, 0, 0 };
		auto snap = cache::get_snapshot();
		for (const auto& p : snap->players) {
			if (p.is_local) {
				local_humanoid = p.humanoid;
				if (p.root_primitive) {
					local_pos = mem.read<Vec3>(p.root_primitive + offsets::Primitive::Position);
				} else if (p.bones[BONE_ROOT].valid) {
					local_pos = p.bones[BONE_ROOT].position;
				}
				break;
			}
		}

		game.set_camera_offset(local_humanoid, globals::world::third_person.load() ? Vec3(0, 2, 12) : Vec3(0, 0, 0));

		if (globals::world::stretched_res_enabled.load()) {
			game.set_stretched_resolution(
				0.80f, 
				globals::world::stretched_res_x.load(), 
				globals::world::stretched_res_y.load()
			);
		}

		if (snap->players.size() <= 1 && !globals::visuals::show_models.load()) return;

		const auto vm = math::get_view_matrix();
		const auto viewport = math::get_viewport_size();
		int w = static_cast<int>(viewport.x);
		int h = static_cast<int>(viewport.y);

		for (const auto& entry : snap->players) {
			if (entry.is_local || !entry.alive) continue;
			if (globals::visuals::team_check.load() && entry.is_teammate) continue;

			bool is_r15 = entry.bones[BONE_UPPER_TORSO].valid;

			// Find a valid root primitive to read live position (1 syscall per player)
			uintptr_t root_prim = entry.root_primitive;
			if (!root_prim && is_r15) root_prim = entry.bones[BONE_LOWER_TORSO].primitive;
			if (!root_prim) root_prim = entry.bones[BONE_TORSO].primitive;
			if (!root_prim) continue;

			// Read live root position for smooth tracking
			Vec3 live_root = mem.read<Vec3>(root_prim + offsets::Primitive::Position);

			float w_cam = (live_root.x * vm.data[12]) + (live_root.y * vm.data[13]) + (live_root.z * vm.data[14]) + vm.data[15];
			if (w_cam < 0.01f) continue;

			Vector2 screen_pos;
			if (!math::world_to_screen(live_root, vm, w, h, screen_pos)) continue;
			if (!math::is_on_screen(screen_pos, w, h, 250.0f)) continue;

			const bool any_visual = globals::visuals::chams.load() || globals::visuals::skeleton.load()
				|| globals::visuals::box.load() || globals::visuals::hp_bar.load()
				|| globals::visuals::display_name.load() || globals::visuals::display_dist.load()
				|| globals::visuals::display_tool.load();
			if (!any_visual) continue;

			// Compute delta between live root and cached root, shift all bones
			Vec3 cached_root = { 0, 0, 0 };
			if (entry.bones[BONE_ROOT].valid) cached_root = entry.bones[BONE_ROOT].position;
			else if (is_r15 && entry.bones[BONE_LOWER_TORSO].valid) cached_root = entry.bones[BONE_LOWER_TORSO].position;
			else if (entry.bones[BONE_TORSO].valid) cached_root = entry.bones[BONE_TORSO].position;

			Vec3 delta = live_root - cached_root;

			player_entry live = entry;
			for (int i = 0; i < BONE_COUNT; ++i) {
				if (live.bones[i].valid) {
					live.bones[i].position = live.bones[i].position + delta;
					live.bones[i].cframe.x += delta.x;
					live.bones[i].cframe.y += delta.y;
					live.bones[i].cframe.z += delta.z;
				}
			}

			if (globals::visuals::chams.load()) draw_chams(live, vm, w, h);
			if (globals::visuals::skeleton.load()) draw_skeleton(live, vm, w, h);

			if (globals::visuals::box.load() || globals::visuals::hp_bar.load() ||
				globals::visuals::display_name.load() || globals::visuals::display_dist.load() ||
				globals::visuals::display_tool.load())
			{
				draw_esp(live, vm, w, h, local_pos, w_cam);
			}
		}

		if (globals::visuals::show_models.load()) {
			for (const auto& entry : snap->models) {
				Vector2 screen_pos;
				if (math::world_to_screen(entry.position, vm, w, h, screen_pos)) {
					if (math::is_on_screen(screen_pos, w, h)) {
						render::text(screen_pos, entry.name, render::to_color(255, 255, 255, 255), true, 0.9f);
						draw_3d_box(entry.cframe, entry.size, vm, w, h, render::to_color(255, 255, 255, 180));
					}
				}
			}

			for (const auto& entry : snap->parts) {
				Vector2 screen_pos;
				if (math::world_to_screen(entry.position, vm, w, h, screen_pos)) {
					if (math::is_on_screen(screen_pos, w, h)) {
						render::text(screen_pos, entry.name, render::to_color(200, 200, 200, 255), true, 0.7f);
						draw_3d_box(entry.cframe, entry.size, vm, w, h, render::to_color(200, 200, 200, 150));
					}
				}
			}
		}
	}
}
