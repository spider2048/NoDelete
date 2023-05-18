#pragma once
#include <common.h>

namespace winapi {
    void get_process_name(HANDLE, std::string&);
    void get_process_basename(HANDLE, std::string&);
    uint64_t get_func_offset(HMODULE, const std::string&);
    HMODULE get_module_base_address(const std::string&);
    HMODULE find_module_by_name(HANDLE, const std::string&);
    void get_cwd(std::string& dir);

    namespace remote {
        HANDLE open(DWORD pid);
        void* alloc(HANDLE hProcess, size_t size);
        void free(HANDLE hProcess, void* base);
        void write(HANDLE hProcess, void* dst, void* src, size_t sz);
        void close(HANDLE hProcess);
    };
};

namespace shell {
    void get_files_from_do(IDataObject* pDataObject, std::vector<std::wstring>& dst);
};