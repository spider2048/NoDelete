#include <common.h>
#include <winapi_helper.h>


namespace inject {
    std::string cwd;
    std::string dll_name;
    uint64_t Off_LoadLibraryA;
    uint64_t Off_FreeLibAndExit;

    void init() {
        winapi::get_cwd(cwd);
        dll_name = cwd + TARGET_DLL;

        HMODULE curr_kernel32_base = winapi::get_module_base_address("kernel32.dll");
        Off_LoadLibraryA = winapi::get_func_offset(curr_kernel32_base, "LoadLibraryA");
        Off_FreeLibAndExit = winapi::get_func_offset(curr_kernel32_base, "FreeLibraryAndExitThread");

        DEBUG("DLL Name: {}", dll_name)
    }

    std::vector<DWORD> get_explorer_pids() {
        std::vector<DWORD> pidlist;

        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            DEBUG("hSnapShot is invalid");
            return pidlist;
        }

        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(PROCESSENTRY32);

        if (!Process32First(hSnapshot, &processEntry)) {
            CloseHandle(hSnapshot);
            DEBUG("Process32First failed");
            return pidlist;
        }

        do {
            HANDLE hProcess = winapi::remote::open(processEntry.th32ProcessID);
            if (hProcess == 0) continue;

            std::string fname;
            winapi::get_process_name(hProcess, fname);

            if (fname.find("C:\\Windows\\explorer.exe") != -1) {
                pidlist.push_back(processEntry.th32ProcessID);
            }

            winapi::remote::close(hProcess);
        } while (Process32Next(hSnapshot, &processEntry));
        
        CloseHandle(hSnapshot);
        return pidlist;
    }

    void inject_to_pid(DWORD pid) {
        DEBUG("Injecting into: {}", pid);
        HANDLE hProcess = winapi::remote::open(pid);
        
        void* remote_mem = winapi::remote::alloc(hProcess, dll_name.size());
        winapi::remote::write(hProcess, remote_mem, W(dll_name), dll_name.size());

        HMODULE remote_kernel32 = winapi::find_module_by_name(hProcess, "kernel32.dll");
        void* remote_loadlib = (void*) (Off_LoadLibraryA + (uint64_t)remote_kernel32);

        DEBUG("Creating remote thread for LoadLibraryA");
        HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)remote_loadlib, remote_mem, NULL, NULL);
        if (!hRemoteThread) {
            CRITICAL("CreateRemoteThreaad into hProcess:{} failed!", hProcess);
        }

        WaitForSingleObject(hRemoteThread, INFINITE);
        winapi::remote::free(hProcess, remote_mem);
        winapi::remote::close(hProcess);

        DEBUG("Done injecting into PID:{}", pid);
    }

    void eject_from_pid(DWORD pid) {
    }
};
