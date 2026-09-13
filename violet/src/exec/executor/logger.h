#pragma once
#include <windows.h>
#include <iostream>
#include <string>
#include <format>

#define RESET   "\033[0m"
#define WHITE   "\033[97m"
#define CYAN    "\033[36m"
#define RED     "\033[31m"
#define ORANGE  "\033[38;5;208m"
#define GRAY    "\033[90m"

namespace logger {

    inline auto set_title(std::wstring_view title) -> void {
        SetConsoleTitleW(title.data());
    }

    template <typename... Args>
    auto log(std::string_view color, std::string_view label, std::string_view fmt, Args&&... args) -> void {
        static bool initialized = []() {
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hOut == INVALID_HANDLE_VALUE) return false;
            DWORD dwMode = 0;
            if (!GetConsoleMode(hOut, &dwMode)) return false;
            return (bool)SetConsoleMode(hOut, dwMode | 0x0004);
            }();

        std::string message = std::vformat(fmt, std::make_format_args(args...));
        std::cout << color << label << message << RESET << std::endl;
    }

    template <typename... Args>
    auto info(std::string_view fmt, Args&&... args) -> void {
        log(CYAN, "", fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    auto success(std::string_view fmt, Args&&... args) -> void {
        log(WHITE, "", fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    auto warn(std::string_view fmt, Args&&... args) -> void {
        log(ORANGE, "", fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    auto error(std::string_view fmt, Args&&... args) -> void {
        log(RED, "", fmt, std::forward<Args>(args)...);
    }
}
