#pragma once
#include "executor/listeners/listener.h"

class hookfunction_listener : public Listener {
public:
	std::string GetAction() const override;
	void Callback(ix::WebSocket& websocket, const ix::WebSocketMessagePtr& message, const json& data) override;
};
