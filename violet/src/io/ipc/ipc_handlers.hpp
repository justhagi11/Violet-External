#pragma once

#include <string>

namespace ipc::handlers {

auto handle_message(const std::string& payload) -> std::string;

} // namespace ipc::handlers
