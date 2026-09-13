#include "executor/websocket.h"
#include "executor/utils.h"
#include "executor/listeners/queue_on_teleport.h"

std::string queue_on_teleport::GetAction() const {
	return "queue_on_teleport";
}

void queue_on_teleport::Callback(ix::WebSocket& websocket, const ix::WebSocketMessagePtr&, const json& data) {
	Data request = ParseData(data);

    json response;
    response["type"] = "response";
    response["success"] = false;
    if (!request.id.empty()) response["id"] = request.id;

    if (IsMissingKeys(data, { "source" })) {
        response["message"] = "missing required fields";
        return _server->Send(response, request.pid);
    }

    const std::string& chunk = data["source"];
    Client* client = _server->GetClient(request.pid);

    if (!client) {
        response["message"] = "failed to find client " + std::to_string(request.pid);
        return _server->Send(response, request.pid);
    }

    client->QueueOnTeleport(chunk);

    response["success"] = true;
    return _server->Send(response, request.pid);
}
