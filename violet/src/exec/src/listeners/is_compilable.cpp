#include "executor/websocket.h"
#include "executor/utils.h"
#include "executor/listeners/is_compilable.h"

#include "executor/bytecode.h"

std::string is_compilable::GetAction() const {
    return "is_compilable";
}

void is_compilable::Callback(ix::WebSocket& websocket, const ix::WebSocketMessagePtr&, const json& data) {
    Data request = ParseData(data);

    json response;
    response["type"] = "response";
    response["success"] = false;

    if (!request.id.empty()) response["id"] = request.id;

    if (IsMissingKeys(data, { "source" })) {
        response["message"] = "missing required fields";
        return _server->Send(response, request.pid);
    }

    const std::string& source = data["source"];
    auto [compile_success, bytecode_or_error] = Bytecode::Compile(source);

    if (!compile_success) {
        response["message"] = bytecode_or_error;
        return _server->Send(response, request.pid);
    }

    response["success"] = true;
    return _server->Send(response, request.pid);
}
