#pragma once


	#include <vector>
	#include <mutex>
	#include <unordered_map>
	#include <string>
	#include <array>


	#include "../math/math.hpp"

enum bone_index {
    BONE_HEAD, BONE_TORSO, BONE_UPPER_TORSO, BONE_LOWER_TORSO, BONE_ROOT,
    BONE_LEFT_UPPER_ARM, BONE_LEFT_LOWER_ARM, BONE_LEFT_HAND,
    BONE_RIGHT_UPPER_ARM, BONE_RIGHT_LOWER_ARM, BONE_RIGHT_HAND,
    BONE_LEFT_UPPER_LEG, BONE_LEFT_LOWER_LEG, BONE_LEFT_FOOT,
    BONE_RIGHT_UPPER_LEG, BONE_RIGHT_LOWER_LEG, BONE_RIGHT_FOOT,
    BONE_COUNT
};

struct mesh_data {
    uint64_t asset_id = 0;
    bool is_mesh_part = false;
    uintptr_t special_mesh_ptr = 0;
    Vec3 mesh_scale = { 1.0f, 1.0f, 1.0f };
};

struct bone_info {
    bool valid = false;
    uintptr_t instance;
    uintptr_t primitive;
    mesh_data mesh;
    Vec3 position;
    CFrame cframe;
    Vec3 size;
};

struct player_entry {

	uintptr_t instance;
	uintptr_t character;
	uintptr_t root_part;
    uintptr_t root_primitive;
	std::string name;
	bool is_local;
	bool alive;
	bool is_teammate;
	float hp;
	float max_hp;
	Vec3 velocity;
	uintptr_t humanoid;
	std::string weapon;
	std::array<bone_info, BONE_COUNT> bones;

};

struct model_entry {
	uintptr_t instance;
	std::string name;
	Vec3 position;
	CFrame cframe;
	Vec3 size;
};

struct part_entry {
	uintptr_t instance;
	std::string name;
	Vec3 position;
	CFrame cframe;
	Vec3 size;
};

struct snapshot {
    std::vector<player_entry> players;
    std::vector<model_entry> models;
    std::vector<part_entry> parts;
};

namespace cache {
    std::shared_ptr<const snapshot> get_snapshot();
    auto update() -> void;

    // Re-read primitive transforms using pointers from the last snapshot (call from render path).
    auto refresh_live_transforms(player_entry& entry) -> void;
}
