
#include "msgpack/msgpack.hpp"

#include "executor/websocket.h"
#include "executor/listeners/getscriptbytecode.h"
#include "executor/utils.h"

#include "executor/instances/datamodel.h"
#include "executor/instances/modulescript.h"
#include "executor/instances/objectvalue.h"
#include "executor/instances/localscript.h"

std::string getscriptbytecode::GetAction() const {
    return "getscriptbytecode";
}

void getscriptbytecode::Callback(ix::WebSocket& websocket, const ix::WebSocketMessagePtr&, const json& data) {
	Data request = ParseData(data);

    MsgPackMap response;
    msgpack::zone zone;

    response["type"] = msgpack::object("response", zone);
    response["success"] = msgpack::object(false, zone);

    if (!request.id.empty()) response["id"] = msgpack::object(request.id, zone);

    if (IsMissingKeys(data, { "pointer_name" })) {
        response["message"] = msgpack::object("missing required fields", zone);
        return _server->SendBinary(response, request.pid);
    }

    const std::string& pointer_name = data["pointer_name"];

    Client* client = _server->GetClient(request.pid);
    if (!client) {
        response["message"] = msgpack::object("failed to find client " + std::to_string(request.pid), zone);
        return _server->SendBinary(response, request.pid);
    }

    DataModel datamodel(GetDataModel(client->GetProcess()), client->GetProcess());
    ObjectValue pointer = ObjectValue(datamodel.FindFirstChildOfClass("CorePackages").FindFirstChildFromPath(client->GetExecutorName() + ".Objects." + pointer_name));

    if (!pointer.IsValid()) {
        response["message"] = msgpack::object("failed to find pointer", zone);
        return _server->SendBinary(response, request.pid);
    }

    Instance value(pointer.Value());

    std::string bytecode;

    if (value.ClassName() == "LocalScript") {
        LocalScript script(value);
        bytecode = script.GetBytecode();
    }
    else if (value.ClassName() == "ModuleScript") {
        ModuleScript script(value);
        bytecode = script.GetBytecode();
    }
    else {
        bytecode = "";
    };

    response["success"] = msgpack::object(true, zone);
    response["bytecode"] = msgpack::object(bytecode, zone);

    return _server->SendBinary(response, request.pid);
}
