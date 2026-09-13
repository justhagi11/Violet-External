#pragma once
#include <atomic>
#include <string>
#include <cstdint>
#include <Windows.h>

#ifndef VK_INSERT
#define VK_INSERT 0x2D
#endif

struct color_t {
    float r, g, b, a;

    color_t(float _r = 1.0f, float _g = 1.0f, float _b = 1.0f, float _a = 1.0f)
        : r(_r), g(_g), b(_b), a(_a) {
    }

    float* data() { return &r; }
};

namespace globals {
    inline std::atomic<bool> running{ true };
    inline std::atomic<bool> menu_open{ true };
    inline std::atomic<bool> toolbar_open{ true };
    inline std::atomic<bool> logged_in{ false };
    inline std::atomic<bool> devbuild{ true };
    inline std::atomic<HWND> ov_win{ nullptr };

    namespace visuals {
        inline std::atomic<bool> enabled{ true };
        inline std::atomic<bool> team_check{ false };
        inline std::atomic<bool> skeleton{ true };
        inline std::atomic<bool> box{ true };
        inline std::atomic<bool> box_outline{ true };
        inline std::atomic<float> box_rounding{ 0.0f };
        inline std::atomic<bool> chams{ false };
        inline std::atomic<bool> chams_fill{ false };
        inline std::atomic<bool> chams_wireframe{ false };
        inline std::atomic<bool> display_name{ false };
        inline std::atomic<bool> display_health{ false };
        inline std::atomic<bool> display_health_value{ false };
        inline std::atomic<bool> display_tool{ false };
        inline std::atomic<bool> display_dist{ false };
        inline std::atomic<bool> highlight_target{ false };
        inline std::atomic<bool> box_filled{ false };
        inline std::atomic<bool> box_gradient{ false };

        inline std::atomic<bool> hp_bar{ false };
        inline std::atomic<bool> hp_bar_outline{ false };
        inline std::atomic<bool> hp_bar_gradient{ false };
        inline std::atomic<float> hp_bar_width{ 2.0f };

        inline std::atomic<float> max_dist{ 500.0f };
        inline std::atomic<int> box_type{ 0 };

        inline std::atomic<bool> visual_check{ true };
        inline std::atomic<bool> show_models{ false };
        inline color_t enemy_color = { 1.0f, 1.0f, 1.0f, 1.0f };
        inline color_t team_color = { 1.0f, 1.0f, 1.0f, 1.0f };
        inline color_t skel_color = { 1.0f, 1.0f, 1.0f, 1.0f };
        inline color_t chams_color = { 1.0f, 0.0f, 1.0f, 0.5f };
        inline color_t box_color = { 1.0f, 1.0f, 1.0f, 1.0f };
        inline color_t box_filled_color = { 0.3f, 0.3f, 0.3f, 0.3f };
        inline color_t box_filled_color2 = { 0.7f, 0.7f, 0.7f, 0.3f };
        inline std::atomic<bool> skeleton_shadow{ true };
        inline std::atomic<float> skeleton_thickness{ 1.5f };

        inline color_t hp_bar_color1 = { 0.0f, 1.0f, 0.0f, 1.0f };
        inline color_t hp_bar_color2 = { 1.0f, 0.0f, 0.0f, 1.0f };
    };

    namespace aim {
        inline std::atomic<int> aimbot_mode{ 1 }; // 1 mouse, 2 memory
        inline std::atomic<bool> aimbot_enabled{ true };
        inline std::atomic<int> aimbot_bind1{ 0x02 }; // bind1
        inline std::atomic<int> aimbot_bind2{ 0 }; // bind2
        inline std::atomic<int> aimbot_bind_mode{ 2 }; // 1 always, 2 hold, 3 toggle
        inline std::atomic<float> aimbot_smoothing_x{ 13.0f };
        inline std::atomic<float> aimbot_smoothing_y{ 14.7f };
        inline std::atomic<float> aimbot_smoothing_xy{ 6.0f };
        inline std::atomic<float> aimbot_smoothing_xz{ 6.0f };
        inline std::atomic<float> aimbot_sensitivity{ 0.3f };
        inline std::atomic<bool> aimbot_humanizer{ false };
        inline std::atomic<int> aimbot_prefered_hitpart{ 0 };
        inline std::atomic<int> aimbot_hitpart_blacklist{ 0 };
        inline std::atomic<float> aimbot_fov{ 400.0f };
        inline std::atomic<bool> aimbot_draw_fov{ false };
        inline std::atomic<int> aimbot_target_priority{ 1 }; // 1 closest to crosshair, 2 lowest health, 3 closest distance
        inline std::atomic<bool> aimbot_wall_check{ false };
        inline std::atomic<bool> aimbot_team_check{ false };
        inline std::atomic<float> aimbot_start_delay{ 0.05f };
        inline std::atomic<float> aimbot_focus_in_seconds{ 0.0f };
        inline std::atomic<bool> aimbot_check_jumping{ false };
        inline std::atomic<bool> aimbot_stop_on_kill{ false };
        inline std::atomic<float> aimbot_rcs_scale{ 0.0f };
        inline std::atomic<bool> aimbot_dynamic_fov{ false };
        inline std::atomic<float> aimbot_overshoot{ 0.0f };
        inline std::atomic<int> aimbot_curve_type{ 0 }; // 0 linear, 1 sine, 2 bezier
        inline std::atomic<bool> aimbot_sticky_aim{ true };

        inline std::atomic<int> silentaim_mode{ 1 }; // 1: new, 2: experimental
        inline std::atomic<bool> silentaim_enabled{ false };
        inline std::atomic<int> silentaim_bind1{ 0x02 };
        inline std::atomic<int> silentaim_bind2{ 0 };
        inline std::atomic<int> silentaim_bind_mode{ 1 }; // 1 always, 2 hold, 3 toggle
        inline std::atomic<int> silentaim_hitpart{ 0 };
        inline std::atomic<int> silentaim_hitpart_blacklist{ 0 };
        inline std::atomic<float> silentaim_hit_chance{ 100.0f };
        inline std::atomic<float> silentaim_fov{ 850.0f };
        inline std::atomic<bool> silentaim_draw_fov{ false };
        inline std::atomic<int> silentaim_target_priority{ 1 }; // 1 closest to crosshair, 2 lowest health, 3 closest distance
        inline std::atomic<bool> silentaim_wall_check{ false };
        inline std::atomic<bool> silentaim_team_check{ true };
        inline std::atomic<float> silentaim_start_delay{ 0.0f };
        inline std::atomic<float> silentaim_focus_in_seconds{ 0.0f };
        inline std::atomic<bool> silentaim_check_jumping{ false };

        inline std::atomic<bool> hitsound_enabled{ false };
        inline std::atomic<int> hitsound_mode{ 1 }; // 1: neverlose, 2: violet
    }

    namespace triggerbot {
        inline std::atomic<bool> triggerbot_enabled{ false };
        inline std::atomic<int> triggerbot_bind1{ 0 };
        inline std::atomic<int> triggerbot_bind2{ 0 };
        inline std::atomic<int> triggerbot_mode{ 1 }; // 1 always, 2 hold, 3 toggle
        inline std::atomic<int> triggerbot_delay{ 0 };
        inline std::atomic<int> triggerbot_delay_variance{ 0 };
        inline std::atomic<float> triggerbot_hitchance{ 100.0f };
        inline std::atomic<int> triggerbot_hitbox_filter{ 0 }; // 0 head, 1 chest, 2 all
        inline std::atomic<bool> triggerbot_burst_mode{ false };
        inline std::atomic<int> triggerbot_burst_count{ 1 };
        inline std::atomic<int> triggerbot_post_shot_delay{ 0 };
        inline std::atomic<bool> triggerbot_wall_check{ false };
        inline std::atomic<bool> triggerbot_team_check{ false };
        inline std::atomic<bool> triggerbot_check_flashed{ false };
        inline std::atomic<bool> triggerbot_check_jumping{ false };
        inline std::atomic<bool> triggerbot_recoil_comp{ false };
        inline std::atomic<float> triggerbot_magnet_fov{ 0.0f };
        inline std::atomic<float> triggerbot_magnet_smooth{ 1.0f };
    }

    namespace world {
        inline std::atomic<bool> fullbright{ false };
        inline color_t ambience_color = { 1.0f, 1.0f, 1.0f, 1.0f };

        inline std::atomic<bool> fov_enabled{ false };
        inline std::atomic<float> fov_value{ 120.0f };
        inline std::atomic<bool> third_person{ false };

        inline std::atomic<bool> skybox_enabled{ false };
        inline std::atomic<bool> skybox_apply{ false };
        inline std::string skybox_bk = "rbxassetid://600886096";
        inline std::string skybox_dn = "rbxassetid://600886096";
        inline std::string skybox_ft = "rbxassetid://600886096";
        inline std::string skybox_lf = "rbxassetid://600886096";
        inline std::string skybox_rt = "rbxassetid://600886096";
        inline std::string skybox_up = "rbxassetid://600886096";

        inline std::atomic<bool> stretched_res_enabled{ false };
        inline std::atomic<float> stretched_res_x{ 1080.0f };
        inline std::atomic<float> stretched_res_y{ 1080.0f };
    }
    namespace weather {
        inline std::atomic<bool> enabled{ false };
        inline std::atomic<int> mode{ 0 }; // 0 - snow 1 -rain
        inline std::atomic<int> particle_count{ 300 };
        inline std::atomic<float> fall_speed{ 8.0f };
        inline std::atomic<float> drift_strength{ 2.0f };
        inline std::atomic<float> spawn_radius{ 80.0f };
        inline std::atomic<float> spawn_height{ 60.0f };
        inline std::atomic<float> size_min{ 4.0f };
        inline std::atomic<float> size_max{ 5.5f };
        inline color_t particle_color = { 1.0f, 1.0f, 1.0f, 0.85f };
    }

    namespace settings {
        inline std::atomic<bool> streamproof{ false };
        inline std::atomic<bool> raycast{ false };
        inline std::atomic<bool> rescan_enabled{ false };
        inline std::atomic<bool> watermark{ true };
        inline std::atomic<int> watermark_style{ 1 }; // 0 default, 1custom
        inline std::atomic<float> watermark_width{ 180.0f };
        inline std::atomic<float> watermark_height{ 26.0f };
        inline std::atomic<float> watermark_rounding{ 6.0f };
        inline color_t watermark_bg_color = { 0.035f, 0.035f, 0.043f, 1.0f }; // zinc-950
        inline color_t watermark_border_color = { 0.153f, 0.153f, 0.165f, 1.0f }; // zinc-800
        inline std::atomic<bool> keybind_list{ false };
        inline std::atomic<bool> performance_graph{ true };
        inline std::atomic<float> player_cache_delay{ 0.0f };
        inline std::atomic<float> aim_thread_delay{ 0.0f };
        inline std::atomic<float> rescan_thread_delay{ 0.0f };
        inline std::atomic<int> perf_mode{ 1 };
        inline std::atomic<int> menu_keybind{ VK_HOME };
        inline std::atomic<bool> fps_limit_enabled{ false };
        inline std::atomic<int> fps_limit{ 240 };
        inline color_t menu_color = { 0.22f, 0.77f, 0.73f, 1.0f };
        inline std::atomic<int> selected_theme{ 0 }; // 0 violet 1 dark 2 sakura
    }

    namespace bytecode {
        inline std::atomic<bool> enabled{ false };
    }

    namespace character {
        inline std::atomic<bool> walkspeed_enabled{ false };
        inline std::atomic<float> walkspeed_value{ 16.0f };
        inline std::atomic<bool> jumppower_enabled{ false };
        inline std::atomic<float> jumppower_value{ 50.0f };
        inline std::atomic<bool> noclip{ false };
        inline std::atomic<bool> fly{ false };
        inline std::atomic<float> fly_speed{ 50.0f };
        inline std::atomic<bool> inf_jump{ false };
    }

    namespace rivals {
        inline std::atomic<bool> auto_parry{ false };
        inline std::atomic<bool> auto_block{ false };
        inline std::atomic<bool> auto_swing{ false };
        inline std::atomic<bool> kill_aura{ false };
        inline std::atomic<bool> auto_farm{ false };
    }
}
