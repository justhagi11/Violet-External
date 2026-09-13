#include "ipc_handlers.hpp"
#include "config.hpp"
#include "../../security/obfuscator.hpp"

namespace ipc::handlers {
namespace {

// [ipc::make_error]
auto make_error(std::string_view reason) -> std::string {
    return HIDE_STR("{\"ok\":false,\"error\":\"") + config::json::escape(reason) + HIDE_STR("\"}");
}

} // namespace

// [ipc::handle_message]
auto handle_message(const std::string& payload) -> std::string {
    std::map<std::string, std::string> request;
    if (!config::json::parse_object(payload, request)) {
        return make_error(HIDE_STR("invalid json payload"));
    }

    const auto action_it = request.find(HIDE_STR("action"));
    if (action_it == request.end()) {
        return make_error(HIDE_STR("missing action"));
    }

    std::string action;
    if (!config::json::parse_string(action_it->second, action)) {
        return make_error(HIDE_STR("action must be string"));
    }

    auto& cfg = config::manager::instance();

    if (action == HIDE_STR("set")) {
        const auto key_it = request.find(HIDE_STR("key"));
        const auto value_it = request.find(HIDE_STR("value"));

        if (key_it == request.end() || value_it == request.end()) {
            return make_error(HIDE_STR("set requires key and value"));
        }

        std::string key;
        if (!config::json::parse_string(key_it->second, key)) {
            return make_error(HIDE_STR("key must be string"));
        }

        if (!cfg.set_from_json(key, value_it->second)) {
            return make_error(HIDE_STR("failed to update key"));
        }

        return HIDE_STR("{\"ok\":true}");
    }

    if (action == HIDE_STR("get_config")) {
        return HIDE_STR("{\"ok\":true,\"config\":") + cfg.serialize() + HIDE_STR("}");
    }

    if (action == HIDE_STR("set_config")) {
        const auto config_it = request.find(HIDE_STR("config"));
        if (config_it == request.end()) {
            return make_error(HIDE_STR("set_config requires config"));
        }

        if (!cfg.deserialize(config_it->second)) {
            return make_error(HIDE_STR("failed to deserialize config"));
        }

        return HIDE_STR("{\"ok\":true}");
    }

    if (action == HIDE_STR("ping")) {
        return HIDE_STR("{\"ok\":true,\"message\":\"pong\"}");
    }

    return make_error(HIDE_STR("unknown action"));
}

} // namespace ipc::handlers
