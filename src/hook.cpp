#include <hook.h>

DlgProc_t                 hook::PDlgProc;
DeleteItemsInDataObject_t hook::PDeleteItemsInDataObject;
std::vector<std::wstring> hook::selected_files;
bool                      hook::in_delete_operation;
bool                      hook::is_enabled;
fs::path                  hook::dll_dir;
fn_offsets                hook::offsets;

void hook::attach() {
    HMODULE self = winapi::get_module_base_address(TARGET_DLL);
    dll_dir = fs::path(winapi::get_module_path(self)).parent_path();

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>((dll_dir / fmt::format("inject_{}.log", GetCurrentProcessId())).string());

    logger::set_default_logger(std::make_shared<spdlog::logger>("FileLogger", file_sink));
    logger::set_level(logger::level::debug);
    logger::flush_on(logger::level::debug);

    long        ret;
    std::string cmdline = GetCommandLineA();
    if (cmdline.find("/factory") == -1) {
        DEBUG("Disabled in pid:{}", GetCurrentProcessId());
        is_enabled = false;
        return;
    }

    if (!fs::exists(dll_dir / OFFSET_FILE)) {
        CRITICAL("Offset file doesn't exist!")
    }

    load_offsets(dll_dir / OFFSET_FILE);
    DEBUG("Enabled in pid:{}", GetCurrentProcessId());

    uint64_t shell32_base = (uint64_t)winapi::get_module_base_address("shell32.dll");
    PDeleteItemsInDataObject = (DeleteItemsInDataObject_t)(shell32_base + offsets.fn_deleteitems);
    PDlgProc = (DlgProc_t)(shell32_base + offsets.fn_dlgproc);

    DETOUR_INIT
    DETOUR_ATTACHEX(DeleteItemsInDataObject)
    DETOUR_ATTACHEX(DlgProc)
    DETOUR_COMMIT

    in_delete_operation = false;
    DEBUG("==== Attach Complete ====")
}

void hook::detach() {
    if (!is_enabled) {
        return;
    }

    DETOUR_INIT
    DETOUR_DETACH(DeleteItemsInDataObject)
    DETOUR_DETACH(DlgProc)
    DETOUR_COMMIT

    DEBUG("==== Detach Complete ====")
}

volatile INT_PTR __fastcall hook::m_DlgProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == MESSAGE_DELETE && in_delete_operation) {
        if (wparam == DELETE_YES) {
            wparam = DELETE_NO;
        }

        in_delete_operation = false;
        selected_files.clear();
    }

    return PDlgProc(hwnd, message, wparam, lparam);
}

volatile void __fastcall hook::m_DeleteItemsInDataObject(HWND hwnd, unsigned int param2, void* param3, IDataObject* pdo) {
    in_delete_operation = shell::get_files_from_do(pdo, selected_files);
    DEBUG("Detected delete operation with pdo:{} and files:{}", (void*)pdo, selected_files.size());
    PDeleteItemsInDataObject(hwnd, param2, param3, pdo);
}

void hook::load_offsets(const fs::path& offset_file) {
    DEBUG("loading offsets from:{}", offset_file.string())

    std::ifstream              fd(offset_file.string());
    cereal::BinaryInputArchive archive(fd);

    archive(offsets);
    DEBUG("loaded offsets from:{} with values:[{:x},{:x}]", offset_file.string(), offsets.fn_deleteitems, offsets.fn_dlgproc)
}