#pragma once

#include "executor/instances/instance.h"
#include "executor/instances/scriptcontext.h"

class DataModel : public Instance {
public:
	using Instance::Instance;
	DataModel(const Instance& instance);

	bool GameLoaded() const;
};
