#include <winsock2.h>
#include <ws2tcpip.h>

#include <array>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

#include "server.hpp"
#include "../../core/logger.hpp"
#include "../../core/globals.hpp"
#include "../../security/obfuscator.hpp"
#include "ipc_handlers.hpp"

#pragma comment(lib, "Ws2_32.lib")

namespace ipc {

struct tcp_server::impl {
    std::string host;
    std::uint16_t port;
    std::atomic<bool> running{ false };
    std::jthread worker;

    SOCKET listen_socket{ INVALID_SOCKET };
    SOCKET client_socket{ INVALID_SOCKET };
    std::mutex socket_mutex;

    explicit impl(std::string host_value, std::uint16_t port_value)
        : host(std::move(host_value)),
        port(port_value) {
    }
};

namespace {

// [ipc::close_socket]
auto close_socket(SOCKET& socket_handle) -> void {
    if (socket_handle == INVALID_SOCKET) {
        return;
    }

    shutdown(socket_handle, SD_BOTH);
    closesocket(socket_handle);
    socket_handle = INVALID_SOCKET;
}

// [ipc::close_all_sockets]
auto close_all_sockets(tcp_server::impl* state) -> void {
    std::scoped_lock lock(state->socket_mutex);
    close_socket(state->client_socket);
    close_socket(state->listen_socket);
}

// [ipc::wait_for_readable]
auto wait_for_readable(SOCKET socket_handle, long timeout_ms) -> int {
    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(socket_handle, &read_set);

    timeval timeout{};
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    return select(0, &read_set, nullptr, nullptr, &timeout);
}

// [ipc::extract_next_json_document]
auto extract_next_json_document(std::string& input_buffer, std::string& out_document) -> bool {
    out_document.clear();

    const std::size_t start = input_buffer.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        input_buffer.clear();
        return false;
    }

    if (start > 0) {
        input_buffer.erase(0, start);
    }

    if (input_buffer.empty()) {
        return false;
    }

    const char first = input_buffer.front();
    if (first != '{' && first != '[') {
        const std::size_t newline = input_buffer.find('\n');
        if (newline == std::string::npos) {
            return false;
        }

        input_buffer.erase(0, newline + 1);
        return false;
    }

    int depth = 0;
    bool in_string = false;
    bool escaped = false;

    for (std::size_t index = 0; index < input_buffer.size(); ++index) {
        const char current = input_buffer[index];

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
                out_document = input_buffer.substr(0, index + 1);
                input_buffer.erase(0, index + 1);
                return true;
            }

            if (depth < 0) {
                input_buffer.clear();
                return false;
            }
        }
    }

    return false;
}

// [ipc::send_all]
auto send_all(SOCKET socket_handle, const std::string& payload) -> bool {
    std::size_t sent_total = 0;

    while (sent_total < payload.size()) {
        const int sent = send(
            socket_handle,
            payload.data() + sent_total,
            static_cast<int>(payload.size() - sent_total),
            0);

        if (sent == SOCKET_ERROR || sent == 0) {
            return false;
        }

        sent_total += static_cast<std::size_t>(sent);
    }

    return true;
}

// [ipc::run_server_loop]
auto run_server_loop(tcp_server::impl* state, std::stop_token stop_token) -> void {
    WSADATA wsa_data{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        logger::error(HIDE_STR("ipc: WSAStartup failed"));
        state->running.store(false);
        return;
    }

    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == INVALID_SOCKET) {
        logger::error(HIDE_STR("ipc: failed to create socket"));
        WSACleanup();
        state->running.store(false);
        return;
    }

    const BOOL reuse_address = TRUE;
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse_address), sizeof(reuse_address));

    sockaddr_in bind_address{};
    bind_address.sin_family = AF_INET;
    bind_address.sin_port = htons(state->port);

    if (inet_pton(AF_INET, state->host.c_str(), &bind_address.sin_addr) != 1) {
        logger::error(HIDE_STR("ipc: invalid bind address: {}"), state->host);
        closesocket(listen_socket);
        WSACleanup();
        state->running.store(false);
        return;
    }

    if (bind(listen_socket, reinterpret_cast<const sockaddr*>(&bind_address), sizeof(bind_address)) == SOCKET_ERROR) {
        logger::error(HIDE_STR("ipc: bind failed on {}:{}"), state->host, state->port);
        closesocket(listen_socket);
        WSACleanup();
        state->running.store(false);
        return;
    }

    if (listen(listen_socket, 1) == SOCKET_ERROR) {
        logger::error(HIDE_STR("ipc: listen failed"));
        closesocket(listen_socket);
        WSACleanup();
        state->running.store(false);
        return;
    }

    {
        std::scoped_lock lock(state->socket_mutex);
        state->listen_socket = listen_socket;
    }

    while (!stop_token.stop_requested() && state->running.load() && globals::running.load()) {
        const int accept_ready = wait_for_readable(listen_socket, 250);

        if (accept_ready == SOCKET_ERROR) {
            if (!stop_token.stop_requested() && state->running.load()) {
                logger::warn(HIDE_STR("ipc: accept select error"));
            }
            break;
        }

        if (accept_ready == 0) {
            continue;
        }

        SOCKET client_socket = accept(listen_socket, nullptr, nullptr);
        if (client_socket == INVALID_SOCKET) {
            continue;
        }

        {
            std::scoped_lock lock(state->socket_mutex);
            state->client_socket = client_socket;
        }

        std::string incoming_buffer;
        std::array<char, 4096> recv_buffer{};

        bool client_connected = true;
        while (client_connected && !stop_token.stop_requested() && state->running.load() && globals::running.load()) {
            const int recv_ready = wait_for_readable(client_socket, 250);

            if (recv_ready == SOCKET_ERROR) {
                client_connected = false;
                break;
            }

            if (recv_ready == 0) {
                continue;
            }

            const int received = recv(client_socket, recv_buffer.data(), static_cast<int>(recv_buffer.size()), 0);
            if (received <= 0) {
                client_connected = false;
                break;
            }

            incoming_buffer.append(recv_buffer.data(), static_cast<std::size_t>(received));

            std::string message;
            while (extract_next_json_document(incoming_buffer, message)) {
                std::string response = handlers::handle_message(message);
                if (response.empty()) {
                    continue;
                }

                response.push_back('\n');
                if (!send_all(client_socket, response)) {
                    client_connected = false;
                    break;
                }
            }
        }

        {
            std::scoped_lock lock(state->socket_mutex);
            close_socket(state->client_socket);
        }
    }

    close_all_sockets(state);
    WSACleanup();
    state->running.store(false);
}

} // namespace

// [ipc::tcp_server]
tcp_server::tcp_server(std::string host, std::uint16_t port)
    : impl_(std::make_unique<impl>(std::move(host), port)) {
}

// [ipc::~tcp_server]
tcp_server::~tcp_server() {
    stop();
}

// [ipc::start]
auto tcp_server::start() -> bool {
    bool expected = false;
    if (!impl_->running.compare_exchange_strong(expected, true)) {
        return false;
    }

    impl_->worker = std::jthread([state = impl_.get()](std::stop_token stop_token) {
        run_server_loop(state, stop_token);
        });

    return true;
}

// [ipc::stop]
auto tcp_server::stop() -> void {
    impl_->running.store(false);

    if (!impl_->worker.joinable()) {
        close_all_sockets(impl_.get());
        return;
    }

    impl_->worker.request_stop();
    close_all_sockets(impl_.get());
    impl_->worker.join();
}

// [ipc::is_running]
auto tcp_server::is_running() const -> bool {
    return impl_->running.load();
}

// [ipc::broadcast]
auto tcp_server::broadcast(std::string message) -> void {
    if (!impl_ || !impl_->running.load()) return;
    std::scoped_lock lock(impl_->socket_mutex);
    if (impl_->client_socket != INVALID_SOCKET) {
        message.push_back('\n');
        send_all(impl_->client_socket, message);
    }
}

} // namespace ipc
