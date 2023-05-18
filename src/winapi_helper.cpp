#include <winapi_helper.h>

namespace winapi {
void get_process_name(HANDLE hProcess, std::string &dst) {
  dst.clear();
  dst.resize(MAX_PATH);

  DWORD sz = MAX_PATH;
  if (!QueryFullProcessImageNameA(hProcess, NULL, W(dst), &sz)) {
    CRITICAL("QueryFullProcessImageNameA into hProcess:{} failed!",
             (void *)hProcess)
  }

  dst.resize(sz + 1);
}

void get_process_basename(HANDLE hProcess, std::string &dst) {
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

HMODULE get_module_base_address(const std::string &module) {
  HMODULE ret = GetModuleHandleA(module.c_str());
  if (ret == NULL)
    CRITICAL("GetModuleBaseAddress with module:{} failed", module)

  return ret;
}

HMODULE find_module_by_name(HANDLE hProcess, const std::string &modname) {
  DWORD modcnt = 0;
  if (!EnumProcessModules(hProcess, NULL, NULL, &modcnt)) {
    CRITICAL("EnumProcessModules[1] into hProcess:{} failed!",
             (void *)hProcess);
  }

  std::vector<HMODULE> process_modules(modcnt);
  if (!EnumProcessModules(hProcess, process_modules.data(), modcnt, &modcnt)) {
    CRITICAL("EnumProcessModules[2] into hProcess:{} failed!",
             (void *)hProcess);
  }

  for (auto hModule : process_modules) {
    std::string basename;
    basename.resize(MAX_PATH);

    if (!GetModuleBaseName(hProcess, hModule, W(basename), MAX_PATH)) {
      CRITICAL("GetModuleBaseName for process:{} module:{} failed!",
               (void *)hProcess, (void *)hModule)
    }

    lowercase(basename);

    if (basename.find(modname) != -1) {
      return hModule;
    }
  }

  return nullptr;
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
  void *ret = VirtualAllocEx(hProcess, NULL, size, MEM_COMMIT | MEM_RESERVE,
                             PAGE_READWRITE);
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
    CRITICAL("WriteProcessMemory into hProcess:{} from:{} to:{} failed!",
             (void *)hProcess, src, dst)
  }

  if (written != sz) {
    DEBUG("WriteProcessMemory {}/{} bytes were written", written, sz)
  }
}

void close(HANDLE hProcess) { CloseHandle(hProcess); }
}; // namespace remote

void get_cwd(std::string &dir) {
  dir.resize(MAX_PATH);
  DWORD sz = GetCurrentDirectoryA(MAX_PATH, W(dir));
  if (sz == 0) {
    CRITICAL("GetCurrentDirectoryA failed");
  }

  if (dir[sz] != '\\') {
    dir[sz] = '\\';
  }

  dir.resize(1 + sz);
}
}; // namespace winapi

namespace shell {
void get_files_from_do(IDataObject *pDataObject,
                       std::vector<std::wstring> &dst) {
  FORMATETC formatEtc = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  STGMEDIUM stgMedium = {TYMED_HGLOBAL, {nullptr}};

  if (SUCCEEDED(pDataObject->GetData(&formatEtc, &stgMedium))) {
    HDROP hDrop = (HDROP)stgMedium.hGlobal;
    UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
    for (UINT i = 0; i < fileCount; i++) {
      std::wstring fp;
      fp.resize(MAX_PATH);
      if (DragQueryFileW(hDrop, i, WW(fp), MAX_PATH) > 0)
        dst.push_back(fp);
    }

    ReleaseStgMedium(&stgMedium);
  }
}
}; // namespace shell