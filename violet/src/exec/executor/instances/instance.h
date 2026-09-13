#pragma once

#include <string>
#include <vector>

#include "executor/process.h"

class Instance {
public:
	const Process* process = nullptr;
	uintptr_t address = 0;

	Instance() = default;
	Instance(uintptr_t address, const Process* process);
	Instance(const Instance& instance);

	template <typename T, typename... Params>
	T ReadFrom(uintptr_t offset, Params... params) const {
		return process->Read<T>(address + offset, std::forward<Params>(params)...);
	}

	template <typename T, typename... Params>
	bool WriteTo(uintptr_t offset, T buffer, Params... params) const {
		return process->Write<T>(address + offset, buffer, std::forward<Params>(params)...);
	}

	uintptr_t Self() const;
	std::string Name() const;
	std::string ClassName() const;
	Instance Parent() const;

	std::vector<Instance> GetChildren() const;
	std::vector<Instance> GetDescendants() const;

	Instance FindFirstChild(const std::string_view name) const;
	Instance FindFirstChildFromPath(const std::string_view path) const;

	Instance FindFirstDescendant(const std::string_view name) const;

	Instance FindFirstChildOfClass(const std::string_view classname) const;
	Instance FindFirstChildWhichIsA(const std::string_view classname) const;

	bool IsValid() const;
	bool IsA(const std::string_view classname) const;
};
