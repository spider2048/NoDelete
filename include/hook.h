#pragma once
#include <common.h>
#include <winapi_helper.h>

namespace hook {
    std::vector<std::wstring> selected_files;
    bool in_delete_operation = false;
    bool is_enabled = false;

    typedef INT_PTR (*DlgProc_t) (HWND, UINT, WPARAM, LPARAM);
    DlgProc_t PDlgProc;

    INT_PTR m_DlgProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
        if (message == MESSAGE_DELETE && in_delete_operation) {
            if (wparam == DELETE_YES) {
                wparam = DELETE_NO;
            }

            in_delete_operation = false;
            selected_files.clear();
        }

        return PDlgProc(hwnd, message, wparam, lparam);
    }

    typedef void (__fastcall *DeleteItemsInDataObject_t) (HWND hwnd, unsigned int param2, void* param3, IDataObject* pdo);
    DeleteItemsInDataObject_t PDeleteItemsInDataObject;
    
    void m_DeleteItemsInDataObject(HWND hwnd, unsigned int param2, void* param3, IDataObject* pdo) {
        shell::get_files_from_do(pdo, selected_files);
        logger::debug("Detected delete operation with pdo:{} and files:{}", (void*)pdo, selected_files.size());
        in_delete_operation = true;
        PDeleteItemsInDataObject(hwnd, param2, param3, pdo);
    }
    
    void init() {
        LONG ret;
        std::wstring cmdline = GetCommandLineW();
        if (cmdline.find(L"/factory") == -1) {
            DEBUG("Disabled in pid:{}", GetCurrentProcessId());
            is_enabled = false;
            return;
        }

        DEBUG("Enabled in pid:{}", GetCurrentProcessId());
        
        uint64_t shell32_base = (uint64_t)winapi::get_module_base_address("shell32.dll");
        PDeleteItemsInDataObject = (DeleteItemsInDataObject_t) (shell32_base + DELETEITEMS_OFFSET);
        PDlgProc = (DlgProc_t)(shell32_base + DLGPROC_OFFSET);
        
        DETOUR_INIT
        DETOUR_ATTACHEX(DeleteItemsInDataObject)
        DETOUR_ATTACHEX(DlgProc);
        DETOUR_COMMIT
    }

    void exit() {
        if (!is_enabled) {
            return;
        }

        DETOUR_INIT
        DETOUR_DETACH(DeleteItemsInDataObject)
        DETOUR_DETACH(DlgProc);
        DETOUR_COMMIT
    }
}; // namespace hook
