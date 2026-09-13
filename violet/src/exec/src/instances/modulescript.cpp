#include "executor/instances/modulescript.h"
#include "game/offsets.hpp"
#include "executor/bytecode.h"
#include "executor/logger.h"

#include <thread>
#include <iostream>

ModuleScript::ModuleScript(const Instance& instance) : Instance(instance) {}

std::string ModuleScript::GetBytecode() const {
	if (!IsValid()) return "";

	uintptr_t embedded = ReadFrom<uintptr_t>(offsets::ModuleScript::Bytecode);

	uintptr_t bytecode_ptr = process->Read<uintptr_t>(embedded + offsets::ByteCode::Pointer);
	uint64_t bytecode_size = process->Read<uint64_t>(embedded + offsets::ByteCode::Size);

	std::string bytecode;
	bytecode.resize(bytecode_size);

	bytecode = process->Read<std::string>(bytecode_ptr, bytecode_size, false);

	return Bytecode::Decompress(bytecode);
}

void ModuleScript::SetBytecode(const std::string& bytecode, bool revert) const {
	if (!IsValid()) return;

	uintptr_t embedded = ReadFrom<uintptr_t>(offsets::ModuleScript::Bytecode);

	LPVOID new_bytecode_ptr = VirtualAllocEx(process->GetHandle(), nullptr, bytecode.size(), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!new_bytecode_ptr)
		return;

	SIZE_T bytes_written;
	if (!WriteProcessMemory(process->GetHandle(), new_bytecode_ptr, bytecode.data(), bytecode.size(), &bytes_written) || bytes_written != bytecode.size()) {
		VirtualFreeEx(process->GetHandle(), new_bytecode_ptr, 0, MEM_RELEASE);
		return;
	}

	if (revert) {
		uintptr_t original_bytecode_ptr = process->Read<uintptr_t>(embedded + offsets::ByteCode::Pointer);
		uint64_t original_bytecode_size = process->Read<uint64_t>(embedded + offsets::ByteCode::Size);

		std::thread([process = this->process, embedded, original_bytecode_ptr, original_bytecode_size]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(850));
			process->Write<uintptr_t>(embedded + offsets::ByteCode::Pointer, original_bytecode_ptr);
			process->Write<uint64_t>(embedded + offsets::ByteCode::Size, original_bytecode_size);
			}).detach();
	}

	process->Write<uintptr_t>(embedded + offsets::ByteCode::Pointer, reinterpret_cast<uintptr_t>(new_bytecode_ptr));
	process->Write<uint64_t>(embedded + offsets::ByteCode::Size, bytecode.size());
}

void ModuleScript::SpoofWith(uintptr_t instance) {
	logger::info("spoof: overwriting 0x8");
	WriteTo<uintptr_t>(0x8, instance);
};
