#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <memory>

#include "executor/process.h"
#include "executor/teleport_handler.h"

class Websocket;

using OnErrorCallback = std::function<void(const std::string&)>;

class Client {
private:
	Process _process;
	Websocket* _server;
	TeleportHandler _teleport_handler;

	std::mutex _teleport_mutex;
	std::vector<std::shared_ptr<std::string>> _teleport_queue;

	std::mutex _error_mutex;
	OnErrorCallback _on_error;

	static constexpr std::string_view _executor_name = "miku-bytecode";
	static constexpr std::string_view _executor_version = "1.1.1";
public:
	Client(DWORD PID, Websocket* server);

	const Process* GetProcess() const;
	std::pair<bool, std::string> GetInitScript() const;

	std::string GetExecutorName() const;
	std::string GetExecutorVersion() const;

	void OnError(OnErrorCallback callback);
	void PushError(const std::string& message);

	void Initialize();
	void Shutdown();

	std::pair<bool, std::string> Inject() const;
	std::pair<bool, std::string> Execute(const std::string& source) const;
	void QueueOnTeleport(const std::string& source);
};

using Clients = std::vector<std::unique_ptr<Client>>;
