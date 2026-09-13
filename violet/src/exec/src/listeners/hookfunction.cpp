#include <iostream>
#include "executor/websocket.h"
#include "executor/listeners/hookfunction.h"
#include "executor/client.h"
#include "executor/logger.h"

std::string hookfunction_listener::GetAction() const {
	return "hookfunction";
}

void hookfunction_listener::Callback(ix::WebSocket& websocket, const ix::WebSocketMessagePtr&, const json& data) {
	json response;
	response["type"] = "response";
	response["success"] = false;

	if (data.contains("id")) response["id"] = data["id"];

	if (!data.contains("target") || !data.contains("hook") || !data.contains("pid")) {
		response["message"] = "missing fields";
		websocket.send(response.dump());
		return;
	}

	uintptr_t target_addr = std::stoull(data["target"].get<std::string>(), nullptr, 16);
	uintptr_t hook_addr = std::stoull(data["hook"].get<std::string>(), nullptr, 16);
	DWORD pid = data.contains("pid") ? data["pid"].get<DWORD>() : 0;

	Client* client = _server->GetClient(pid);
	if (!client) return;
	const Process* proc = client->GetProcess();

	uint8_t target_tt = 0;
	proc->ReadMemory(target_addr + 0x08, &target_tt, 1);
	uint8_t hook_tt = 0;
	proc->ReadMemory(hook_addr + 0x08, &hook_tt, 1);

	if (target_tt != hook_tt) {
		response["message"] = "closure type mismatch";
		websocket.send(response.dump());
		return;
	}

	uint8_t nup = 0, ss = 0, pre = 0;
	uintptr_t env = 0, func_ptr = 0;

	proc->ReadMemory(hook_addr + 0x0A, &nup, 1);
	proc->ReadMemory(hook_addr + 0x0B, &ss, 1);
	proc->ReadMemory(hook_addr + 0x0C, &pre, 1);
	proc->ReadMemory(hook_addr + 0x10, &env, 8);
	proc->ReadMemory(hook_addr + 0x18, &func_ptr, 8);

	uintptr_t raw_ptr = func_ptr ^ (hook_addr + 0x18);

	uintptr_t new_ptr = raw_ptr ^ (target_addr + 0x18);

	DWORD oldProtect;
	VirtualProtectEx(proc->GetHandle(), (LPVOID)target_addr, 0x100, PAGE_EXECUTE_READWRITE, &oldProtect);

	proc->WriteMemory(target_addr + 0x0A, &nup, 1);
	proc->WriteMemory(target_addr + 0x0B, &ss, 1);
	proc->WriteMemory(target_addr + 0x0C, &pre, 1);
	proc->WriteMemory(target_addr + 0x10, &env, 8);
	proc->WriteMemory(target_addr + 0x18, &new_ptr, 8);

	if (target_tt == 6) {
		for (int i = 0; i < nup; ++i) {
			uintptr_t upval = 0;
			proc->ReadMemory(hook_addr + 0x20 + (i * 8), &upval, 8);
			proc->WriteMemory(target_addr + 0x20 + (i * 8), &upval, 8);
		}
	} else if (target_tt == 7) {
		for (int i = 0; i < nup; ++i) {
			char buffer[16];
			proc->ReadMemory(hook_addr + 0x20 + (i * 16), buffer, 16);
			proc->WriteMemory(target_addr + 0x20 + (i * 16), buffer, 16);
		}
	}

	VirtualProtectEx(proc->GetHandle(), (LPVOID)target_addr, 0x100, oldProtect, &oldProtect);

	response["success"] = true;
	logger::info("hookfunction: 0x{:X} detoured to 0x{:X}", target_addr, hook_addr);
	websocket.send(response.dump());
}
