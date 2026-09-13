#pragma once

#include "executor/instances/instance.h"

class LocalScript : public Instance {
public:
	using Instance::Instance;
	LocalScript(const Instance& instance);

	std::string GetBytecode() const;
	void SetBytecode(const std::string& bytecode, bool revert = true) const;
};
