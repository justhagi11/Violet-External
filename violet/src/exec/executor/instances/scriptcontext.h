#pragma once

#include "executor/instances/instance.h"

class ScriptContext : public Instance {
public:
	using Instance::Instance;
	ScriptContext(const Instance& instance);

	bool GetRequireBypass() const;
	void SetRequireBypass(bool value) const;
};
