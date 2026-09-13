#pragma once

#include "executor/listeners/listener.h"

class queue_on_teleport : public Listener {
public:

	std::string GetAction() const override;
	void Callback(ix::WebSocket& websocket, const ix::WebSocketMessagePtr&, const json& data) override;
};
