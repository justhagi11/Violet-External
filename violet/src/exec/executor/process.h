#pragma once

#include <Windows.h>
#include <string>
#include <vector>

std::vector<DWORD> GetProcessIds(const wchar_t* name);
uintptr_t GetBaseAddress(DWORD PID);
HWND GetWindowFromProcessId(DWORD PID);
void EnableVirtualTerminal();

class Process {
private:
    DWORD _PID;
    HANDLE _handle;
    uintptr_t _base_address;
public:
    Process(DWORD PID);

    uintptr_t GetBaseAddress() const noexcept;
    HANDLE GetHandle() const noexcept;
    DWORD GetProcessId() const noexcept;

    template <typename T>
    T Read(uintptr_t address, uint64_t size = NULL, bool convert = true) const {
        T buffer{};
        ReadProcessMemory(_handle, reinterpret_cast<LPCVOID>(address), &buffer, sizeof(buffer), nullptr);
        return buffer;
    }

    template <>
    inline std::string Read(uintptr_t address, uint64_t size, bool convert) const {
        uint64_t length = size ? size : Read<uint64_t>(address + 0x10);

        if (length == 0 || length > 0xFFFF)
            return "";

        if (convert)
            address = (length >= 16) ? Read<uintptr_t>(address) : address;

        if (!address)
            return "";

        std::string buffer;
        buffer.resize(length);

        ReadProcessMemory(_handle, reinterpret_cast<LPCVOID>(address), buffer.data(), length, nullptr);

        return buffer;
    }

    template <typename T>
    bool Write(uintptr_t address, T buffer, uintptr_t size_ptr = NULL) const {
        return WriteProcessMemory(_handle, reinterpret_cast<LPVOID>(address), &buffer, sizeof(buffer), nullptr);
    }

    template <>
    inline bool Write<std::string>(uintptr_t address, std::string buffer, uintptr_t size_ptr) const {
        size_ptr = size_ptr ? size_ptr : address + 0x10;

        address = (Read<uint64_t>(size_ptr) >= 16) ? Read<uintptr_t>(address) : address;

        size_t buffer_size = buffer.size();

        return WriteProcessMemory(_handle, reinterpret_cast<LPVOID>(address), buffer.c_str(), buffer_size, nullptr) &&
            WriteProcessMemory(_handle, reinterpret_cast<LPVOID>(size_ptr), &buffer_size, sizeof(buffer_size), nullptr);
    }

    bool ReadMemory(uintptr_t address, void* buffer, size_t size) const {
        return ReadProcessMemory(_handle, reinterpret_cast<LPCVOID>(address), buffer, size, nullptr);
    }

    bool WriteMemory(uintptr_t address, const void* buffer, size_t size) const {
        return WriteProcessMemory(_handle, reinterpret_cast<LPVOID>(address), buffer, size, nullptr);
    }
};
