#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include "../../security/obfuscator.hpp"

namespace ipc {

class tcp_server {
public:
    struct impl;

    explicit tcp_server(std::string host = HIDE_STR("127.0.0.1"), std::uint16_t port = 5005);
    ~tcp_server();

    auto start() -> bool;
    auto stop() -> void;
    auto is_running() const -> bool;
    auto broadcast(std::string message) -> void;

private:
    std::unique_ptr<impl> impl_;
};

} // namespace ipc
