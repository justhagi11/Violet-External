#pragma once

#include "executor/listeners/listener.h"

class is_compilable : public Listener {
public:

	std::string GetAction() const override;
	void Callback(ix::WebSocket& websocket, const ix::WebSocketMessagePtr&, const json& data) override;
};
