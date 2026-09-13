#include <Windows.h>
#include "executor/logger.h"
#include <memory>
#include <fstream>
#include <filesystem>

#include "executor/repl.h"
#include "executor/client.h"
#include "executor/utils.h"
#include "executor/process.h"
#include "executor/websocket.h"

REPL::REPL(Websocket* server) : _server(server) {}

void REPL::Run() {
	std::string input;
	std::string path;

	logger::info("Executor's REPL, type exit to quit");

	while (input != "exit") {
		std::cout << "\033[2K\r";
		std::cout << "> ";
		std::getline(std::cin, input);
		std::istringstream iss(input);

		std::string command;
		iss >> command;

		if (command == "inject") {
			DWORD PID;

			auto initialize_client = [&](DWORD _PID) {
				try {
					auto client = std::make_unique<Client>(_PID, _server);
					client->OnError([&](const std::string& error) {
						REPLPrint(error);
						});
					client->Initialize();
					_server->AddClient(std::move(client));
					REPLPrint("Injected client " + std::to_string(_PID) + " successfully");

				}
				catch (const std::exception& exception) {
					REPLPrint("Failed to initialize client " + std::to_string(_PID));
					REPLPrint(std::string(exception.what()));
				}
			};

			if (iss >> PID) {
				initialize_client(PID);
			} else {
				std::vector<DWORD> PIDs = GetProcessIds(L"RobloxPlayerBeta.exe");
				if (PIDs.empty()) {
					REPLPrint("Could not find any roblox process");
					continue;
				}

				for (const auto& _PID : PIDs) {
					Client* client = _server->GetClient(_PID);

					if (client) continue;
					initialize_client(_PID);
				}
			}
		}
		else if (command == "select") {
			std::string raw_path;
			std::getline(iss >> std::ws, raw_path);

			if (!raw_path.empty()) {

				if (raw_path.front() == '"' && raw_path.back() == '"') {
					raw_path = raw_path.substr(1, raw_path.length() - 2);
				}

				std::filesystem::path fs_path(raw_path);
				path = raw_path;

				if (!std::filesystem::exists(fs_path) && !std::filesystem::is_regular_file(fs_path)) {
					REPLPrint("Invalid path");
					continue;
				}

				REPLPrint(fs_path.filename().string() + " has been selected");

			}
			else {
				REPLPrint("No path was specified");
			}
		}
		else if (command == "execute") {
			std::ifstream file(path);

			if (!file.is_open()) {
				REPLPrint("Failed to open file");
				continue;

			};

			std::stringstream buffer;
			buffer << file.rdbuf();

			DWORD PID;

			if (iss >> PID) {
				for (const auto& client : _server->GetClients()) {
					if (client->GetProcess()->GetProcessId() == PID) {
						auto [success, error] = client->Execute(buffer.str());

						if (!success) {
							REPLPrint(error);
							break;
						}

						REPLPrint("Executed successfully for client " + std::to_string(PID));
					}
				}
			}
			else {
				for (const auto& client : _server->GetClients()) {
					auto [success, error] = client->Execute(buffer.str());

					if (!success) {
						REPLPrint(error);
						break;
					}

					REPLPrint("Executed successfully for client " + std::to_string(client->GetProcess()->GetProcessId()));
				}
			}
		}
	}

	std::cout << "\033[2K\r";
}
