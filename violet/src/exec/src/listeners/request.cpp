#include "executor/websocket.h"
#include "executor/utils.h"
#include "executor/listeners/request.h"

#include "httplib/httplib.h"

std::string request::GetAction() const {
	return "request";
}

void request::Callback(ix::WebSocket& websocket, const ix::WebSocketMessagePtr&, const json& data) {
    Data request = ParseData(data);

    json response;
    response["type"] = "response";
    response["success"] = false;
    if (!request.id.empty()) response["id"] = request.id;

    if (IsMissingKeys(data, { "url", "method" })) {
        response["message"] = "missing required fields";
        return _server->Send(response, request.pid);
    }

    const std::string& url = data["url"];
    const std::string& method = data["method"];
    const json headers = data.contains("headers") ? data["headers"] : json::object();
    const std::string body = data.contains("body") ? data["body"] : "";

    auto parsed_url = ParseUrl(url);
    if (!parsed_url) {
        response["message"] = "invalid url";
        return _server->Send(response, request.pid);
    }

    const auto& [host, path] = *parsed_url;

    httplib::Client client(host.c_str());
    client.set_follow_location(true);

    httplib::Headers request_headers;
    std::string content_type = "application/json";

    for (auto& [key, value] : headers.items()) {
        std::string header_key = key;
        std::string header_value = value.get<std::string>();

        if (header_key == "Content-Type" || header_key == "content-type") {
            content_type = header_value;
        }

        request_headers.insert({ header_key, header_value });
    }

    httplib::Result result;

    if (method == "GET") {
        result = client.Get(path, request_headers);
    }
    else if (method == "POST") {
        result = client.Post(path, request_headers, body, content_type);
    }
    else if (method == "PUT") {
        result = client.Put(path, request_headers, body, content_type);
    }
    else if (method == "DELETE") {
        result = client.Delete(path, request_headers, body, content_type);
    }
    else if (method == "PATCH") {
        result = client.Patch(path, request_headers, body, content_type);
    }
    else if (method == "HEAD") {
        result = client.Head(path, request_headers);
    }
    else if (method == "OPTIONS") {
        result = client.Options(path, request_headers);
    }
    else {
        response["message"] = "unsupported http method";
        return _server->Send(response, request.pid);
    }

    if (!result) {
        response["message"] = httplib::to_string(result.error());
        return _server->Send(response, request.pid);
    }

    response["response"]["success"] = (result->status >= 200 && result->status <= 299);
    response["response"]["status_code"] = result->status;
    response["response"]["status_message"] = result->reason;

    json json_headers;
    for (const auto& header : result->headers) {
        json_headers[header.first] = header.second;
    }

    response["response"]["headers"] = json_headers;
    response["response"]["body"] = result->body;

    response["success"] = true;
    return _server->Send(response, request.pid);
}
