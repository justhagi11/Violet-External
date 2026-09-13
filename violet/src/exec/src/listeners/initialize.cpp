#include "executor/websocket.h"
#include "executor/listeners/initialize.h"

std::string initialize::GetAction() const {
	return "initialize";
}

void initialize::Callback(ix::WebSocket& websocket, const ix::WebSocketMessagePtr&, const json& data) {
	_server->RegisterConnection(data["pid"], &websocket);
}
