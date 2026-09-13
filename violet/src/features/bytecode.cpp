#include "bytecode.hpp"
#include "../threads/manager.hpp"
#include "../io/memory/memory.hpp"
#include "../core/logger.hpp"
#include "../exec/executor/websocket.h"
#include "../exec/executor/client.h"

#include <thread>
#include <memory>
#include <exception>

void bytecode::hook()
{
	task::manager::add(HIDE_STR("bytecode_executor"), [](std::stop_token stop) {
		logger::info("bytecode: starting server");
		
		Websocket server("127.0.0.1", 25565);
		if (!server.Run()) {
			logger::error("bytecode: failed to run server");
			return;
		}

		bool last_enabled = false;
		while (!stop.stop_requested() && globals::running) {
			std::this_thread::sleep_for(std::chrono::milliseconds(500));

			bool enabled = globals::bytecode::enabled.load();
			if (enabled && !last_enabled) {
				logger::info("bytecode: toggled on");
				if (mem.process_handle) {
					DWORD pid = GetProcessId(mem.cached_handle);
					if (pid != 0) {
						if (auto client = server.GetClient(pid)) {
							client->Execute("print('violet')");
						}
					}
				}
			}
			last_enabled = enabled;

			if (!enabled) {
				continue;
			}

			if (!mem.process_handle) {
				continue;
			}

			DWORD pid = GetProcessId(mem.cached_handle);
			if (pid == 0) {
				continue;
			}

			if (server.GetClient(pid) != nullptr) {
				continue;
			}

			logger::info("bytecode: injecting...");

			try {
				auto client = std::make_unique<Client>(pid, &server);
				auto [success, error] = client->Inject();
				if (!success) {
					logger::error("bytecode: injection failed");
					continue;
				}

				logger::success("bytecode: injected");
				client->Initialize();
				server.AddClient(std::move(client));
			}
			catch (const std::exception& e) {
				logger::error("bytecode: injection exception");
			}
		}

		server.Stop();
	});
}

