#include <iostream>
#include "executor/websocket.h"
#include "executor/listeners/setreadonly.h"
#include "executor/client.h"
#include "executor/logger.h"

std::string setreadonly_listener::GetAction() const {
	return "setreadonly";
}

void setreadonly_listener::Callback(ix::WebSocket& websocket, const ix::WebSocketMessagePtr&, const json& data) {
	json response;
	response["type"] = "response";
	response["success"] = true;

	if (data.contains("id")) response["id"] = data["id"];

	logger::info("setreadonly: faked success for addr={}", data.contains("address") ? data["address"].get<std::string>() : "unknown");

	websocket.send(response.dump());
}
