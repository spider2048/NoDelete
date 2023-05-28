#include <winapi_helper.h>
namespace fs = std::filesystem;

#define CASE(X)      \
    case X:          \
        err += (#X); \
        break;

namespace winapi {
    LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS *ex) {
        std::string err;
        switch (ex->ExceptionRecord->ExceptionCode) {
            CASE(EXCEPTION_ACCESS_VIOLATION)
            CASE(EXCEPTION_ARRAY_BOUNDS_EXCEEDED)
            CASE(EXCEPTION_BREAKPOINT)
            CASE(EXCEPTION_DATATYPE_MISALIGNMENT)
            CASE(EXCEPTION_FLT_DENORMAL_OPERAND)
            CASE(EXCEPTION_FLT_DIVIDE_BY_ZERO)
            CASE(EXCEPTION_FLT_INEXACT_RESULT)
            CASE(EXCEPTION_FLT_INVALID_OPERATION)
            CASE(EXCEPTION_FLT_OVERFLOW)
            CASE(EXCEPTION_FLT_STACK_CHECK)
            CASE(EXCEPTION_FLT_UNDERFLOW)
            CASE(EXCEPTION_ILLEGAL_INSTRUCTION)
            CASE(EXCEPTION_IN_PAGE_ERROR)
            CASE(EXCEPTION_INT_DIVIDE_BY_ZERO)
            CASE(EXCEPTION_INT_OVERFLOW)
            CASE(EXCEPTION_INVALID_DISPOSITION)
            CASE(EXCEPTION_NONCONTINUABLE_EXCEPTION)
            CASE(EXCEPTION_PRIV_INSTRUCTION)
            CASE(EXCEPTION_SINGLE_STEP)
            CASE(EXCEPTION_STACK_OVERFLOW)
            default:
                err += "CUSTOM ERROR";
        }

        auto rip = ex->ContextRecord->Rip;
        auto base = winapi::get_module_handle(winapi::get_process_name(GetCurrentProcess()));

        std::string errmsg = fmt::format("critical exception {:x} occured at rip={} base={} in pid:{}", err, rip, (void *)base, GetCurrentProcessId());
        MessageBoxA(NULL, errmsg.c_str(), "Hook critical error", MB_OK | MB_ICONERROR);

        return EXCEPTION_CONTINUE_SEARCH;
    }

    HMODULE load_library_as_datafile(const std::string &lib) {
        HMODULE ret = LoadLibraryExA(lib.c_str(), NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (ret == nullptr) {
            CRITICAL("LoadLibraryExA failed for:{}", lib);
        }

        return ret;
    }

    std::filesystem::path get_windows_dir() {
        std::string dst;
        dst.resize(MAX_PATH);
        auto sz = GetWindowsDirectoryA(W(dst), MAX_PATH);
        if (sz == 0) {
            CRITICAL("GetWindowsDirectoryA failed")
        }

        dst[sz] = '\\';
        dst.resize(1 + sz);
        return dst;
    }

    std::string get_process_name(HANDLE hProcess) {
        std::string dst;
        dst.resize(MAX_PATH);

        DWORD sz = MAX_PATH;
        if (!QueryFullProcessImageNameA(hProcess, NULL, W(dst), &sz)) {
            CRITICAL("QueryFullProcessImageNameA into hProcess:{} failed!", (void *)hProcess)
        }

        dst.resize(sz);
        return dst;
    }

    void get_process_base_name(HANDLE hProcess, std::string &dst) {
        dst.clear();
        dst.resize(MAX_PATH);

        if (!GetModuleBaseNameA(hProcess, NULL, W(dst), MAX_PATH)) {
            CRITICAL("GetModuleBaseNameA into hProcess:{} failed!", (void *)hProcess)
        }

        DWORD sz = strlen(W(dst));
        dst.resize(sz + 1);
    }

    uint64_t get_func_offset(HMODULE hModule, const std::string &func) {
        uint64_t loaded_addr = (uint64_t)GetProcAddress(hModule, func.c_str());
        if (loaded_addr == 0) {
            CRITICAL("GetProcAddress with func:{} failed!", func);
        }

        return loaded_addr - (uint64_t)hModule;
    }

    HMODULE get_module_handle(const std::string &module) {
        HMODULE ret = GetModuleHandleA(module.c_str());
        if (ret == nullptr)
            CRITICAL("GetModuleBaseAddress with module:{} failed!", module)

        return ret;
    }

    std::string get_module_file_name(HMODULE hModule) {
        std::string dst;
        dst.resize(MAX_PATH);
        auto sz = GetModuleFileNameA(hModule, W(dst), MAX_PATH);
        if (sz < 0) {
            CRITICAL("GetModuleFileNameA for hModule:{} failed!", (void *)hModule)
        }

        dst.resize(sz);
        return dst;
    }

    HMODULE find_module_by_name(HANDLE hProcess, const std::string &modname) {
        DWORD modcnt = 0;
        if (!EnumProcessModules(hProcess, NULL, NULL, &modcnt)) {
            CRITICAL("EnumProcessModules[1] into hProcess:{} failed!", (void *)hProcess);
        }

        std::vector<HMODULE> process_modules(modcnt);
        if (!EnumProcessModules(hProcess, process_modules.data(), modcnt, &modcnt)) {
            CRITICAL("EnumProcessModules[2] into hProcess:{} failed!", (void *)hProcess);
        }

        for (auto hModule : process_modules) {
            std::string basename;
            basename.resize(MAX_PATH);

            if (!GetModuleBaseName(hProcess, hModule, W(basename), MAX_PATH)) {
                DEBUG("GetModuleBaseName for process:{} module:{} failed!", (void *)hProcess, (void *)hModule)
            }

            lowercase(basename);
            if (basename.find(modname) != -1) {
                return hModule;
            }
        }

        return nullptr;
    }

    std::filesystem::path get_cwd() {
        std::string dir;
        dir.resize(MAX_PATH);
        DWORD sz = GetCurrentDirectoryA(MAX_PATH, W(dir));
        if (sz == 0) {
            CRITICAL("GetCurrentDirectoryA failed");
        }

        dir.resize(sz);
        return fs::path(dir);
    }

    namespace remote {
        HANDLE open(DWORD pid) {
            HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
            if (hProcess == INVALID_HANDLE_VALUE) {
                CRITICAL("OpenProcess pid:{} failed", pid)
            }

            return hProcess;
        }

        void *alloc(HANDLE hProcess, size_t size) {
            void *ret = VirtualAllocEx(hProcess, NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (ret == NULL) {
                CRITICAL("VirtualAllocEx into hProcess:{} failed!", (void *)hProcess)
            }

            return ret;
        }

        void free(HANDLE hProcess, void *base) {
            if (!VirtualFreeEx(hProcess, base, NULL, MEM_RELEASE)) {
                CRITICAL("VirtualFreeEx into hProcess:{} failed!", (void *)hProcess)
            }
        }

        void write(HANDLE hProcess, void *dst, void *src, size_t sz) {
            size_t written = 0;
            if (!WriteProcessMemory(hProcess, dst, src, sz, &written)) {
                CRITICAL("WriteProcessMemory into hProcess:{} from:{} to:{} failed!", (void *)hProcess, src, dst)
            }

            if (written != sz) {
                DEBUG("WriteProcessMemory {}/{} bytes were written", written, sz)
            }
        }

        void close(HANDLE hProcess) { CloseHandle(hProcess); }
    };  // namespace remote
};      // namespace winapi

namespace shell {
    void get_files_from_do(IDataObject *pDataObject, std::vector<fs::path> &dst) {
        FORMATETC formatEtc = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        STGMEDIUM stgMedium = {TYMED_HGLOBAL, {nullptr}};

        auto hr = pDataObject->GetData(&formatEtc, &stgMedium);
        if (SUCCEEDED(hr)) {
            HDROP hDrop = (HDROP)stgMedium.hGlobal;
            UINT  fileCount = DragQueryFileA(hDrop, 0xFFFFFFFF, nullptr, 0);
            for (UINT i = 0; i < fileCount; i++) {
                std::string fp;
                fp.resize(MAX_PATH);

                UINT sz;
                if ((sz = DragQueryFileA(hDrop, i, W(fp), MAX_PATH)) > 0) {
                    fp.resize(sz);
                    dst.push_back(fs::path(fp));
                }
            }

            ReleaseStgMedium(&stgMedium);
        }
    }
};  // namespace shell