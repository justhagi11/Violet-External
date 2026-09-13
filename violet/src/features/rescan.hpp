#pragma once
#include "../game/sdk.hpp"
#include "../io/memory/memory.hpp"
#include <thread>
#include <chrono>
#include "../render/overlay.hpp"

namespace rescan {

    struct module_states {
        bool visuals_enabled = false;
        bool aimbot_enabled = false;
        bool silentaim_enabled = false;
        bool triggerbot_enabled = false;
        bool fullbright = false;
        bool fov_enabled = false;
        bool weather_enabled = false;
        bool skybox_enabled = false;
    };

    inline std::string last_job_id = "";
    inline bool was_valid = false;
    inline module_states saved_state;
    
    // [rescan::check]
    inline void check() {
        auto start_time = std::chrono::steady_clock::now();
        if (!globals::settings::rescan_enabled.load()) {
            globals::settings::rescan_thread_delay.store(0.0f);
            return;
        }

        std::string current_job_id = game.get_jobid();
        std::string current_game_id = game.get_gameid();

        bool is_valid_game = (current_job_id != "DM Not Found.") && (current_game_id != "0");
        bool job_id_changed = (is_valid_game && !last_job_id.empty() && current_job_id != last_job_id);

        if (!is_valid_game || job_id_changed) {
            if (was_valid) {
                render::add_notification(job_id_changed ? HIDE_STR("Server changed, rescanning...") : HIDE_STR("Game lost, waiting..."), 1.0f);

                saved_state.visuals_enabled = globals::visuals::enabled.load();
                saved_state.aimbot_enabled = globals::aim::aimbot_enabled.load();
                saved_state.silentaim_enabled = globals::aim::silentaim_enabled.load();
                saved_state.triggerbot_enabled = globals::triggerbot::triggerbot_enabled.load();
                saved_state.fullbright = globals::world::fullbright.load();
                saved_state.fov_enabled = globals::world::fov_enabled.load();
                saved_state.weather_enabled = globals::weather::enabled.load();
                saved_state.skybox_enabled = globals::world::skybox_enabled.load();

                globals::visuals::enabled.store(false);
                globals::aim::aimbot_enabled.store(false);
                globals::aim::silentaim_enabled.store(false);
                globals::triggerbot::triggerbot_enabled.store(false);
                globals::world::fullbright.store(false);
                globals::world::fov_enabled.store(false);
                globals::weather::enabled.store(false);
                globals::world::skybox_enabled.store(false);

                was_valid = false;
            }
            
            if (job_id_changed) {
                last_job_id = "";
            }
        } 
        
        if (is_valid_game && !was_valid) {
            render::add_notification(HIDE_STR("Rescanned successfully!"), 1.0f);

            globals::visuals::enabled.store(saved_state.visuals_enabled);
            globals::aim::aimbot_enabled.store(saved_state.aimbot_enabled);
            globals::aim::silentaim_enabled.store(saved_state.silentaim_enabled);
            globals::triggerbot::triggerbot_enabled.store(saved_state.triggerbot_enabled);
            globals::world::fullbright.store(saved_state.fullbright);
            globals::world::fov_enabled.store(saved_state.fov_enabled);
            globals::weather::enabled.store(saved_state.weather_enabled);
            globals::world::skybox_enabled.store(saved_state.skybox_enabled);

            was_valid = true;
            last_job_id = current_job_id;
        }

        if (is_valid_game && globals::world::skybox_enabled.load() && globals::world::skybox_apply.load()) {
            //game.update_skybox();
            globals::world::skybox_apply.store(false);
        }

        auto end_time = std::chrono::steady_clock::now();
        float elapsed = std::chrono::duration<float, std::milli>(end_time - start_time).count();
        globals::settings::rescan_thread_delay.store(elapsed);
    }
}
