#pragma once
#include <common.h>

namespace hook {
struct opened_files {
  DWORD lastModified;
  std::wstring fileName;
};

std::unordered_map<DWORD, std::vector<opened_files>> thread_map;

typedef INT_PTR (*DialogBoxParamW_t)(HINSTANCE hInstance,
                                     LPCWSTR lpTemplateName, HWND hWndParent,
                                     DLGPROC lpDialogFunc, LPARAM dwInitParam);

DialogBoxParamW_t PDialogBoxParamW;

INT_PTR m_DialogBoxParamW(HINSTANCE hInstance, LPCWSTR lpTemplateName,
                          HWND hWndParent, DLGPROC lpDialogFunc,
                          LPARAM dwInitParam);

typedef DWORD (*GetFileAttributesW_t)(LPCWSTR lpFileName);

GetFileAttributesW_t PGetFileAttributesW;
DWORD m_GetFileAttributesW(LPCWSTR lpFileName);

void init() {
    logger::info(" -------- ATTACHED --------");

    DETOUR_INIT
    DETOUR_ATTACH(GetFileAttributesW)
    DETOUR_ATTACH(DialogBoxParamW)
    DETOUR_COMMIT
}

void cleanup() {
    logger::info(" -------- DETACHED --------");
    DETOUR_INIT
    DETOUR_DETACH(GetFileAttributesW)
    DETOUR_DETACH(DialogBoxParamW);
    DETOUR_COMMIT
}

INT_PTR m_DialogBoxParamW(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam) {
    if ((int)lpTemplateName == RC_DELETE) {
        DWORD tid = GetCurrentThreadId();
        auto _size = thread_map[tid].size();

        std::string target_file = ws2s(thread_map[tid].at(_size - 2).fileName);

        logger::info("Detected delete window func:{} initparam:{} target_file:{}", (void *)lpDialogFunc, (void *)dwInitParam, target_file);
        thread_map.erase(tid);
    }

    return PDialogBoxParamW(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
}

DWORD m_GetFileAttributesW(LPCWSTR lpFileName) {
    DWORD tid = GetCurrentThreadId();
    if (!hook::thread_map.count(tid)) {
        hook::thread_map.emplace(tid, std::vector<opened_files>{});
    }

    thread_map[tid].push_back({timeGetTime(), lpFileName});
    return PGetFileAttributesW(lpFileName);
}

void init();
void cleanup();
}; // namespace hook
