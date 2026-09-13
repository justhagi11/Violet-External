#pragma once

// Library Includes.
#include <vector>
#include <cstdint>
#include <memory>
#include <string>

// Internal Includes.
#include "../math/math.hpp" 

namespace mesh_parser
{

    using vec3_t = Vec3;

    struct face_t
    {
        uint32_t a, b, c;
    };

    struct vertex_t
    {
        Vec3 position;
        Vec3 normal;
        Vector2 uv;
        Vec3 tangent;
        uint8_t r, g, b, a;
    };

    struct bounds_t
    {
        Vec3 min;
        Vec3 max;
        Vec3 center;
        Vec3 size;
        bool is_valid = false;
    };

    struct mesh_t
    {
        std::vector<vertex_t> vertices;
        std::vector<face_t> faces;
        std::vector<uint32_t> lods;
        bounds_t bounds;
        std::string version;
    };

    void initialize();
    std::shared_ptr<mesh_t> get_mesh(uint64_t asset_id);
}
