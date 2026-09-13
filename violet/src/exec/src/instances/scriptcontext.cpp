#include "executor/instances/scriptcontext.h"
#include "game/offsets.hpp"

ScriptContext::ScriptContext(const Instance& instance) : Instance(instance) {}

bool ScriptContext::GetRequireBypass() const {
	if (offsets::ScriptContext::RequireBypass == 0) return false;
	return ReadFrom<uintptr_t>(offsets::ScriptContext::RequireBypass);
}

void ScriptContext::SetRequireBypass(bool value) const {
	if (offsets::ScriptContext::RequireBypass != 0) {
		WriteTo<uintptr_t>(offsets::ScriptContext::RequireBypass, static_cast<int>(value));
	}
}
