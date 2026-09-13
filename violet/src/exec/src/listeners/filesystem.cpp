#include "executor/websocket.h"
#include "executor/listeners/filesystem.h"
#include "executor/utils.h"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

std::string filesystem_listener::GetAction() const {
    return "filesystem";
}

void filesystem_listener::Callback(ix::WebSocket& websocket, const ix::WebSocketMessagePtr&, const json& data) {
    Data request = ParseData(data);
    json response;

    response["type"] = "response";
    response["success"] = false;
    if (!request.id.empty()) response["id"] = request.id;

    if (IsMissingKeys(data, { "action_type", "path" })) {
        response["message"] = "missing required fields";
        return _server->Send(response, request.pid);
    }

    std::string action_type = data["action_type"];
    std::string path_str = data["path"];
    fs::path base_path = fs::current_path() / "workspace";
    fs::path target_path = base_path / path_str;

    if (target_path.lexically_normal().string().find(base_path.lexically_normal().string()) != 0) {
        response["message"] = "path traversal attempted";
        return _server->Send(response, request.pid);
    }

    if (!fs::exists(base_path)) {
        fs::create_directories(base_path);
    }

    try {
        if (action_type == "readfile") {
            if (!fs::exists(target_path) || !fs::is_regular_file(target_path)) {
                response["message"] = "file not found";
            } else {
                std::ifstream ifs(target_path, std::ios::binary);
                std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
                response["content"] = content;
                response["success"] = true;
            }
        } else if (action_type == "writefile") {
            if (IsMissingKeys(data, { "content" })) {
                response["message"] = "missing content";
            } else {
                std::string content = data["content"];
                std::ofstream ofs(target_path, std::ios::binary);
                ofs << content;
                response["success"] = true;
            }
        } else if (action_type == "appendfile") {
            if (IsMissingKeys(data, { "content" })) {
                response["message"] = "missing content";
            } else {
                std::string content = data["content"];
                std::ofstream ofs(target_path, std::ios::binary | std::ios::app);
                ofs << content;
                response["success"] = true;
            }
        } else if (action_type == "makefolder") {
            fs::create_directories(target_path);
            response["success"] = true;
        } else if (action_type == "delfolder") {
            if (fs::exists(target_path) && fs::is_directory(target_path)) {
                fs::remove_all(target_path);
            }
            response["success"] = true;
        } else if (action_type == "delfile") {
            if (fs::exists(target_path) && fs::is_regular_file(target_path)) {
                fs::remove(target_path);
            }
            response["success"] = true;
        } else if (action_type == "listfiles") {
            if (fs::exists(target_path) && fs::is_directory(target_path)) {
                json files = json::array();
                for (const auto& entry : fs::directory_iterator(target_path)) {
                    files.push_back(entry.path().string());
                }
                response["files"] = files;
                response["success"] = true;
            } else {
                response["message"] = "directory not found";
            }
        } else if (action_type == "isfile") {
            response["result"] = fs::exists(target_path) && fs::is_regular_file(target_path);
            response["success"] = true;
        } else if (action_type == "isfolder") {
            response["result"] = fs::exists(target_path) && fs::is_directory(target_path);
            response["success"] = true;
        } else {
            response["message"] = "invalid action_type";
        }
    } catch (const std::exception& e) {
        response["message"] = e.what();
    }

    return _server->Send(response, request.pid);
}
