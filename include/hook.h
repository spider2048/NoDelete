#pragma once
#include <common.h>

typedef INT_PTR (*DlgProc_t)(HWND, UINT, WPARAM, LPARAM);
typedef void(__fastcall* DeleteItemsInDataObject_t)(HWND hwnd, unsigned int param2, void* param3, IDataObject* pdo);

class hook {
    static std::vector<std::wstring> selected_files;
    static bool                      in_delete_operation;
    static bool                      is_enabled;
    static fs::path                  dll_dir;
    static fn_offsets                offsets;

    static DlgProc_t                 PDlgProc;
    static DeleteItemsInDataObject_t PDeleteItemsInDataObject;

    volatile static INT_PTR __fastcall m_DlgProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    volatile static void __fastcall m_DeleteItemsInDataObject(HWND hwnd, unsigned int param2, void* param3, IDataObject* pdo);
    static void load_offsets(const fs::path& offset_file);

   public:
    static void attach();
    static void detach();
};