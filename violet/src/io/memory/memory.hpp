#pragma once
#include <windows.h>
#include <winternl.h>
#include <string_view>
#include <cstdint>

#include <memory>
#include <optional>
#include <atomic>

extern "C" DWORD NtUserSendInput_SSN;
extern "C" NTSTATUS Hagi_ReadVirtualMemory(HANDLE hProc, PVOID base, PVOID buf, SIZE_T size, PSIZE_T read);
extern "C" NTSTATUS Hagi_WriteVirtualMemory(HANDLE hProc, PVOID base, PVOID buf, SIZE_T size, PSIZE_T written);
extern "C" NTSTATUS Hagi_NtUserSendInput(UINT cInputs, LPINPUT pInputs, int cbSize);

class memory_manager {
public:
    std::shared_ptr<void> process_handle;
    HANDLE cached_handle = nullptr;  // Raw handle cache to avoid shared_ptr refcount per read
    std::atomic<uintptr_t> base_address = 0;

    // Call after attach() or whenever process_handle changes
    void refresh_handle() { cached_handle = process_handle ? process_handle.get() : nullptr; }

    // [memory::read]
    template <typename T>
    auto read(uintptr_t address) const -> T {
        T buffer{};
        if (!address || !cached_handle) return buffer;

        Hagi_ReadVirtualMemory(cached_handle, reinterpret_cast<PVOID>(address), &buffer, sizeof(T), nullptr);
        return buffer;
    }

    // [memory::try_read]
    template <typename T>
    auto try_read(uintptr_t address, T& out) const -> bool {
        if (!address || !cached_handle) return false;

        return Hagi_ReadVirtualMemory(cached_handle, reinterpret_cast<PVOID>(address), &out, sizeof(T), nullptr) == 0;
    }

    // [memory::read_buf]
    auto read_buf(uintptr_t address, void* buf, size_t size) const -> bool {
        if (!address || !buf || !size || !cached_handle) return false;
        return Hagi_ReadVirtualMemory(cached_handle, reinterpret_cast<PVOID>(address), buf, size, nullptr) == 0;
    }

    // [memory::write]
    template <typename T>
    auto write(uintptr_t address, const T& value) const -> bool {
        if (!address || !cached_handle) return false;

        return Hagi_WriteVirtualMemory(cached_handle, reinterpret_cast<PVOID>(address), (PVOID)&value, sizeof(T), nullptr) == 0;
    }

    auto attach(std::wstring_view process_name) -> bool;
};

inline memory_manager mem;
