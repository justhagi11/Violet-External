// Im not letting ai do an module ever again, rewriting this today. Can't believe i let an agent do work on the c++ side :c - hagi.


// Starting rewrite 3/21/2026 2:58AM - hagi.
#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "../game/sdk.hpp"
#include "../game/entity_cache.hpp"
#include "../core/globals.hpp"
#include "../io/memory/memory.hpp"
#include "../math/perlin.hpp"
#include "../render/overlay.hpp"

// Ive decided to keep the modules in separate files so header will be declatarion only - hagi.

/*
		Roadmap


		[+ Implemented Aimbot (3/21/2026)]
		[+] Implemented Silent Aim (4/2/2026)





*/

class aiming {
private:
	static inline uintptr_t CurrentTarget = 0;
	static inline Matrix3x3 backup_rotation;
	static inline bool is_silent_active = false;
	
	static inline int current_random_bone = BONE_HEAD;
	static inline int current_random_silent_bone = BONE_HEAD;

	static void reset_silent_aim(); // restores camera
	static int get_target_bone(int setting, int& random_storage);
	
public:
	static void tick();
};
