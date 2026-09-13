#pragma once
#include <Windows.h>

namespace ui::web_menu {
    auto initialize(HWND overlay_hwnd) -> bool;
    auto shutdown() -> void;
    auto is_open() -> bool;
    auto is_ready() -> bool;
    auto process_input(HWND overlay_hwnd, HWND game_hwnd, bool allowed) -> void;
}
