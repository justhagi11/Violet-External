#include "config.hpp"
#include "../../core/globals.hpp"
#include "../../security/obfuscator.hpp"

namespace config {
namespace {

auto trim_view(std::string_view text) -> std::string_view {
    std::size_t start = 0;
    std::size_t end = text.size();

    while (start < end && std::isspace(static_cast<unsigned char>(text[start])) != 0) {
        ++start;
    }

    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }

    return text.substr(start, end - start);
}

auto skip_ws(std::string_view text, std::size_t& index) -> void {
    while (index < text.size() && std::isspace(static_cast<unsigned char>(text[index])) != 0) {
        ++index;
    }
}

auto parse_hex_digit(char ch, int& out_value) -> bool {
    if (ch >= '0' && ch <= '9') {
        out_value = ch - '0';
        return true;
    }

    if (ch >= 'a' && ch <= 'f') {
        out_value = 10 + (ch - 'a');
        return true;
    }

    if (ch >= 'A' && ch <= 'F') {
        out_value = 10 + (ch - 'A');
        return true;
    }

    return false;
}

auto parse_string_token(std::string_view text, std::size_t& index, std::string& out_value) -> bool {
    skip_ws(text, index);

    if (index >= text.size() || text[index] != '"') {
        return false;
    }

    ++index;
    out_value.clear();

    while (index < text.size()) {
        const char current = text[index++];

        if (current == '"') {
            return true;
        }

        if (current != '\\') {
            out_value.push_back(current);
            continue;
        }

        if (index >= text.size()) {
            return false;
        }

        const char escaped = text[index++];
        switch (escaped) {
        case '"': out_value.push_back('"'); break;
        case '\\': out_value.push_back('\\'); break;
        case '/': out_value.push_back('/'); break;
        case 'b': out_value.push_back('\b'); break;
        case 'f': out_value.push_back('\f'); break;
        case 'n': out_value.push_back('\n'); break;
        case 'r': out_value.push_back('\r'); break;
        case 't': out_value.push_back('\t'); break;
        case 'u': {
            if (index + 4 > text.size()) {
                return false;
            }

            unsigned int codepoint = 0;
            for (int offset = 0; offset < 4; ++offset) {
                int digit = 0;
                if (!parse_hex_digit(text[index + static_cast<std::size_t>(offset)], digit)) {
                    return false;
                }

                codepoint = (codepoint << 4) | static_cast<unsigned int>(digit);
            }

            index += 4;
            if (codepoint <= 0x7F) {
                out_value.push_back(static_cast<char>(codepoint));
            }
            else {
                out_value.push_back('?');
            }
            break;
        }
        default:
            return false;
        }
    }

    return false;
}

auto capture_json_value(std::string_view text, std::size_t& index, std::string& out_value) -> bool {
    skip_ws(text, index);

    if (index >= text.size()) {
        return false;
    }

    const std::size_t start = index;

    if (text[index] == '"') {
        std::string decoded;
        if (!parse_string_token(text, index, decoded)) {
            return false;
        }

        out_value = std::string(text.substr(start, index - start));
        return true;
    }

    if (text[index] == '{' || text[index] == '[') {
        int depth = 0;
        bool in_string = false;
        bool escaped = false;

        for (; index < text.size(); ++index) {
            const char current = text[index];

            if (in_string) {
                if (escaped) {
                    escaped = false;
                }
                else if (current == '\\') {
                    escaped = true;
                }
                else if (current == '"') {
                    in_string = false;
                }

                continue;
            }

            if (current == '"') {
                in_string = true;
                continue;
            }

            if (current == '{' || current == '[') {
                ++depth;
            }
            else if (current == '}' || current == ']') {
                --depth;

                if (depth == 0) {
                    ++index;
                    out_value = std::string(trim_view(text.substr(start, index - start)));
                    return true;
                }

                if (depth < 0) {
                    return false;
                }
            }
        }

        return false;
    }

    while (index < text.size() && text[index] != ',' && text[index] != '}' && text[index] != ']') {
        ++index;
    }

    const auto raw_value = trim_view(text.substr(start, index - start));
    if (raw_value.empty()) {
        return false;
    }

    out_value = std::string(raw_value);
    return true;
}

auto matches_type(value_type type, const value& candidate) -> bool {
    switch (type) {
    case value_type::boolean:
        return std::holds_alternative<bool>(candidate);
    case value_type::integer:
        return std::holds_alternative<int>(candidate);
    case value_type::number:
        return std::holds_alternative<float>(candidate);
    case value_type::string:
        return std::holds_alternative<std::string>(candidate);
    case value_type::color:
        return std::holds_alternative<color_rgba>(candidate);
    }

    return false;
}

auto value_to_json(const value& current_value) -> std::string {
    if (const auto* bool_value = std::get_if<bool>(&current_value)) {
        return *bool_value ? HIDE_STR("true") : HIDE_STR("false");
    }

    if (const auto* int_value = std::get_if<int>(&current_value)) {
        return std::to_string(*int_value);
    }

    if (const auto* float_value = std::get_if<float>(&current_value)) {
        return std::to_string(*float_value);
    }

    if (const auto* string_value = std::get_if<std::string>(&current_value)) {
        return "\"" + json::escape(*string_value) + "\"";
    }

    const auto& color = std::get<color_rgba>(current_value);
    return HIDE_STR("{\"r\":") + std::to_string(color.r)
        + HIDE_STR(",\"g\":") + std::to_string(color.g)
        + HIDE_STR(",\"b\":") + std::to_string(color.b)
        + HIDE_STR(",\"a\":") + std::to_string(color.a) + HIDE_STR("}");
}

auto parse_color_object(const std::string& text, color_rgba& out_value) -> bool {
    std::map<std::string, std::string> color_fields;
    if (!json::parse_object(text, color_fields)) {
        return false;
    }

    const auto r_it = color_fields.find(HIDE_STR("r"));
    const auto g_it = color_fields.find(HIDE_STR("g"));
    const auto b_it = color_fields.find(HIDE_STR("b"));
    const auto a_it = color_fields.find(HIDE_STR("a"));

    if (r_it == color_fields.end() || g_it == color_fields.end()
        || b_it == color_fields.end() || a_it == color_fields.end()) {
        return false;
    }

    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
    double a = 0.0;

    if (!json::parse_number(r_it->second, r)
        || !json::parse_number(g_it->second, g)
        || !json::parse_number(b_it->second, b)
        || !json::parse_number(a_it->second, a)) {
        return false;
    }

    out_value = {
        static_cast<float>(r),
        static_cast<float>(g),
        static_cast<float>(b),
        static_cast<float>(a)
    };

    return true;
}

} // namespace

namespace json {

auto parse_object(const std::string& text, std::map<std::string, std::string>& out_values) -> bool {
    out_values.clear();

    const auto trimmed = trim_view(text);
    if (trimmed.size() < 2 || trimmed.front() != '{' || trimmed.back() != '}') {
        return false;
    }

    std::size_t index = 1;
    while (index < trimmed.size()) {
        skip_ws(trimmed, index);

        if (index >= trimmed.size() - 1) {
            break;
        }

        std::string key;
        if (!parse_string_token(trimmed, index, key)) {
            return false;
        }

        skip_ws(trimmed, index);
        if (index >= trimmed.size() || trimmed[index] != ':') {
            return false;
        }

        ++index;

        std::string raw_value;
        if (!capture_json_value(trimmed, index, raw_value)) {
            return false;
        }

        out_values[key] = raw_value;

        skip_ws(trimmed, index);
        if (index >= trimmed.size() - 1) {
            break;
        }

        if (trimmed[index] != ',') {
            return false;
        }

        ++index;
    }

    skip_ws(trimmed, index);
    return index == trimmed.size() - 1;
}

auto parse_string(const std::string& text, std::string& out_value) -> bool {
    const auto trimmed = trim_view(text);
    std::size_t index = 0;

    if (!parse_string_token(trimmed, index, out_value)) {
        return false;
    }

    skip_ws(trimmed, index);
    return index == trimmed.size();
}

auto parse_bool(const std::string& text, bool& out_value) -> bool {
    const auto trimmed = trim_view(text);

    if (trimmed == HIDE_STR("true")) {
        out_value = true;
        return true;
    }

    if (trimmed == HIDE_STR("false")) {
        out_value = false;
        return true;
    }

    return false;
}

auto parse_number(const std::string& text, double& out_value) -> bool {
    const auto trimmed = trim_view(text);
    if (trimmed.empty()) {
        return false;
    }

    std::string owned(trimmed);
    char* end_ptr = nullptr;

    errno = 0;
    const double parsed = std::strtod(owned.c_str(), &end_ptr);

    if (end_ptr == owned.c_str() || *end_ptr != '\0' || errno == ERANGE) {
        return false;
    }

    out_value = parsed;
    return true;
}

auto escape(std::string_view text) -> std::string {
    static constexpr char hex[] = "0123456789ABCDEF";

    std::string result;
    result.reserve(text.size() + 8);

    for (const unsigned char current : text) {
        switch (current) {
        case '"': result += "\\\""; break;
        case '\\': result += "\\\\"; break;
        case '\b': result += "\\b"; break;
        case '\f': result += "\\f"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default:
            if (current < 0x20) {
                result += "\\u00";
                result.push_back(hex[(current >> 4) & 0x0F]);
                result.push_back(hex[current & 0x0F]);
            }
            else {
                result.push_back(static_cast<char>(current));
            }
            break;
        }
    }

    return result;
}

} // namespace json

auto manager::instance() -> manager& {
    static manager singleton;
    return singleton;
}

auto manager::insert_or_assign(const std::string& key, setting entry) -> void {
    std::function<void(const value&)> writer;
    value assigned_value = entry.current_value;

    {
        std::scoped_lock lock(mutex_);
        settings_[key] = std::move(entry);
        writer = settings_[key].writer;
        assigned_value = settings_[key].current_value;
    }

    if (writer) {
        writer(assigned_value);
    }
}

auto manager::set_value(const std::string& key, const value& new_value) -> bool {
    std::function<void(const value&)> writer;
    value assigned_value;

    {
        std::scoped_lock lock(mutex_);

        const auto it = settings_.find(key);
        if (it == settings_.end()) {
            return false;
        }

        if (!matches_type(it->second.type, new_value)) {
            return false;
        }

        it->second.current_value = new_value;
        writer = it->second.writer;
        assigned_value = it->second.current_value;
    }

    if (writer) {
        writer(assigned_value);
    }

    return true;
}

auto manager::register_bool(const std::string& key, bool default_value,
    std::function<void(bool)> setter,
    std::function<bool()> getter) -> void {
    setting entry{};
    entry.type = value_type::boolean;
    entry.current_value = default_value;

    if (setter) {
        entry.writer = [setter = std::move(setter)](const value& incoming) {
            setter(std::get<bool>(incoming));
            };
    }

    if (getter) {
        entry.reader = [getter = std::move(getter)]() -> value {
            return getter();
            };
    }

    insert_or_assign(key, std::move(entry));
}

auto manager::register_int(const std::string& key, int default_value,
    std::function<void(int)> setter,
    std::function<int()> getter) -> void {
    setting entry{};
    entry.type = value_type::integer;
    entry.current_value = default_value;

    if (setter) {
        entry.writer = [setter = std::move(setter)](const value& incoming) {
            setter(std::get<int>(incoming));
            };
    }

    if (getter) {
        entry.reader = [getter = std::move(getter)]() -> value {
            return getter();
            };
    }

    insert_or_assign(key, std::move(entry));
}

auto manager::register_float(const std::string& key, float default_value,
    std::function<void(float)> setter,
    std::function<float()> getter) -> void {
    setting entry{};
    entry.type = value_type::number;
    entry.current_value = default_value;

    if (setter) {
        entry.writer = [setter = std::move(setter)](const value& incoming) {
            setter(std::get<float>(incoming));
            };
    }

    if (getter) {
        entry.reader = [getter = std::move(getter)]() -> value {
            return getter();
            };
    }

    insert_or_assign(key, std::move(entry));
}

auto manager::register_string(const std::string& key, std::string default_value,
    std::function<void(const std::string&)> setter,
    std::function<std::string()> getter) -> void {
    setting entry{};
    entry.type = value_type::string;
    entry.current_value = std::move(default_value);

    if (setter) {
        entry.writer = [setter = std::move(setter)](const value& incoming) {
            setter(std::get<std::string>(incoming));
            };
    }

    if (getter) {
        entry.reader = [getter = std::move(getter)]() -> value {
            return getter();
            };
    }

    insert_or_assign(key, std::move(entry));
}

auto manager::register_color(const std::string& key, color_rgba default_value,
    std::function<void(const color_rgba&)> setter,
    std::function<color_rgba()> getter) -> void {
    setting entry{};
    entry.type = value_type::color;
    entry.current_value = default_value;

    if (setter) {
        entry.writer = [setter = std::move(setter)](const value& incoming) {
            setter(std::get<color_rgba>(incoming));
            };
    }

    if (getter) {
        entry.reader = [getter = std::move(getter)]() -> value {
            return getter();
            };
    }

    insert_or_assign(key, std::move(entry));
}

auto manager::set_bool(const std::string& key, bool new_value) -> bool {
    return set_value(key, new_value);
}

auto manager::set_int(const std::string& key, int new_value) -> bool {
    return set_value(key, new_value);
}

auto manager::set_float(const std::string& key, float new_value) -> bool {
    return set_value(key, new_value);
}

auto manager::set_string(const std::string& key, const std::string& new_value) -> bool {
    return set_value(key, new_value);
}

auto manager::set_color(const std::string& key, color_rgba new_value) -> bool {
    return set_value(key, new_value);
}

auto manager::set_from_json(const std::string& key, const std::string& json_value) -> bool {
    value_type expected_type{};

    {
        std::scoped_lock lock(mutex_);
        const auto it = settings_.find(key);
        if (it == settings_.end()) {
            return false;
        }

        expected_type = it->second.type;
    }

    switch (expected_type) {
    case value_type::boolean: {
        bool parsed = false;
        if (!json::parse_bool(json_value, parsed)) {
            return false;
        }

        return set_bool(key, parsed);
    }
    case value_type::integer: {
        double parsed_number = 0.0;
        if (!json::parse_number(json_value, parsed_number)) {
            return false;
        }

        double integral_part = 0.0;
        if (std::modf(parsed_number, &integral_part) != 0.0) {
            return false;
        }

        if (integral_part < static_cast<double>((std::numeric_limits<int>::min)())
            || integral_part > static_cast<double>((std::numeric_limits<int>::max)())) {
            return false;
        }

        return set_int(key, static_cast<int>(integral_part));
    }
    case value_type::number: {
        double parsed_number = 0.0;
        if (!json::parse_number(json_value, parsed_number)) {
            return false;
        }

        return set_float(key, static_cast<float>(parsed_number));
    }
    case value_type::string: {
        std::string parsed_string;
        if (!json::parse_string(json_value, parsed_string)) {
            return false;
        }

        return set_string(key, parsed_string);
    }
    case value_type::color: {
        color_rgba parsed_color{};
        if (!parse_color_object(json_value, parsed_color)) {
            return false;
        }

        return set_color(key, parsed_color);
    }
    }

    return false;
}

auto manager::get(const std::string& key) const -> std::optional<value> {
    setting selected{};

    {
        std::scoped_lock lock(mutex_);
        const auto it = settings_.find(key);
        if (it == settings_.end()) {
            return std::nullopt;
        }

        selected = it->second;
    }

    if (selected.reader) {
        const value latest = selected.reader();
        if (matches_type(selected.type, latest)) {
            return latest;
        }
    }

    return selected.current_value;
}

auto manager::serialize() const -> std::string {
    std::map<std::string, setting> snapshot;

    {
        std::scoped_lock lock(mutex_);
        snapshot = settings_;
    }

    std::string result = "{";
    bool first = true;

    for (const auto& [key, entry] : snapshot) {
        value latest = entry.current_value;
        if (entry.reader) {
            const value candidate = entry.reader();
            if (matches_type(entry.type, candidate)) {
                latest = candidate;
            }
        }

        if (!first) {
            result += ",";
        }

        result += "\"" + json::escape(key) + "\":" + value_to_json(latest);
        first = false;
    }

    result += "}";
    return result;
}

auto manager::deserialize(const std::string& text) -> bool {
    std::map<std::string, std::string> input;
    if (!json::parse_object(text, input)) {
        return false;
    }

    bool all_applied = true;
    for (const auto& [key, raw_value] : input) {
        if (!set_from_json(key, raw_value)) {
            all_applied = false;
        }
    }

    return all_applied;
}

auto register_default_settings() -> void {
    auto& cfg = manager::instance();

    const auto register_bool_atomic = [&cfg](const std::string& key, std::atomic<bool>& setting) {
        cfg.register_bool(key, setting.load(),
            [&setting](bool new_value) {
                setting.store(new_value);
            },
                [&setting]() {
                    return setting.load();
                });
    };

    const auto register_int_atomic = [&cfg](const std::string& key, std::atomic<int>& setting) {
        cfg.register_int(key, setting.load(),
            [&setting](int new_value) {
                setting.store(new_value);
            },
                [&setting]() {
                    return setting.load();
                });
    };

    const auto register_float_atomic = [&cfg](const std::string& key, std::atomic<float>& setting) {
        cfg.register_float(key, setting.load(),
            [&setting](float new_value) {
                setting.store(new_value);
            },
                [&setting]() {
                    return setting.load();
                });
    };

    const auto register_float = [&cfg](const std::string& key, float& setting) {
        cfg.register_float(key, setting,
            [&setting](float new_value) {
                setting = new_value;
            },
                [&setting]() {
                    return setting;
                });
    };

    const auto register_color_setting = [&cfg](const std::string& key, color_t& setting) {
        cfg.register_color(key,
            { setting.r, setting.g, setting.b, setting.a },
            [&setting](const color_rgba& new_value) {
                setting.r = new_value.r;
                setting.g = new_value.g;
                setting.b = new_value.b;
                setting.a = new_value.a;
            },
                [&setting]() {
                    return color_rgba{ setting.r, setting.g, setting.b, setting.a };
                });
    };

    const auto register_bool = [&cfg](const std::string& key, bool& setting) {
        cfg.register_bool(key, setting,
            [&setting](bool new_value) {
                setting = new_value;
            },
                [&setting]() {
                    return setting;
                });
    };

    const auto register_string = [&cfg](const std::string& key, std::string& setting) {
        cfg.register_string(key, setting,
            [&setting](const std::string& new_value) {
                setting = new_value;
            },
                [&setting]() {
                    return setting;
                });
    };

    const auto register_int = [&cfg](const std::string& key, int& setting) {
        cfg.register_int(key, setting,
            [&setting](int new_value) {
                setting = new_value;
            },
                [&setting]() {
                    return setting;
                });
    };

    register_bool_atomic(HIDE_STR("globals.running"), globals::running);
    register_bool_atomic(HIDE_STR("globals.menu_open"), globals::menu_open);
    register_bool_atomic(HIDE_STR("globals.toolbar_open"), globals::toolbar_open);
    register_bool_atomic(HIDE_STR("globals.logged_in"), globals::logged_in);
    register_bool_atomic(HIDE_STR("globals.devbuild"), globals::devbuild);

    register_int_atomic(HIDE_STR("aimbot.mode"), globals::aim::aimbot_mode);
    register_bool_atomic(HIDE_STR("aimbot.enabled"), globals::aim::aimbot_enabled);
    register_int_atomic(HIDE_STR("aimbot.bind1"), globals::aim::aimbot_bind1);
    register_int_atomic(HIDE_STR("aimbot.bind2"), globals::aim::aimbot_bind2);
    register_int_atomic(HIDE_STR("aimbot.bind_mode"), globals::aim::aimbot_bind_mode);
    register_float_atomic(HIDE_STR("aimbot.smoothing_x"), globals::aim::aimbot_smoothing_x);
    register_float_atomic(HIDE_STR("aimbot.smoothing_y"), globals::aim::aimbot_smoothing_y);
    register_float_atomic(HIDE_STR("aimbot.smoothing_xy"), globals::aim::aimbot_smoothing_xy);
    register_float_atomic(HIDE_STR("aimbot.smoothing_xz"), globals::aim::aimbot_smoothing_xz);
    register_bool_atomic(HIDE_STR("aimbot.humanizer"), globals::aim::aimbot_humanizer);
    register_int_atomic(HIDE_STR("aimbot.prefered_hitpart"), globals::aim::aimbot_prefered_hitpart);
    register_int_atomic(HIDE_STR("aimbot.hitpart_blacklist"), globals::aim::aimbot_hitpart_blacklist);
    register_float_atomic(HIDE_STR("aimbot.sensitivity"), globals::aim::aimbot_sensitivity);
    register_float_atomic(HIDE_STR("aimbot.fov"), globals::aim::aimbot_fov);
    register_bool_atomic(HIDE_STR("aimbot.draw_fov"), globals::aim::aimbot_draw_fov);
    register_int_atomic(HIDE_STR("aimbot.target_priority"), globals::aim::aimbot_target_priority);
    register_bool_atomic(HIDE_STR("aimbot.wall_check"), globals::aim::aimbot_wall_check);
    register_bool_atomic(HIDE_STR("aimbot.team_check"), globals::aim::aimbot_team_check);
    register_float_atomic(HIDE_STR("aimbot.start_delay"), globals::aim::aimbot_start_delay);
    register_float_atomic(HIDE_STR("aimbot.focus_in_seconds"), globals::aim::aimbot_focus_in_seconds);
    register_bool_atomic(HIDE_STR("aimbot.check_jumping"), globals::aim::aimbot_check_jumping);
    register_bool_atomic(HIDE_STR("aimbot.stop_on_kill"), globals::aim::aimbot_stop_on_kill);
    register_float_atomic(HIDE_STR("aimbot.rcs_scale"), globals::aim::aimbot_rcs_scale);
    register_bool_atomic(HIDE_STR("aimbot.dynamic_fov"), globals::aim::aimbot_dynamic_fov);
    register_float_atomic(HIDE_STR("aimbot.overshoot"), globals::aim::aimbot_overshoot);
    register_int_atomic(HIDE_STR("aimbot.curve_type"), globals::aim::aimbot_curve_type);
    register_bool_atomic(HIDE_STR("aimbot.sticky_aim"), globals::aim::aimbot_sticky_aim);
    register_int_atomic(HIDE_STR("silentaim.mode"), globals::aim::silentaim_mode);
    register_bool_atomic(HIDE_STR("silentaim.enabled"), globals::aim::silentaim_enabled);
    register_int_atomic(HIDE_STR("silentaim.bind1"), globals::aim::silentaim_bind1);
    register_int_atomic(HIDE_STR("silentaim.bind2"), globals::aim::silentaim_bind2);
    register_int_atomic(HIDE_STR("silentaim.bind_mode"), globals::aim::silentaim_bind_mode);
    register_int_atomic(HIDE_STR("silentaim.hitpart"), globals::aim::silentaim_hitpart);
    register_int_atomic(HIDE_STR("silentaim.hitpart_blacklist"), globals::aim::silentaim_hitpart_blacklist);
    register_float_atomic(HIDE_STR("silentaim.hit_chance"), globals::aim::silentaim_hit_chance);
    register_float_atomic(HIDE_STR("silentaim.fov"), globals::aim::silentaim_fov);
    register_bool_atomic(HIDE_STR("silentaim.draw_fov"), globals::aim::silentaim_draw_fov);
    register_int_atomic(HIDE_STR("silentaim.target_priority"), globals::aim::silentaim_target_priority);
    register_bool_atomic(HIDE_STR("silentaim.wall_check"), globals::aim::silentaim_wall_check);
    register_bool_atomic(HIDE_STR("silentaim.team_check"), globals::aim::silentaim_team_check);
    register_float_atomic(HIDE_STR("silentaim.start_delay"), globals::aim::silentaim_start_delay);
    register_float_atomic(HIDE_STR("silentaim.focus_in_seconds"), globals::aim::silentaim_focus_in_seconds);
    register_bool_atomic(HIDE_STR("silentaim.check_jumping"), globals::aim::silentaim_check_jumping);

    register_bool_atomic(HIDE_STR("triggerbot.enabled"), globals::triggerbot::triggerbot_enabled);
    register_int_atomic(HIDE_STR("triggerbot.bind1"), globals::triggerbot::triggerbot_bind1);
    register_int_atomic(HIDE_STR("triggerbot.bind2"), globals::triggerbot::triggerbot_bind2);
    register_int_atomic(HIDE_STR("triggerbot.mode"), globals::triggerbot::triggerbot_mode);
    register_int_atomic(HIDE_STR("triggerbot.delay"), globals::triggerbot::triggerbot_delay);
    register_int_atomic(HIDE_STR("triggerbot.delay_variance"), globals::triggerbot::triggerbot_delay_variance);
    register_float_atomic(HIDE_STR("triggerbot.hitchance"), globals::triggerbot::triggerbot_hitchance);
    register_int_atomic(HIDE_STR("triggerbot.hitbox_filter"), globals::triggerbot::triggerbot_hitbox_filter);
    register_bool_atomic(HIDE_STR("triggerbot.burst_mode"), globals::triggerbot::triggerbot_burst_mode);
    register_int_atomic(HIDE_STR("triggerbot.burst_count"), globals::triggerbot::triggerbot_burst_count);
    register_int_atomic(HIDE_STR("triggerbot.post_shot_delay"), globals::triggerbot::triggerbot_post_shot_delay);
    register_bool_atomic(HIDE_STR("triggerbot.wall_check"), globals::triggerbot::triggerbot_wall_check);
    register_bool_atomic(HIDE_STR("triggerbot.team_check"), globals::triggerbot::triggerbot_team_check);
    register_bool_atomic(HIDE_STR("triggerbot.check_flashed"), globals::triggerbot::triggerbot_check_flashed);
    register_bool_atomic(HIDE_STR("triggerbot.check_jumping"), globals::triggerbot::triggerbot_check_jumping);
    register_bool_atomic(HIDE_STR("triggerbot.recoil_comp"), globals::triggerbot::triggerbot_recoil_comp);
    register_float_atomic(HIDE_STR("triggerbot.magnet_fov"), globals::triggerbot::triggerbot_magnet_fov);
    register_float_atomic(HIDE_STR("triggerbot.magnet_smooth"), globals::triggerbot::triggerbot_magnet_smooth);

    register_bool_atomic(HIDE_STR("hitsound.enabled"), globals::aim::hitsound_enabled);
    register_int_atomic(HIDE_STR("hitsound.mode"), globals::aim::hitsound_mode);

    register_bool_atomic(HIDE_STR("visuals.enabled"), globals::visuals::enabled);
    register_bool_atomic(HIDE_STR("visuals.team_check"), globals::visuals::team_check);
    register_bool_atomic(HIDE_STR("visuals.skeleton"), globals::visuals::skeleton);
    register_bool_atomic(HIDE_STR("visuals.box"), globals::visuals::box);
    register_bool_atomic(HIDE_STR("visuals.box_outline"), globals::visuals::box_outline);
    register_float_atomic(HIDE_STR("visuals.box_rounding"), globals::visuals::box_rounding);
    register_bool_atomic(HIDE_STR("visuals.skeleton_shadow"), globals::visuals::skeleton_shadow);
    register_float_atomic(HIDE_STR("visuals.skeleton_thickness"), globals::visuals::skeleton_thickness);
    register_bool_atomic(HIDE_STR("visuals.chams"), globals::visuals::chams);
    register_bool_atomic(HIDE_STR("visuals.chams_fill"), globals::visuals::chams_fill);
    register_bool_atomic(HIDE_STR("visuals.chams_wireframe"), globals::visuals::chams_wireframe);
    register_bool_atomic(HIDE_STR("visuals.display_name"), globals::visuals::display_name);
    register_bool_atomic(HIDE_STR("visuals.display_health"), globals::visuals::display_health);
    register_bool_atomic(HIDE_STR("visuals.display_health_value"), globals::visuals::display_health_value);
    register_bool_atomic(HIDE_STR("visuals.display_tool"), globals::visuals::display_tool);
    register_bool_atomic(HIDE_STR("visuals.display_dist"), globals::visuals::display_dist);
    register_bool_atomic(HIDE_STR("visuals.highlight_target"), globals::visuals::highlight_target);
    register_bool_atomic(HIDE_STR("visuals.box_filled"), globals::visuals::box_filled);
    register_bool_atomic(HIDE_STR("visuals.box_gradient"), globals::visuals::box_gradient);
    register_bool_atomic(HIDE_STR("visuals.hp_bar"), globals::visuals::hp_bar);
    register_bool_atomic(HIDE_STR("visuals.hp_bar_outline"), globals::visuals::hp_bar_outline);
    register_bool_atomic(HIDE_STR("visuals.hp_bar_gradient"), globals::visuals::hp_bar_gradient);
    register_float_atomic(HIDE_STR("visuals.hp_bar_width"), globals::visuals::hp_bar_width);
    register_bool_atomic(HIDE_STR("visuals.visual_check"), globals::visuals::visual_check);
    register_bool_atomic(HIDE_STR("visuals.show_models"), globals::visuals::show_models);
    register_float_atomic(HIDE_STR("visuals.max_dist"), globals::visuals::max_dist);
    register_int_atomic(HIDE_STR("visuals.box_type"), globals::visuals::box_type);
    register_color_setting(HIDE_STR("visuals.enemy_color"), globals::visuals::enemy_color);
    register_color_setting(HIDE_STR("visuals.team_color"), globals::visuals::team_color);
    register_color_setting(HIDE_STR("visuals.skel_color"), globals::visuals::skel_color);
    register_color_setting(HIDE_STR("visuals.chams_color"), globals::visuals::chams_color);
    register_color_setting(HIDE_STR("visuals.box_color"), globals::visuals::box_color);
    register_color_setting(HIDE_STR("visuals.box_filled_color"), globals::visuals::box_filled_color);
    register_color_setting(HIDE_STR("visuals.box_filled_color2"), globals::visuals::box_filled_color2);
    register_color_setting(HIDE_STR("visuals.hp_bar_color1"), globals::visuals::hp_bar_color1);
    register_color_setting(HIDE_STR("visuals.hp_bar_color2"), globals::visuals::hp_bar_color2);

    register_bool_atomic(HIDE_STR("world.fullbright"), globals::world::fullbright);
    register_color_setting(HIDE_STR("world.ambience_color"), globals::world::ambience_color);
    register_bool_atomic(HIDE_STR("world.fov_enabled"), globals::world::fov_enabled);
    register_float_atomic(HIDE_STR("world.fov"), globals::world::fov_value);
    register_bool_atomic(HIDE_STR("world.third_person"), globals::world::third_person);
    register_bool_atomic(HIDE_STR("world.stretched_res_enabled"), globals::world::stretched_res_enabled);
    register_float_atomic(HIDE_STR("world.stretched_res_x"), globals::world::stretched_res_x);
    register_float_atomic(HIDE_STR("world.stretched_res_y"), globals::world::stretched_res_y);

    register_bool_atomic(HIDE_STR("world.skybox_enabled"), globals::world::skybox_enabled);
    register_bool_atomic(HIDE_STR("world.skybox_apply"), globals::world::skybox_apply);
    register_string(HIDE_STR("world.skybox_bk"), globals::world::skybox_bk);
    register_string(HIDE_STR("world.skybox_dn"), globals::world::skybox_dn);
    register_string(HIDE_STR("world.skybox_ft"), globals::world::skybox_ft);
    register_string(HIDE_STR("world.skybox_lf"), globals::world::skybox_lf);
    register_string(HIDE_STR("world.skybox_rt"), globals::world::skybox_rt);
    register_string(HIDE_STR("world.skybox_up"), globals::world::skybox_up);

    register_bool_atomic(HIDE_STR("weather.enabled"), globals::weather::enabled);
    register_int_atomic(HIDE_STR("weather.mode"), globals::weather::mode);
    register_int_atomic(HIDE_STR("weather.particle_count"), globals::weather::particle_count);
    register_float_atomic(HIDE_STR("weather.fall_speed"), globals::weather::fall_speed);
    register_float_atomic(HIDE_STR("weather.drift_strength"), globals::weather::drift_strength);
    register_float_atomic(HIDE_STR("weather.spawn_radius"), globals::weather::spawn_radius);
    register_float_atomic(HIDE_STR("weather.spawn_height"), globals::weather::spawn_height);
    register_float_atomic(HIDE_STR("weather.size_min"), globals::weather::size_min);
    register_float_atomic(HIDE_STR("weather.size_max"), globals::weather::size_max);
    register_color_setting(HIDE_STR("weather.particle_color"), globals::weather::particle_color);
    register_bool_atomic(HIDE_STR("settings.streamproof"), globals::settings::streamproof);
    register_bool_atomic(HIDE_STR("settings.raycast"), globals::settings::raycast);
    register_bool_atomic(HIDE_STR("settings.rescan"), globals::settings::rescan_enabled);
    register_bool_atomic(HIDE_STR("settings.watermark"), globals::settings::watermark);
    register_int_atomic(HIDE_STR("settings.watermark_style"), globals::settings::watermark_style);
    register_float_atomic(HIDE_STR("settings.watermark_width"), globals::settings::watermark_width);
    register_float_atomic(HIDE_STR("settings.watermark_height"), globals::settings::watermark_height);
    register_float_atomic(HIDE_STR("settings.watermark_rounding"), globals::settings::watermark_rounding);
    register_color_setting(HIDE_STR("settings.watermark_bg_color"), globals::settings::watermark_bg_color);
    register_color_setting(HIDE_STR("settings.watermark_border_color"), globals::settings::watermark_border_color);
    register_bool_atomic(HIDE_STR("settings.keybind_list"), globals::settings::keybind_list);
    register_bool_atomic(HIDE_STR("settings.performance_graph"), globals::settings::performance_graph);
    register_float_atomic(HIDE_STR("settings.player_cache_delay"), globals::settings::player_cache_delay);
    register_float_atomic(HIDE_STR("settings.aim_thread_delay"), globals::settings::aim_thread_delay);
    register_float_atomic(HIDE_STR("settings.rescan_thread_delay"), globals::settings::rescan_thread_delay);
    register_bool_atomic(HIDE_STR("settings.fps_limit_enabled"), globals::settings::fps_limit_enabled);
    register_int_atomic(HIDE_STR("settings.fps_limit"), globals::settings::fps_limit);
    register_int_atomic(HIDE_STR("settings.perf_mode"), globals::settings::perf_mode);
    register_int_atomic(HIDE_STR("settings.menu_keybind"), globals::settings::menu_keybind);
    register_color_setting(HIDE_STR("settings.menu_color"), globals::settings::menu_color);

    register_bool_atomic(HIDE_STR("bytecode.enabled"), globals::bytecode::enabled);

    register_bool_atomic(HIDE_STR("character.walkspeed_enabled"), globals::character::walkspeed_enabled);
    register_float_atomic(HIDE_STR("character.walkspeed_value"), globals::character::walkspeed_value);
    register_bool_atomic(HIDE_STR("character.jumppower_enabled"), globals::character::jumppower_enabled);
    register_float_atomic(HIDE_STR("character.jumppower_value"), globals::character::jumppower_value);
    register_bool_atomic(HIDE_STR("character.noclip"), globals::character::noclip);
    register_bool_atomic(HIDE_STR("character.fly"), globals::character::fly);
    register_float_atomic(HIDE_STR("character.fly_speed"), globals::character::fly_speed);
    register_bool_atomic(HIDE_STR("character.inf_jump"), globals::character::inf_jump);

    register_bool_atomic(HIDE_STR("rivals.auto_parry"), globals::rivals::auto_parry);
    register_bool_atomic(HIDE_STR("rivals.auto_block"), globals::rivals::auto_block);
    register_bool_atomic(HIDE_STR("rivals.auto_swing"), globals::rivals::auto_swing);
    register_bool_atomic(HIDE_STR("rivals.kill_aura"), globals::rivals::kill_aura);
    register_bool_atomic(HIDE_STR("rivals.auto_farm"), globals::rivals::auto_farm);
}

} // namespace config
