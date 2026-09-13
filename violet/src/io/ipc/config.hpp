#pragma once

#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace config {

struct color_rgba {
    float r;
    float g;
    float b;
    float a;
};

enum class value_type {
    boolean,
    integer,
    number,
    string,
    color
};

using value = std::variant<bool, int, float, std::string, color_rgba>;

namespace json {
auto parse_object(const std::string& text, std::map<std::string, std::string>& out_values) -> bool;
auto parse_string(const std::string& text, std::string& out_value) -> bool;
auto parse_bool(const std::string& text, bool& out_value) -> bool;
auto parse_number(const std::string& text, double& out_value) -> bool;
auto escape(std::string_view text) -> std::string;
} // namespace json

class manager {
public:
    static auto instance() -> manager&;

    auto register_bool(const std::string& key, bool default_value,
        std::function<void(bool)> setter = {},
        std::function<bool()> getter = {}) -> void;
    auto register_int(const std::string& key, int default_value,
        std::function<void(int)> setter = {},
        std::function<int()> getter = {}) -> void;
    auto register_float(const std::string& key, float default_value,
        std::function<void(float)> setter = {},
        std::function<float()> getter = {}) -> void;
    auto register_string(const std::string& key, std::string default_value,
        std::function<void(const std::string&)> setter = {},
        std::function<std::string()> getter = {}) -> void;
    auto register_color(const std::string& key, color_rgba default_value,
        std::function<void(const color_rgba&)> setter = {},
        std::function<color_rgba()> getter = {}) -> void;

    auto set_bool(const std::string& key, bool new_value) -> bool;
    auto set_int(const std::string& key, int new_value) -> bool;
    auto set_float(const std::string& key, float new_value) -> bool;
    auto set_string(const std::string& key, const std::string& new_value) -> bool;
    auto set_color(const std::string& key, color_rgba new_value) -> bool;
    auto set_from_json(const std::string& key, const std::string& json_value) -> bool;

    auto get(const std::string& key) const -> std::optional<value>;
    auto serialize() const -> std::string;
    auto deserialize(const std::string& text) -> bool;

private:
    struct setting {
        value_type type;
        value current_value;
        std::function<void(const value&)> writer;
        std::function<value()> reader;
    };

    manager() = default;

    auto insert_or_assign(const std::string& key, setting entry) -> void;
    auto set_value(const std::string& key, const value& new_value) -> bool;

    mutable std::mutex mutex_;
    std::map<std::string, setting> settings_;
};

auto register_default_settings() -> void;

} // namespace config
