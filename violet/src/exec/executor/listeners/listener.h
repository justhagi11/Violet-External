#pragma once

#include "ixwebsocket/IXWebSocket.h"
#include "nlohmann/json.hpp"
using nlohmann::json;

class Websocket;

class Listener {
protected:
	Websocket* _server;
public:
	virtual ~Listener() = default;

	virtual void SetContext(Websocket* server);
	virtual std::string GetAction() const = 0;
	virtual void Callback(ix::WebSocket& websocket, const ix::WebSocketMessagePtr&, const json& data) = 0;
};
