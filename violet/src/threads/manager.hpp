#pragma once

// Library Includes. ;
	#include <thread>
	#include <vector>
	#include <string>
	#include <functional>
	#include <map>
	#include <mutex>

// Internal Includes. ;
	#include "../core/logger.hpp"
	#include "../security/obfuscator.hpp"

namespace task
	// Modern jthread wrapper.
{

	struct thread_entry

	{
		std::string name;
		std::jthread handle;
	};

	class manager
	{
	private:
		inline static std::map<std::string, std::jthread> threads;
		inline static std::mutex mtx;

	public:
		// [task::add]
		static auto add(const std::string& name, std::function<void(std::stop_token)> func) -> void {
			std::lock_guard<std::mutex> lock(mtx);
			threads[name] = std::jthread(func);
		}

		// jthreads stop; after user clicks eject.
		// [task::stop_all]
		static auto stop_all() -> void {
			std::lock_guard<std::mutex> lock(mtx);
			threads.clear();
		}
	};
}
