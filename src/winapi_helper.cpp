#include <winapi_helper.h>
namespace fs = std::filesystem;

#define CASE(X)      \
    case X:          \
        err += (#X); \
        break;

namespace winapi {
    /* gets the windows directory of the process */
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

    /* gets the full process name using a handle */
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

    /* gets the process base name using a handle */
    void get_process_base_name(HANDLE hProcess, std::string &dst) {
        dst.clear();
        dst.resize(MAX_PATH);

        if (!GetModuleBaseNameA(hProcess, NULL, W(dst), MAX_PATH)) {
            CRITICAL("GetModuleBaseNameA into hProcess:{} failed!", (void *)hProcess)
        }

        DWORD sz = strlen(W(dst));
        dst.resize(sz + 1);
    }

    /* gets the functoin offset using GetProcAddress */
    uint64_t get_func_offset(HMODULE hModule, const std::string &func) {
        uint64_t loaded_addr = (uint64_t)GetProcAddress(hModule, func.c_str());
        if (loaded_addr == 0) {
            CRITICAL("GetProcAddress with func:{} failed!", func);
        }

        return loaded_addr - (uint64_t)hModule;
    }

    /* gets the module handle */
    HMODULE get_module_handle(const std::string &module) {
        HMODULE ret = GetModuleHandleA(module.c_str());
        if (ret == nullptr)
            CRITICAL("GetModuleBaseAddress with module:{} failed!", module)

        return ret;
    }

    /* gets the module file name using a handle */
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

    /* finds a module by name in a process */
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

    /* gets the cwd of the calling process */
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
        /* gets a handle to another process by PID */
        HANDLE open(DWORD pid) {
            HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
            if (hProcess == INVALID_HANDLE_VALUE) {
                CRITICAL("OpenProcess pid:{} failed", pid)
            }

            return hProcess;
        }

        /* allocates some memory in the process (commits) */
        void *alloc(HANDLE hProcess, size_t size) {
            void *ret = VirtualAllocEx(hProcess, NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (ret == NULL) {
                CRITICAL("VirtualAllocEx into hProcess:{} failed!", (void *)hProcess)
            }

            return ret;
        }

        /* frees the allocated memory in the process */
        void free(HANDLE hProcess, void *base) {
            if (!VirtualFreeEx(hProcess, base, NULL, MEM_RELEASE)) {
                CRITICAL("VirtualFreeEx into hProcess:{} failed!", (void *)hProcess)
            }
        }

        /* writes some memory in the process' virtual memory */
        void write(HANDLE hProcess, void *dst, void *src, size_t sz) {
            size_t written = 0;
            if (!WriteProcessMemory(hProcess, dst, src, sz, &written)) {
                CRITICAL("WriteProcessMemory into hProcess:{} from:{} to:{} failed!", (void *)hProcess, src, dst)
            }

            if (written != sz) {
                DEBUG("WriteProcessMemory {}/{} bytes were written", written, sz)
            }
        }

        /* closes the process handle */
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