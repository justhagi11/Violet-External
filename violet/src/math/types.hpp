#pragma once
#include <cmath>

struct Vector2 { float x, y; };

struct Vec3 {
    float x, y, z;

    constexpr Vec3(float _x = 0.f, float _y = 0.f, float _z = 0.f) noexcept : x(_x), y(_y), z(_z) {}

    constexpr Vec3 operator+(const Vec3& other) const noexcept { return { x + other.x, y + other.y, z + other.z }; }
    constexpr Vec3 operator-(const Vec3& other) const noexcept { return { x - other.x, y - other.y, z - other.z }; }
    constexpr Vec3 operator*(float s) const noexcept { return { x * s, y * s, z * s }; }

    float dist(const Vec3& other) const noexcept {
        float dx = x - other.x;
        float dy = y - other.y;
        float dz = z - other.z;
        return std::sqrtf(dx * dx + dy * dy + dz * dz);
    }
};

struct Vector2Int16 {
    int16_t x, y;
};

struct Matrix3x3 {
    float r00, r01, r02;
    float r10, r11, r12;
    float r20, r21, r22;
};

struct CFrame {
    float r00, r01, r02;
    float r10, r11, r12;
    float r20, r21, r22;
    float x, y, z;

    Vec3 point_to_world(const Vec3& local) const {
        return {
            x + r00 * local.x + r01 * local.y + r02 * local.z,
            y + r10 * local.x + r11 * local.y + r12 * local.z,
            z + r20 * local.x + r21 * local.y + r22 * local.z
        };
    }

    Vec3 point_to_object(const Vec3& world) const {
        Vec3 rel = { world.x - x, world.y - y, world.z - z };
        return {
            rel.x * r00 + rel.y * r10 + rel.z * r20,
            rel.x * r01 + rel.y * r11 + rel.z * r21,
            rel.x * r02 + rel.y * r12 + rel.z * r22
        };
    }

    Vec3 vector_to_object(const Vec3& world) const {
        return {
            world.x * r00 + world.y * r10 + world.z * r20,
            world.x * r01 + world.y * r11 + world.z * r21,
            world.x * r02 + world.y * r12 + world.z * r22
        };
    }

    CFrame operator*(const CFrame& other) const {
        CFrame res;
        res.r00 = r00 * other.r00 + r01 * other.r10 + r02 * other.r20;
        res.r01 = r00 * other.r01 + r01 * other.r11 + r02 * other.r21;
        res.r02 = r00 * other.r02 + r01 * other.r12 + r02 * other.r22;

        res.r10 = r10 * other.r00 + r11 * other.r10 + r12 * other.r20;
        res.r11 = r10 * other.r01 + r11 * other.r11 + r12 * other.r21;
        res.r12 = r10 * other.r02 + r11 * other.r12 + r12 * other.r22;

        res.r20 = r20 * other.r00 + r21 * other.r10 + r22 * other.r20;
        res.r21 = r20 * other.r01 + r21 * other.r11 + r22 * other.r21;
        res.r22 = r20 * other.r02 + r21 * other.r12 + r22 * other.r22;
        Vec3 new_pos = point_to_world({other.x, other.y, other.z});
        res.x = new_pos.x;
        res.y = new_pos.y;
        res.z = new_pos.z;

        return res;
    }

    void scale_y_axis(float scale) {
        r01 *= scale;
        r11 *= scale;
        r21 *= scale;
    }
};

struct ViewMatrix { float data[16]; };
