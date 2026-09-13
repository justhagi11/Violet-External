#pragma once

#include <Windows.h>
#include <string>

#include "nlohmann/json.hpp"
using nlohmann::json;

#include "executor/process.h"

uintptr_t GetDataModel(const Process* process);
HMODULE GetModule();

bool ReplaceString(std::string& data, const std::string_view replace, const std::string_view replacement);
void REPLPrint(const std::string& message);
std::string GenerateGUID();
bool IsMissingKeys(const json& data, const std::vector<std::string>& keys);
std::optional<std::pair<std::string, std::string>> ParseUrl(const std::string& url);

struct Data {
	std::string id;
	std::string type;
	std::string action;
	DWORD pid;

	bool has_id()   const noexcept { return !id.empty(); }
	bool is_response() const noexcept { return type == "response"; }
};
Data ParseData(const json& data);
