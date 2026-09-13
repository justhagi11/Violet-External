#include "memory.hpp"
#include <TlHelp32.h>
#include "../../deps/lazy_importer.hpp"

extern "C" DWORD NtUserSendInput_SSN = 0x107B;

// [memory::attach]
auto memory_manager::attach(std::wstring_view process_name) -> bool {
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(PROCESSENTRY32W);

    const auto snapshot = LI_FN(CreateToolhelp32Snapshot)(TH32CS_SNAPPROCESS, NULL);
    if (snapshot == INVALID_HANDLE_VALUE) return false;

    if (LI_FN(Process32FirstW)(snapshot, &entry)) {
        do {
            if (process_name == entry.szExeFile) {
                HANDLE hProc = LI_FN(OpenProcess)(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, FALSE, entry.th32ProcessID);
                if (!hProc || hProc == INVALID_HANDLE_VALUE) continue;

                uintptr_t base = 0;
                const auto mod_snap = LI_FN(CreateToolhelp32Snapshot)(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, entry.th32ProcessID);
                if (mod_snap != INVALID_HANDLE_VALUE) {
                    MODULEENTRY32W mod_entry;
                    mod_entry.dwSize = sizeof(mod_entry);
                    if (LI_FN(Module32FirstW)(mod_snap, &mod_entry)) {
                        base = reinterpret_cast<uintptr_t>(mod_entry.modBaseAddr);
                    }
                    LI_FN(CloseHandle)(mod_snap);
                }
                
                if (base != 0) {
                    DWORD exit_code = 0;
                    if (GetExitCodeProcess(hProc, &exit_code) && exit_code == STILL_ACTIVE) {
                        this->process_handle = std::shared_ptr<void>(hProc, LI_FN(CloseHandle));
                        this->base_address = base;
                        this->refresh_handle();
                        LI_FN(CloseHandle)(snapshot);
                        return true;
                    }
                }
                
                LI_FN(CloseHandle)(hProc);
            }
        } while (LI_FN(Process32NextW)(snapshot, &entry));
    }

    LI_FN(CloseHandle)(snapshot);
    return false;
}
