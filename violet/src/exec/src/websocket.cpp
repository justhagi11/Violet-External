#include <Windows.h>
#include "executor/logger.h"
#include <future>
#include <regex>

#include "ixwebsocket/IXBase64.h"

#include "executor/bytecode.h"
#include "executor/websocket.h"
#include "executor/utils.h"
#include "executor/client.h"

#include "executor/listeners/listener.h"
#include "executor/listeners/loadstring.h"
#include "executor/listeners/initialize.h"
#include "executor/listeners/getscriptbytecode.h"
#include "executor/listeners/request.h"
#include "executor/listeners/is_compilable.h"
#include "executor/listeners/queue_on_teleport.h"
#include "executor/listeners/setreadonly.h"
#include "executor/listeners/hookfunction.h"
#include "executor/listeners/setrequirebypass.h"
#include "executor/listeners/filesystem.h"

Websocket::Websocket(std::string host, int port) : _host(host), _port(port), _server(port, host) {
    _server.setOnClientMessageCallback(
        [this](std::shared_ptr<ix::ConnectionState> connection_state, ix::WebSocket& websocket, const ix::WebSocketMessagePtr& message) {
            switch (message->type) {
            case ix::WebSocketMessageType::Open: {

                break;
            }

            case ix::WebSocketMessageType::Close: {
                std::lock_guard<std::mutex> lock(_connection_mutex);

                auto it = std::find_if(
                    _connections.begin(),
                    _connections.end(),
                    [&](const std::pair<DWORD, ix::WebSocket*> pair)
                    {
                        return pair.second == &websocket;
                    });

                if (it != _connections.end())
                {
                    _connections.erase(it);
                }
                break;
            }

            case ix::WebSocketMessageType::Message: {
                OnMessage(websocket, message);
                break;
            }

            default:
                break;
            }
        }
    );
    SetupListeners();
}

Websocket::~Websocket() {
    Stop();
}

const std::string& Websocket::GetHost() const {
    return _host;
}

int Websocket::GetPort() const {
    return _port;
}

const Clients& Websocket::GetClients() const {
    return _clients;
}

Connections& Websocket::GetConnections() {
    return _connections;
}

ix::WebSocket* Websocket::GetConnection(DWORD PID) const {
    auto it = _connections.find(PID);

    if (it != _connections.end()) return it->second;

    return nullptr;
}

bool Websocket::Run() {
    auto res = _server.listen();
    if (!res.first)
    {
        logger::error("Failed to listen: {}", res.second);
        return false;
    }

    _server.start();
    return true;
}

void Websocket::Stop() {
    _server.stop();

    std::lock_guard<std::mutex> lock(_connection_mutex);
    _connections.clear();
}

void Websocket::Send(const std::string& data, DWORD PID) const {
    auto connection = GetConnection(PID);
    if (!connection) return;

    connection->send(data);
}

void Websocket::Send(const json& data, DWORD PID) const {
    std::string dump = data.dump();
    Send(dump, PID);
}

void Websocket::SendBinary(const std::string& binary, DWORD PID) const {
    auto connection = GetConnection(PID);
    if (!connection) return;

    connection->sendBinary(binary);
}

void Websocket::SendBinary(const MsgPackMap& binary, DWORD PID) const {
    msgpack::sbuffer sbuffer;
    msgpack::pack(sbuffer, binary);

    std::string buffer(sbuffer.data(), sbuffer.size());
    SendBinary(std::move(buffer), PID);
}

std::optional<json> Websocket::SendAndReceive(json& data, DWORD PID, int timeout) {
    std::string id = GenerateGUID();

    data["id"] = id;

    auto promise = std::make_unique<std::promise<json>>();
    auto future = promise->get_future();

    {
        std::lock_guard<std::mutex> lock(_request_mutex);
        _on_going_requests.emplace(id, std::move(promise));
    }

    Send(data, PID);

    if (future.wait_for(std::chrono::seconds(timeout)) == std::future_status::ready) {
        std::lock_guard<std::mutex> lock(_request_mutex);

        json response = future.get();

        _on_going_requests.erase(id);

        return response;
    }

    {
        std::lock_guard<std::mutex> lock(_request_mutex);
        _on_going_requests.erase(id);
    }

    return std::nullopt;
}

void Websocket::RegisterConnection(DWORD PID, ix::WebSocket* websocket) {
    std::lock_guard<std::mutex> lock(_connection_mutex);

    _connections[PID] = websocket;

    websocket->setPingInterval(10000);
}

void Websocket::UnregisterConnection(DWORD PID) {

}

void Websocket::AddClient(std::unique_ptr<Client> client) {
    std::lock_guard<std::mutex> lock(_client_mutex);

    HANDLE handle = client->GetProcess()->GetHandle();
    DWORD PID = client->GetProcess()->GetProcessId();

    _clients.push_back(std::move(client));

    std::thread([this, PID, handle]() {
        WaitForSingleObject(handle, INFINITE);
        RemoveClient(PID);
    }).detach();
}

void Websocket::RemoveClient(DWORD PID) {
    std::lock_guard<std::mutex> lock(_client_mutex);

    if (auto it = _connections.find(PID); it != _connections.end()) {
        if (it->second) it->second->close();
        _connections.erase(it);
    }

    auto it = std::remove_if(_clients.begin(), _clients.end(),
        [&](const std::unique_ptr<Client>& client) {
            if (client && client->GetProcess()->GetProcessId() == PID) {
                client->Shutdown();
                return true;
            }
            return false;
        });
    _clients.erase(it, _clients.end());
}

void Websocket::AddListener(std::unique_ptr<Listener> listener) {
    listener->SetContext(this);
    _listeners.push_back(std::move(listener));
}

void Websocket::OnMessage(ix::WebSocket& websocket, const ix::WebSocketMessagePtr& message) {
    json data = json::parse(message->str, nullptr, false);
    if (data.is_discarded()) return;

    Data request = ParseData(data);
    if (!request.id.empty() and request.type == "response") {
        std::lock_guard<std::mutex> lock(_request_mutex);
        auto it = _on_going_requests.find(request.id);
        if (it != _on_going_requests.end()) {
            it->second->set_value(data);
            _on_going_requests.erase(it);
            return;
        }
    }

    if (request.action.empty() || !request.pid) return;

    for (const auto& listener : _listeners) {
        if (listener->GetAction() == request.action) {
            listener->Callback(websocket, message, data);
            return;
        }
    }
}

Client* Websocket::GetClient(DWORD PID) {
    auto it = std::find_if(_clients.begin(), _clients.end(),
        [PID](const std::unique_ptr<Client>& client) {
            return client && client->GetProcess()->GetProcessId() == PID;
        });
    return it != _clients.end() ? it->get() : nullptr;
}

void Websocket::SetupListeners() {
    AddListener(std::make_unique<initialize>());
    AddListener(std::make_unique<is_compilable>());
    AddListener(std::make_unique<loadstring>());
    AddListener(std::make_unique<getscriptbytecode>());
    AddListener(std::make_unique<request>());
    AddListener(std::make_unique<queue_on_teleport>());
    AddListener(std::make_unique<setreadonly_listener>());
    AddListener(std::make_unique<hookfunction_listener>());
    AddListener(std::make_unique<setrequirebypass_listener>());
    AddListener(std::make_unique<filesystem_listener>());
}
