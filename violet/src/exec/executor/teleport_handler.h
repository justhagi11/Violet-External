#pragma once

#include <mutex>
#include <atomic>
#include <functional>

#include "executor/process.h"

using TeleportEvent = std::function<void()>;

class TeleportHandler {
private:
	std::atomic<bool> _is_game_ready = false;
	std::atomic<int> _epoch = 0;
	bool _running = false;
	const Process* _process;

	std::mutex _event_mutex;
	std::vector<TeleportEvent> _events;
public:
	TeleportHandler(const Process* process);

	void AddEvent(TeleportEvent event);

	int GetEpoch() const;

	bool IsGameReady() const;
	bool IsRunning() const;

	void Start();
	void Stop();
};
