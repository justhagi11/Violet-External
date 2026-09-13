#include "executor/websocket.h"
#include "executor/listeners/setrequirebypass.h"
#include "executor/utils.h"
#include "executor/instances/datamodel.h"
#include "executor/instances/scriptcontext.h"

std::string setrequirebypass_listener::GetAction() const {
    return "setrequirebypass";
}

void setrequirebypass_listener::Callback(ix::WebSocket& websocket, const ix::WebSocketMessagePtr&, const json& data) {
    Data request = ParseData(data);
    json response;

    response["type"] = "response";
    response["success"] = false;
    if (!request.id.empty()) response["id"] = request.id;

    if (IsMissingKeys(data, { "enabled" })) {
        response["message"] = "missing required fields";
        return _server->Send(response, request.pid);
    }

    bool enabled = data["enabled"].get<bool>();

    Client* client = _server->GetClient(request.pid);

    if (!client) {
        response["message"] = "failed to find client " + std::to_string(request.pid);
        return _server->Send(response, request.pid);
    }

    DataModel datamodel(GetDataModel(client->GetProcess()), client->GetProcess());

    if (!datamodel.IsValid()) {
        response["message"] = "failed to get datamodel";
        return _server->Send(response, request.pid);
    }

    ScriptContext scriptcontext = datamodel.FindFirstChildOfClass("ScriptContext");
    if (!scriptcontext.IsValid()) {
        response["message"] = "failed to get scriptcontext";
        return _server->Send(response, request.pid);
    }

    scriptcontext.SetRequireBypass(enabled);

    response["success"] = true;
    return _server->Send(response, request.pid);
}
