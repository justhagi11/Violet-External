#pragma once
#include <windows.h>
#include <iostream>
#include <string>
#include <format>
#include "../security/obfuscator.hpp"

#define RESET   HIDE_STR("\033[0m")
#define WHITE   HIDE_STR("\033[97m")
#define CYAN    HIDE_STR("\033[36m")
#define RED     HIDE_STR("\033[31m")
#define ORANGE  HIDE_STR("\033[38;5;208m")
#define GRAY    HIDE_STR("\033[90m")

namespace logger {

    // [logger::set_title]
    inline auto set_title(std::wstring_view title) -> void {
        SetConsoleTitleW(title.data());
    }

    template <typename... Args>

    // [logger::log]
    auto log(std::string_view color, std::string_view label, std::string_view fmt, Args&&... args) -> void {
        static bool initialized = []() {
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD dwMode = 0;
            GetConsoleMode(hOut, &dwMode);
            return SetConsoleMode(hOut, dwMode | 0x0004);
            }();

        std::string message = std::vformat(fmt, std::make_format_args(args...));
        std::cout << color << label << message << RESET << std::endl;
    }

    template <typename... Args>
    // [logger::info]
    auto info(std::string_view fmt, Args&&... args) -> void {
        log(CYAN, HIDE_STR(""), fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    // [logger::success]
    auto success(std::string_view fmt, Args&&... args) -> void {
        log(WHITE, HIDE_STR(""), fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    // [logger::warn]
    auto warn(std::string_view fmt, Args&&... args) -> void {
        log(ORANGE, HIDE_STR(""), fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    // [logger::error]
    auto error(std::string_view fmt, Args&&... args) -> void {
        log(RED, HIDE_STR(""), fmt, std::forward<Args>(args)...);
    }
}
