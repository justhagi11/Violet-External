// Library Includes. ;
    #include <Windows.h>
    #include <dwmapi.h>
    #include <format>
    #include <iostream>
    #include <thread>
    #include <ctime>
    #include <cstdio>
    #include <cstring>

// Internal Includes. ;
    #include "../core/logger.hpp"
    #include "../security/obfuscator.hpp"
    #include "../core/globals.hpp"
    #include "../core/detail.hpp"
    #include "../deps/imgui/imgui.h"
    #include "../game/sdk.hpp"
    #include "../io/ipc/config.hpp"
    #include "../io/ipc/server.hpp"
    #include "../game/entity_cache.hpp"
    #include "../threads/manager.hpp"
    #include "../features/rescan.hpp"
    #include "../features/aiming.hpp"
    #include "../render/overlay.hpp"
    #include "../features/bytecode.hpp"


// [entry::main]
auto main() -> int {
#ifdef _DEBUG
    {
        char month_str[16] = { 0 }; 
        int day = 0, year = 0;
        if (sscanf_s(__DATE__, "%s %d %d", month_str, (unsigned int)sizeof(month_str), &day, &year) == 3) {
            const char* months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
            int month = -1;
            for (int i = 0; i < 12; i++) {
                if (month_str[0] == months[i][0] && month_str[1] == months[i][1] && month_str[2] == months[i][2]) { 
                    month = i; 
                    break; 
                }
            }
            if (month != -1) {
                struct tm build_time = { 0 };
                build_time.tm_mon = month;
                build_time.tm_mday = day;
                build_time.tm_year = year - 1900;
                time_t build_t = mktime(&build_time);
                time_t now_t = time(nullptr);
                if (difftime(now_t, build_t) > 3 * 24 * 60 * 60) {
                    MessageBoxA(NULL, "This developer build has expired.", "Violet", MB_OK | MB_ICONERROR);
                    return 0;
                }
            }
        }
    }
#endif

    ipc::tcp_server ipc_server(HIDE_STR("127.0.0.1").c_str(), 5005);
    ipc_server.start();
    config::register_default_settings();

    // game.set_ambient(0, 0, 0);
    task::manager::add(HIDE_STR("player_cache"), [](std::stop_token stop) {
        while (!stop.stop_requested() && globals::running) {
            cache::update();
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
        });

    task::manager::add(HIDE_STR("rescan"), [](std::stop_token stop) {
        while (!stop.stop_requested() && globals::running) {
            rescan::check();
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        }
        });

    task::manager::add(HIDE_STR("aiming"), [](std::stop_token stop) {
        while (!stop.stop_requested() && globals::running) {
            aiming::tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        });

    task::manager::add(HIDE_STR("overlay"), [](std::stop_token stop) {
        if (overlay::initialize()) {

            while (!stop.stop_requested() && globals::running) {
                overlay::render_loop();
            }

            overlay::shutdown();
        }
        else {
            //logger::error(HIDE_STR("dx11 error"));
        }
        });


    task::manager::add(HIDE_STR("process_monitor"), [](std::stop_token stop) {
        while (!stop.stop_requested() && globals::running) {
            auto h = mem.process_handle;
            if (h) {
                DWORD exit_code = 0;
                if (GetExitCodeProcess(h.get(), &exit_code) && exit_code != STILL_ACTIVE) {
                    logger::warn("process_monitor: Roblox process ended.");
                    mem.process_handle.reset();
                    mem.base_address = 0;
                    logger::info("entry.cpp: LOG[Exit]");
                    std::exit(0);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }
        });

    bytecode::hook();

    while (globals::running) {
        if (!mem.process_handle) {
            if (mem.attach(HIDE_WSTR(L"RobloxPlayerBeta.exe").c_str())) {
                logger::set_title(detail::random_wstring(16));
                logger::success(HIDE_STR("base: 0x{:05X}"), (mem.base_address & 0xFFFFF));
                ipc_server.broadcast(HIDE_STR("{\"type\":\"status\",\"attached\":true}"));
            }
        } else {
            ipc_server.broadcast(HIDE_STR("{\"type\":\"status\",\"attached\":true}"));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    ipc_server.stop();
    task::manager::stop_all();
    return 0;
}
