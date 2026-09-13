#pragma once


#include "Windows.h"
#include <vector>
#include <string>
#include <cmath>


#include "types.hpp"
#include "../io/memory/memory.hpp"
#include "../game/offsets.hpp"


#ifndef PI
#define PI 3.14159265358979323846f
#endif

#ifndef PI_2
#define PI_2 6.28318530717958647692f
#endif

#define DEG2RAD(x) ((x) * (PI / 180.f))
#define RAD2DEG(x) ((x) * (180.f / PI))



namespace math {

    // [math::world_to_screen]
    inline bool world_to_screen(Vec3 pos, const ViewMatrix& matrix, int width, int height, Vector2& out) {
        float w = (pos.x * matrix.data[12]) + (pos.y * matrix.data[13]) + (pos.z * matrix.data[14]) + matrix.data[15];
        if (w < 0.01f) return false;

        float x = (pos.x * matrix.data[0]) + (pos.y * matrix.data[1]) + (pos.z * matrix.data[2]) + matrix.data[3];
        float y = (pos.x * matrix.data[4]) + (pos.y * matrix.data[5]) + (pos.z * matrix.data[6]) + matrix.data[7];

        float inv_w = 1.0f / w;
        float ndc_x = x * inv_w;
        float ndc_y = y * inv_w;

        out.x = (width * 0.5f) + (ndc_x * width * 0.5f);
        out.y = (height * 0.5f) - (ndc_y * height * 0.5f);

        return true;
    }

    // [math::is_on_screen]
    inline bool is_on_screen(Vector2 pos, int width, int height, float margin = 0.0f) {
        return (pos.x >= -margin && pos.x <= (float)width + margin &&
                pos.y >= -margin && pos.y <= (float)height + margin);
    }

    // [math::calculate_look_at]
    inline Matrix3x3 calculate_look_at(Vec3 origin, Vec3 target) {
        Vec3 forward = origin - target;
        float dist = std::sqrtf(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
        if (dist < 0.0001f) return { 1, 0, 0, 0, 1, 0, 0, 0, 1 };

        forward.x /= dist; forward.y /= dist; forward.z /= dist;

        Vec3 up = { 0, 1, 0 };
        Vec3 right = {
            up.y * forward.z - up.z * forward.y,
            up.z * forward.x - up.x * forward.z,
            up.x * forward.y - up.y * forward.x
        };

        float right_dist = std::sqrtf(right.x * right.x + right.y * right.y + right.z * right.z);
        if (right_dist < 0.0001f) right = { 1, 0, 0 };
        else { right.x /= right_dist; right.y /= right_dist; right.z /= right_dist; }

        Vec3 actual_up = {
            forward.y * right.z - forward.z * right.y,
            forward.z * right.x - forward.x * right.z,
            forward.x * right.y - forward.y * right.x
        };

        return {
            right.x, actual_up.x, forward.x,
            right.y, actual_up.y, forward.y,
            right.z, actual_up.z, forward.z
        };
    }

    // [math::calculate_silent_viewport]
    inline Vector2Int16 calculate_silent_viewport(Vector2 target_screen_pos, Vector2 screen_size) {
        Vector2Int16 result;
        result.x = static_cast<int16_t>(2.0f * (screen_size.x - target_screen_pos.x));
        result.y = static_cast<int16_t>(2.0f * (screen_size.y - target_screen_pos.y));
        return result;
    }

    // [math::get_viewport_size]
    inline auto get_viewport_size() -> Vector2 {

        const auto visual_engine = mem.read<uintptr_t>(mem.base_address + offsets::VisualEngine::Pointer);

        return mem.read<Vector2>(visual_engine + offsets::VisualEngine::Dimensions);

    }

    // [math::get_view_matrix]
    inline auto get_view_matrix() -> ViewMatrix {

        const auto visual_engine = mem.read<uintptr_t>(mem.base_address + offsets::VisualEngine::Pointer);
        return mem.read<ViewMatrix>(visual_engine + offsets::VisualEngine::ViewMatrix);

    }

}
