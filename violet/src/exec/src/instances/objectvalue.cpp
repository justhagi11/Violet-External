#include "executor/instances/objectvalue.h"
#include "game/offsets.hpp"

ObjectValue::ObjectValue(const Instance& instance) : Instance(instance) {}

Instance ObjectValue::Value() const {
	if (!IsValid()) return Instance();

	if (ClassName() != "ObjectValue")
		return Instance();

	return Instance(ReadFrom<uintptr_t>(offsets::Value::Value), process);
}
