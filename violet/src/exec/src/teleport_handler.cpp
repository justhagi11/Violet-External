#include <string>
#include <thread>
#include <mutex>
#include "executor/logger.h"
#include "executor/teleport_handler.h"
#include "executor/instances/datamodel.h"
#include "executor/utils.h"
#include "game/offsets.hpp"

TeleportHandler::TeleportHandler(const Process* process) : _process(process) {};

void TeleportHandler::AddEvent(TeleportEvent event) {
	std::lock_guard<std::mutex> lock(_event_mutex);
	_events.push_back(event);

	if (_is_game_ready.load()) {
		std::thread([event]() {
			event();
		}).detach();
	}
}

int TeleportHandler::GetEpoch() const {
	return _epoch.load();
}

bool TeleportHandler::IsGameReady() const {
	return _is_game_ready.load();
}

bool TeleportHandler::IsRunning() const {
	return _running;
}

void TeleportHandler::Start() {
	_running = true;

	while (_running) {
		DataModel datamodel(GetDataModel(_process), _process);

		while (_running) {
			uint64_t gl_value = _process->Read<uint64_t>(datamodel.Self() + offsets::DataModel::GameLoaded);

			if (datamodel.GameLoaded()) {
				break;
			}

			datamodel = DataModel(GetDataModel(_process), _process);
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}

		if (!_running)
			break;

		std::vector<TeleportEvent> events;
		{
			std::lock_guard<std::mutex> lock(_event_mutex);
			events = _events;
		}

		for (const auto& event : events) {
			event();
		}
		_epoch.fetch_add(1, std::memory_order_acq_rel);
		_is_game_ready.store(true);

		while (_running) {
			if (!datamodel.GameLoaded())
				break;

			uintptr_t new_datamodel_address = GetDataModel(_process);
			if (new_datamodel_address != datamodel.Self() && new_datamodel_address != 0) {
				logger::info("TeleportHandler: DataModel address changed from 0x{:X} to 0x{:X}. Teleport detected!", (uintptr_t)datamodel.Self(), new_datamodel_address);
				break;
			}

			datamodel = DataModel(new_datamodel_address, _process);
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}

		if (!_running)
			break;

		_is_game_ready.store(false);
		std::this_thread::sleep_for(std::chrono::milliseconds(150));
	}
}

void TeleportHandler::Stop() {
	_running = false;
}
