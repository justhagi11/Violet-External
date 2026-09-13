#pragma once

#include <Windows.h>
#include <map>
#include <future>
#include <memory>
#include <unordered_map>
#include <functional>

#include "ixwebsocket/IXWebSocket.h"
#include "ixwebsocket/IXWebSocketServer.h"
#include "nlohmann/json.hpp"
#include "msgpack/msgpack.hpp"
#include "executor/client.h"

#include "executor/listeners/listener.h"

using nlohmann::json;
using MsgPackMap = std::map<std::string, msgpack::object>;
using Connections = std::map<DWORD, ix::WebSocket*>;
using MessageListener = std::function<void(ix::WebSocket&, const ix::WebSocketMessagePtr&, const json&)>;

class Websocket {
private:
	ix::WebSocketServer _server;
	Connections _connections;
	Clients _clients;

	const std::string _host;
	const int _port;

	std::unordered_map<std::string, std::unique_ptr<std::promise<json>>> _on_going_requests;
	std::vector<std::unique_ptr<Listener>> _listeners;

	std::mutex _connection_mutex;
	std::mutex _client_mutex;
	std::mutex _request_mutex;
	std::mutex _listener_mutex;

	void SetupListeners();
public:
	Websocket(std::string host, int port);
	~Websocket();

	bool Run();
	void Stop();

	void Send(const std::string& data, DWORD PID) const;
	void Send(const json& data, DWORD PID) const;
	void SendBinary(const std::string& binary, DWORD PID) const;
	void SendBinary(const MsgPackMap& binary, DWORD PID) const;
	std::optional<json> SendAndReceive(json& data, DWORD PID, int timeout=5);

	int GetPort() const;
	const std::string& GetHost() const;
	const Clients& GetClients() const;
	Connections& GetConnections();
	ix::WebSocket* GetConnection(DWORD PID) const;

	void RegisterConnection(DWORD PID, ix::WebSocket* websocket);
	void UnregisterConnection(DWORD PID);

	void AddClient(std::unique_ptr<Client> client);
	void RemoveClient(DWORD PID);
	Client* GetClient(DWORD PID);

	void OnMessage(ix::WebSocket& websocket, const ix::WebSocketMessagePtr& message);
	void AddListener(std::unique_ptr<Listener> listener);
};
