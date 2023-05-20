#pragma once
#include <common.h>

namespace winapi {
    std::string get_process_name(HANDLE);
    void        get_process_basename(HANDLE, std::string&);
    uint64_t    get_func_offset(HMODULE, const std::string&);
    HMODULE     get_module_base_address(const std::string&);
    HMODULE     find_module_by_name(HANDLE, const std::string&);
    std::string get_cwd();
    std::string get_windows_dir();
    HMODULE     load_library_as_datafile(const std::string& lib);
    std::string get_module_path(HMODULE hModule);

    namespace remote {
        HANDLE open(DWORD pid);
        void*  alloc(HANDLE hProcess, size_t size);
        void   free(HANDLE hProcess, void* base);
        void   write(HANDLE hProcess, void* dst, void* src, size_t sz);
        void   close(HANDLE hProcess);
    };  // namespace remote
};      // namespace winapi

namespace shell {
    void get_files_from_do(IDataObject* pDataObject, std::vector<std::filesystem::path>& dst);
};