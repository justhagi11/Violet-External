#pragma once

#include "executor/instances/instance.h"

class ObjectValue : public Instance {
public:
	using Instance::Instance;
	ObjectValue(const Instance& instance);

	Instance Value() const;
};
