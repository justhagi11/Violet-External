#include "snow.hpp"
#include "overlay.hpp"
#include "../core/globals.hpp"
#include "../game/sdk.hpp"
#include "../math/math.hpp"

#include <vector>
#include <random>
#include <chrono>
#include <cmath>

struct Particle {
    Vec3 pos;
    float speed;
    float drift_phase;
    float size;
};

static std::vector<Particle> particles;
static std::mt19937 rng;
static auto last_time = std::chrono::high_resolution_clock::now();

// [snow::rand_float]
static float rand_float(float lo, float hi) {
    return std::uniform_real_distribution<float>(lo, hi)(rng);
}

// [snow::spawn_particle]
static void spawn_particle(Particle& p, Vec3 origin, bool randomize_y) {
    float radius = globals::weather::spawn_radius.load();
    float height = globals::weather::spawn_height.load();
    float smin = globals::weather::size_min.load();
    float smax = globals::weather::size_max.load();

    p.pos.x = origin.x + rand_float(-radius, radius);
    p.pos.z = origin.z + rand_float(-radius, radius);

    if (randomize_y) {
        p.pos.y = origin.y + rand_float(25.0f, height);
    }
    else {
        p.pos.y = origin.y + rand_float(height * 0.8f, height);
    }

    p.speed = rand_float(0.7f, 1.3f);
    p.drift_phase = rand_float(0.0f, PI_2);
    p.size = rand_float(smin, smax);
}

// [snow::get_camera_position]
static bool get_camera_position(Vec3& out) {
    auto dm = game.get_datamodel();
    if (!dm) return false;

    auto workspace = game.find_first_class(*dm, "Workspace");
    if (!workspace) return false;

    auto camera = mem.read<uintptr_t>(*workspace + offsets::Workspace::CurrentCamera);
    if (!camera) return false;

    out = mem.read<Vec3>(camera + offsets::Camera::Position);
    return (out.x != 0.0f || out.y != 0.0f || out.z != 0.0f);
}

// [snow::initialize]
void snow::initialize() {
    rng.seed(static_cast<unsigned>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    last_time = std::chrono::high_resolution_clock::now();
    particles.clear();
}

// [snow::update_and_render]
void snow::update_and_render() {
    if (!globals::weather::enabled.load()) return;

    auto now = std::chrono::high_resolution_clock::now();
    float dt = std::chrono::duration<float>(now - last_time).count();
    last_time = now;
    if (dt > 0.1f) dt = 0.1f;

    int target_count = globals::weather::particle_count.load();
    float fall_speed = globals::weather::fall_speed.load();
    float drift_str = globals::weather::drift_strength.load();
    int mode = globals::weather::mode.load();
    color_t col = globals::weather::particle_color;
    static Vec3 last_known_pos = { 0, 0, 0 };
    static bool ever_had_pos = false;
    Vec3 cam_pos = last_known_pos;

    if (get_camera_position(cam_pos)) {
        last_known_pos = cam_pos;
        ever_had_pos = true;
    }

    if (!ever_had_pos) return;


    if ((int)particles.size() < target_count) {
        int old_size = (int)particles.size();
        particles.resize(target_count);
        for (int i = old_size; i < target_count; i++) {
            spawn_particle(particles[i], cam_pos, true);
        }
    }
    else if ((int)particles.size() > target_count) {
        particles.resize(target_count);
    }

    ViewMatrix vm = math::get_view_matrix();
    Vector2 vp = math::get_viewport_size();
    int vw = static_cast<int>(vp.x);
    int vh = static_cast<int>(vp.y);
    if (vw <= 0 || vh <= 0) return;

    float spawn_h = globals::weather::spawn_height.load();
    float time_now = std::chrono::duration<float>(
        now.time_since_epoch()).count();

    ImU32 draw_color = render::to_color(
        static_cast<int>(col.r * 255),
        static_cast<int>(col.g * 255),
        static_cast<int>(col.b * 255),
        static_cast<int>(col.a * 255));

    for (auto& p : particles) {
        p.pos.y -= fall_speed * p.speed * dt;

        if (mode == 0) {
            float drift_x = sinf(time_now * 1.5f + p.drift_phase) * drift_str * dt;
            float drift_z = cosf(time_now * 1.1f + p.drift_phase * 0.7f) * drift_str * dt * 0.6f;
            p.pos.x += drift_x;
            p.pos.z += drift_z;
        }
        else {
            p.pos.x += drift_str * dt * 0.8f;
            p.pos.z += drift_str * dt * 0.3f;
        }

        float ground_y = cam_pos.y + 20.0f;
        if (p.pos.y < ground_y) {
            spawn_particle(p, cam_pos, false);
            continue;
        }

        float fade_zone = 12.0f;
        float dist_to_ground = p.pos.y - ground_y;
        float ground_fade = (dist_to_ground < fade_zone) ? (dist_to_ground / fade_zone) : 1.0f;
        if (ground_fade < 0.0f) ground_fade = 0.0f;

        float dx = p.pos.x - cam_pos.x;
        float dz = p.pos.z - cam_pos.z;
        float dist_sq = dx * dx + dz * dz;
        float radius = globals::weather::spawn_radius.load();
        if (dist_sq > radius * radius * 1.5f) {
            spawn_particle(p, cam_pos, true);
            continue;
        }

        Vector2 screen;
        if (!math::world_to_screen(p.pos, vm, vw, vh, screen)) {
            continue;
        }

        float w = (p.pos.x * vm.data[12]) + (p.pos.y * vm.data[13]) + (p.pos.z * vm.data[14]) + vm.data[15];
        float depth_scale = 1.0f;
        if (w > 1.0f) depth_scale = 15.0f / w;
        if (depth_scale > 2.5f) depth_scale = 2.5f;
        if (depth_scale < 0.3f) depth_scale = 0.3f;

        float draw_size = p.size * depth_scale;

        int alpha = static_cast<int>(col.a * 255.0f * ground_fade);
        ImU32 particle_color = render::to_color(
            static_cast<int>(col.r * 255.0f),
            static_cast<int>(col.g * 255.0f),
            static_cast<int>(col.b * 255.0f),
            alpha);

        if (mode == 0) {
            render::rect_filled(
                { screen.x - draw_size * 0.5f, screen.y - draw_size * 0.5f },
                { draw_size, draw_size },
                particle_color,
                draw_size * 0.5f);
        }
        else {
            float streak_len = draw_size * 4.0f;
            render::line(
                { screen.x, screen.y },
                { screen.x + draw_size * 0.3f, screen.y + streak_len },
                particle_color,
                draw_size * 0.4f);
        }
    }
}
