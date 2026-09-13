#include <Windows.h>
#include <objbase.h>
#include <iostream>
#include <string>
#include <stdexcept>
#include <optional>
#include "executor/logger.h"

#include "executor/utils.h"
#include "executor/process.h"
#include "game/offsets.hpp"

uintptr_t GetDataModel(const Process* process) {
    uintptr_t fake_datamodel = process->Read<uintptr_t>(process->GetBaseAddress() + offsets::FakeDataModel::Pointer);

    if (!fake_datamodel) return 0;

    return process->Read<uintptr_t>(fake_datamodel + offsets::FakeDataModel::RealDataModel);
}

HMODULE GetModule() {
    HMODULE module = nullptr;
    GetModuleHandleEx(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        reinterpret_cast<LPCTSTR>(&GetModule),
        &module);
    return module;
}

bool ReplaceString(std::string& data, const std::string_view replace, const std::string_view replacement) {
    size_t pos = data.find(replace);

    if (pos == std::string::npos)
        return false;

    data.replace(pos, replace.length(), replacement);
    return true;
}

void REPLPrint(const std::string& message) {
    std::cout << "\033[2K\r" << WHITE << message << RESET << std::endl;
    std::cout << "> ";
}

std::string GenerateGUID() {
    GUID guid;
    HRESULT hresult = CoCreateGuid(&guid);

    if (SUCCEEDED(hresult)) {
        char buf[64] = { 0 };
        sprintf_s(buf, sizeof(buf),
            "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
            guid.Data1, guid.Data2, guid.Data3,
            guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
            guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
        return std::string(buf);
    }
    return "";

}

bool IsMissingKeys(const json& data, const std::vector<std::string>& keys) {
    for (const auto& key : keys) {
        if (!data.contains(key)) {
            return true;
        }
    }

    return false;
}

std::optional<std::pair<std::string, std::string>> ParseUrl(const std::string& url) {
    auto scheme_pos = url.find("://");
    if (scheme_pos == std::string::npos)
        return std::nullopt;

    auto path_pos = url.find('/', scheme_pos + 3);

    std::string host;
    std::string path;

    if (path_pos == std::string::npos) {
        host = url;
        path = "/";
    }
    else {
        host = url.substr(0, path_pos);
        path = url.substr(path_pos);
    }

    return std::make_pair(host, path);
}

Data ParseData(const json& data) {
    Data request;

    std::string id = data.contains("id") ? data["id"] : "";
    std::string type = data.contains("type") ? data["type"] : "";
    std::string action = data.contains("action") ? data["action"] : "";
    DWORD pid = data.contains("pid") ? (DWORD)data["pid"] : 0;

    request.id = id;
    request.type = type;
    request.action = action;
    request.pid = pid;

    return request;
}
