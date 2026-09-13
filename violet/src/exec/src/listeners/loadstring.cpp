#include "executor/websocket.h"
#include "executor/utils.h"
#include "executor/bytecode.h"
#include "executor/listeners/loadstring.h"

#include "executor/instances/datamodel.h"
#include "executor/instances/modulescript.h"

std::string loadstring::GetAction() const {
    return "loadstring";
}

void loadstring::Callback(ix::WebSocket& websocket, const ix::WebSocketMessagePtr&, const json& data) {
    Data request = ParseData(data);
    json response;

    response["type"] = "response";
    response["success"] = false;
    if (!request.id.empty()) response["id"] = request.id;

    if (IsMissingKeys(data, { "chunk", "chunk_name", "module_name" })) {
        response["message"] = "missing required fields";
        return _server->Send(response, request.pid);
    }

    const std::string& chunk = data["chunk"];
    const std::string& chunk_name = data["chunk_name"];
    const std::string& module_name = data["module_name"];

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

    ModuleScript module(datamodel.FindFirstChildOfClass("CorePackages").FindFirstChildFromPath(client->GetExecutorName() + ".Scripts." + module_name));

    if (!module.IsValid()) {
        response["message"] = "failed to find module";
        return _server->Send(response, request.pid);
    }

    auto [compile_success, bytecode_or_error] = Bytecode::Compile("local function " + chunk_name + "(...)" + chunk + "\nend;return " + chunk_name);

    if (!compile_success) {
        response["message"] = bytecode_or_error;
        return _server->Send(response, request.pid);
    }

    module.SetBytecode(bytecode_or_error);

    response["success"] = true;
    return _server->Send(response, request.pid);
}
