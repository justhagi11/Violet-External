#include <chrono>
#include "executor/logger.h"

#include "nlohmann/json.hpp"
using nlohmann::json;

#include "ixwebsocket/IXBase64.h"

#include "executor/client.h"
#include "executor/process.h"
#include "../../game/offsets.hpp"
#include "executor/utils.h"
#include "executor/websocket.h"
#include "executor/bytecode.h"
#include "../initscript.hpp"

#include "executor/instances/instance.h"
#include "executor/instances/datamodel.h"
#include "executor/instances/modulescript.h"

Client::Client(DWORD PID, Websocket* server) : _process(PID), _server(server), _teleport_handler(&_process) {}

const Process* Client::GetProcess() const {
	return &_process;
}

std::string Client::GetExecutorName() const {
	return std::string(_executor_name);
}

std::string Client::GetExecutorVersion() const {
	return std::string(_executor_version);
}

std::pair<bool, std::string> Client::GetInitScript() const {
	std::string initscript = std::string(initscript_data);

	if (!ReplaceString(initscript, "%HOST%", "\"" + _server->GetHost() + "\""))
		return { false, "Failed to set host" };

	if (!ReplaceString(initscript, "%PORT%", std::to_string(_server->GetPort())))
		return { false, "Failed to set port" };

	if (!ReplaceString(initscript, "%PROCESS_ID%", std::to_string(_process.GetProcessId())))
		return { false, "Failed to set process id" };

	if (!ReplaceString(initscript, "%EXECUTOR_NAME%", "\"" + std::string(_executor_name) + "\""))
		return { false, "Failed to set executor name" };

	if (!ReplaceString(initscript, "%EXECUTOR_VERSION%", "\"" + std::string(_executor_version) + "\""))
		return { false, "Failed to set executor version" };

	return { true, initscript };
}

void Client::OnError(OnErrorCallback callback) {
	std::lock_guard lock(_error_mutex);
	_on_error = std::move(callback);
}

void Client::PushError(const std::string& message) {
	std::function<void(const std::string&)> callback;

	{
		std::lock_guard lock(_error_mutex);
		if (!_on_error)
			return;

		callback = _on_error;
	}

	if (callback) {
		callback(message);
	}
}

void Client::Initialize() {
	_teleport_handler.AddEvent([this]() {
		auto [success, error] = Inject();
		if (!success) {
			PushError(error);
		}
		});

	_teleport_handler.AddEvent([this]() {
		std::vector<std::shared_ptr<std::string>> teleport_queue;

		{
			std::lock_guard lock(_teleport_mutex);
			teleport_queue = _teleport_queue;
		}

		for (const auto& script : teleport_queue) {
			auto [success, error] = Execute(*script);
			if (!success) {
				PushError(error);
			}
		}
		});

	std::thread(&TeleportHandler::Start, &_teleport_handler).detach();
}

void Client::Shutdown() {
	_teleport_handler.Stop();
}

std::pair<bool, std::string> Client::Inject() const {

	DataModel datamodel(GetDataModel(GetProcess()), GetProcess());

	if (!datamodel.IsValid()) {
		return { false, "Failed to get datamodel" };
	}

	ScriptContext scriptcontext = datamodel.FindFirstChildOfClass("ScriptContext");
	if (scriptcontext.IsValid()) {
		scriptcontext.SetRequireBypass(true);
	}
	if (offsets::FFlags::WebSocketServiceEnableClientCreation != 0) {
		_process.Write<uint8_t>(_process.GetBaseAddress() + offsets::FFlags::WebSocketServiceEnableClientCreation, 1);
	}
	if (offsets::FFlags::EnableExternalWebSockets != 0) {
		_process.Write<uint8_t>(_process.GetBaseAddress() + offsets::FFlags::EnableExternalWebSockets, 1);
	}
	if (offsets::FFlags::EnableWebSockets != 0) {
		_process.Write<uint8_t>(_process.GetBaseAddress() + offsets::FFlags::EnableWebSockets, 1);
	}

	bool already_in_memory = datamodel.FindFirstChildOfClass("CorePackages").FindFirstChild(_executor_name).IsValid();
	bool is_connected = _server->GetConnection(_process.GetProcessId()) != nullptr;

	if (already_in_memory && is_connected) {
		return { true, "" };
	}

	if (already_in_memory) {
		for (int i = 0; i < 30; i++) {
			if (_server->GetConnection(_process.GetProcessId())) {
				if (scriptcontext.IsValid()) scriptcontext.SetRequireBypass(false);
				if (offsets::FFlags::WebSocketServiceEnableClientCreation != 0) {
					_process.Write<uint8_t>(_process.GetBaseAddress() + offsets::FFlags::WebSocketServiceEnableClientCreation, 0);
				}
				if (offsets::FFlags::EnableExternalWebSockets != 0) {
					_process.Write<uint8_t>(_process.GetBaseAddress() + offsets::FFlags::EnableExternalWebSockets, 0);
				}
				if (offsets::FFlags::EnableWebSockets != 0) {
					_process.Write<uint8_t>(_process.GetBaseAddress() + offsets::FFlags::EnableWebSockets, 0);
				}
				return { true, "" };
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

	}

	ModuleScript vrnavigation;
	for (int i = 0; i < 20; i++) {
		Instance starter_player = datamodel.FindFirstChildOfClass("StarterPlayer");
		if (starter_player.IsValid()) {
			Instance scripts = starter_player.FindFirstChild("StarterPlayerScripts");
			if (scripts.IsValid()) {
				vrnavigation = scripts.FindFirstDescendant("VRNavigation");
				if (vrnavigation.IsValid()) break;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(250));
	}

	if (!vrnavigation.IsValid()) {
		return { false, "Failed to find VRNavigation" };
	}

	ModuleScript playerlistmanager;
	for (int i = 0; i < 20; i++) {
		Instance core_gui = datamodel.FindFirstChildOfClass("CoreGui");
		if (core_gui.IsValid()) {
			playerlistmanager = core_gui.FindFirstChildFromPath("RobloxGui.Modules.PlayerList.PlayerListManager");
			if (playerlistmanager.IsValid()) break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(250));
	}

	if (!playerlistmanager.IsValid()) {
		return { false, "Failed to find PlayerListManager" };
	}

	auto [initscript_success, initscript_or_error] = GetInitScript();
	if (!initscript_success) {
		return { false, initscript_or_error };
	}

	std::string script = "task.spawn(function() local success, err = xpcall(function()\n" + initscript_or_error + "\nend, warn); if not success then warn('Init Error:', err) end end); print('violet'); task.wait(1.5); return require(game:GetService('CoreGui'):FindFirstChild('RobloxGui', true):FindFirstChild('Modules', true):FindFirstChild('PlayerList', true):FindFirstChild('PlayerListManager', true))";

	auto [compile_success, bytecode_or_error] = Bytecode::Compile(script);
	if (!compile_success) {
		return { false, "Failed to compile initscript\n" + bytecode_or_error };
	}

	vrnavigation.SetBytecode(bytecode_or_error);
	playerlistmanager.SpoofWith(vrnavigation.Self());

	HWND window = GetWindowFromProcessId(_process.GetProcessId());
	SetForegroundWindow(window);
	while (GetForegroundWindow() != window) {
		SetForegroundWindow(window);
		std::this_thread::sleep_for(std::chrono::microseconds(100));
	}

	INPUT inputs[2] = {};
	inputs[0].type = INPUT_KEYBOARD; inputs[0].ki.wVk = VK_ESCAPE; inputs[0].ki.wScan = MapVirtualKey(VK_ESCAPE, 0);
	inputs[1].type = INPUT_KEYBOARD; inputs[1].ki.wVk = VK_ESCAPE; inputs[1].ki.wScan = MapVirtualKey(VK_ESCAPE, 0); inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));

	std::this_thread::sleep_for(std::chrono::milliseconds(800));
	playerlistmanager.SpoofWith(playerlistmanager.Self());

	std::this_thread::sleep_for(std::chrono::milliseconds(1200));
	scriptcontext.SetRequireBypass(false);
	if (offsets::FFlags::WebSocketServiceEnableClientCreation != 0) {
		_process.Write<uint8_t>(_process.GetBaseAddress() + offsets::FFlags::WebSocketServiceEnableClientCreation, 0);
	}
	if (offsets::FFlags::EnableExternalWebSockets != 0) {
		_process.Write<uint8_t>(_process.GetBaseAddress() + offsets::FFlags::EnableExternalWebSockets, 0);
	}
	if (offsets::FFlags::EnableWebSockets != 0) {
		_process.Write<uint8_t>(_process.GetBaseAddress() + offsets::FFlags::EnableWebSockets, 0);
	}

	for (int i = 0; i < 50; i++) {
		if (_server->GetConnection(_process.GetProcessId())) {
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

	return { true, "" };
}

std::pair<bool, std::string> Client::Execute(const std::string& source) const {
	auto [compile_success, bytecode_or_error] = Bytecode::Compile("local function __miku_bytecode_wrapper(...) " + source + "\nend; return __miku_bytecode_wrapper");
	if (!compile_success) return { false, bytecode_or_error };

	json request;
	request["action"] = "spawn_module";

	auto spawn_response = _server->SendAndReceive(request, _process.GetProcessId());
	if (!spawn_response) return { false, "Timeout waiting for response (Client not connected?)" };
	if (!spawn_response.value()["success"]) return { false, spawn_response.value()["message"].get<std::string>() };

	const std::string& module_name = spawn_response.value()["module_name"];
	DataModel datamodel(GetDataModel(GetProcess()), GetProcess());
	if (!datamodel.IsValid()) return { false, "Failed to get datamodel" };

	ModuleScript module(datamodel.FindFirstChildOfClass("CorePackages").FindFirstChildFromPath(GetExecutorName() + ".Scripts." + module_name));
	if (!module.IsValid()) return { false, "Failed to find module" };

	module.SetBytecode(bytecode_or_error);

	ScriptContext scriptcontext = datamodel.FindFirstChildOfClass("ScriptContext");
	if (scriptcontext.IsValid()) {
		scriptcontext.SetRequireBypass(true);
	}

	request["action"] = "load_module";
	request["module_name"] = module_name;

	auto load_response = _server->SendAndReceive(request, _process.GetProcessId());

	if (scriptcontext.IsValid()) {
		scriptcontext.SetRequireBypass(false);
	}

	if (!load_response) return { false, "Timeout waiting for response (Client disconnected?)" };
	if (!load_response.value()["success"]) return { false, load_response.value()["message"].get<std::string>() };

	return { true, "" };
}

void Client::QueueOnTeleport(const std::string& source) {
	auto ptr = std::make_shared<std::string>(source);
	std::lock_guard lock(_teleport_mutex);
	_teleport_queue.push_back(ptr);
}
