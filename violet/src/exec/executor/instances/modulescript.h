#pragma once

#include "executor/instances/instance.h"

class ModuleScript : public Instance {
public:
	using Instance::Instance;
	ModuleScript(const Instance& instance);

	std::string GetBytecode() const;
	void SetBytecode(const std::string& bytecode, bool revert = true) const;

	void SpoofWith(uintptr_t instance);
};
