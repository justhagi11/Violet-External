#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <chrono>
#include <random>
#include <cmath>
#include <algorithm>
#include <unordered_map>

namespace detail {
    // mode 1 = always 2 = hold 3 = toggle
    // [detail::is_keybind_active]
    inline bool is_keybind_active(int mode, int key1, int key2 = 0) {
        if (mode == 1) return true;

        bool down1 = key1 != 0 && (GetAsyncKeyState(key1) & 0x8000);
        bool down2 = key2 != 0 && (GetAsyncKeyState(key2) & 0x8000);
        bool is_pressed = down1 || down2;

        if (mode == 2) return is_pressed;

        if (mode == 3) {
            static std::unordered_map<int, bool> toggle_states;
            static std::unordered_map<int, bool> prev_states;

            bool& t_state = toggle_states[key1];
            bool& p_state = prev_states[key1];
            if (is_pressed && !p_state) {
                t_state = !t_state;
            }
            p_state = is_pressed;

            return t_state;
        }

        return false;
    }
    // [detail::now_sec]
    inline auto now_sec() -> double {
        static const auto epoch = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - epoch).count();
    }

    // [detail::now_ms]
    inline auto now_ms() -> int64_t {
        static const auto epoch = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - epoch).count();
    }

    // [detail::rng_float]
    inline auto rng_float(float lo, float hi) -> float {
        static thread_local std::mt19937 gen{ std::random_device{}() };
        return std::uniform_real_distribution<float>(lo, hi)(gen);
    }

    // [detail::lerp]
    inline float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }

    // [detail::ease_in_out_sine]
    inline float ease_in_out_sine(float t) {
        return -0.5f * (std::cos(3.14159265358979323846f * t) - 1.0f);
    }

    // [detail::ease_out_cubic]
    inline float ease_out_cubic(float t) {
        float f = t - 1.0f;
        return f * f * f + 1.0f;
    }

    // [detail::ease_in_out_cubic]
    inline float ease_in_out_cubic(float t) {
        return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
    }

    // [detail::ema]
    inline float ema(float current, float target, float alpha) {
        return (current * (1.0f - alpha)) + (target * alpha);
    }

    // [detail::spring_smooth]
    inline float spring_smooth(float current, float target, float& velocity, float stiffness, float damping, float dt) {
        float force = (target - current) * stiffness;
        velocity += force * dt;
        velocity *= damping; // apply friction
        return current + velocity * dt;
    }

    // [detail::subtle_noise]
    inline float subtle_noise(double time, float speed, float amplitude) {
        return static_cast<float>(std::sin(time * speed) * std::cos(time * speed * 0.73) * amplitude);
    }

    // [detail::random_string]
    inline auto random_string(size_t length) -> std::string {
        static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        static thread_local std::mt19937 gen{ std::random_device{}() };
        std::uniform_int_distribution<int> dist(0, sizeof(charset) - 2);
        std::string res;
        for (size_t i = 0; i < length; i++) res += charset[dist(gen)];
        return res;
    }

    // [detail::random_wstring]
    inline auto random_wstring(size_t length) -> std::wstring {
        static const wchar_t charset[] = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        static thread_local std::mt19937 gen{ std::random_device{}() };
        std::uniform_int_distribution<int> dist(0, (int)(sizeof(charset) / sizeof(wchar_t)) - 2);
        std::wstring res;
        for (size_t i = 0; i < length; i++) res += charset[dist(gen)];
        return res;
    }
}
