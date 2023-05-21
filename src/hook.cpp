#include <hook.h>

DlgProc_t                 hook::PDlgProc;
DeleteItemsInDataObject_t hook::PDeleteItemsInDataObject;
std::vector<fs::path>     hook::selected_files;
bool                      hook::in_delete_operation;
bool                      hook::is_enabled;
fs::path                  hook::dll_dir;
fn_offsets                hook::offsets;
fs::path                  hook::store_dir;
std::vector<std::thread>  hook::active_threads;

void hook::attach() {
    /* set exception handler */
    SetUnhandledExceptionFilter(&winapi::ExceptionHandler);

    /* dll_dir*/
    HMODULE self = winapi::get_module_handle(TARGET_DLL);
    dll_dir = fs::path(winapi::get_module_file_name(self)).parent_path();

    /* logging - create the log directory before injecting */
    fs::path log_dir = dll_dir / "log";

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>((log_dir / fmt::format("inject_{}.log", GetCurrentProcessId())).string());
    logger::set_default_logger(std::make_shared<spdlog::logger>("FileLogger", file_sink));
    logger::set_level(logger::level::debug);
    logger::flush_on(logger::level::debug);

    /* command line check (disable in root process) */
    if (std::string(GetCommandLineA()).find("/factory") == -1) {
        DEBUG("Disabled in pid:{}", GetCurrentProcessId());
        is_enabled = false;
        return;
    }

    DEBUG("Enabled in pid:{}", GetCurrentProcessId());

    /* store files dir */
    store_dir = dll_dir / "store";
    DEBUG("{}", store_dir.string())
    if (!fs::exists(store_dir))
        fs::create_directory(store_dir);

    /* load offsets */
    fs::path offset_path = dll_dir / OFFSET_FILE;
    if (!fs::exists(offset_path))
        CRITICAL("{} file doesn't exist!", OFFSET_FILE)
    load_offsets(offset_path);
    uint64_t shell32_base = (uint64_t)winapi::get_module_handle("shell32.dll");
    PDeleteItemsInDataObject = (DeleteItemsInDataObject_t)(shell32_base + offsets.fn_deleteitems);
    PDlgProc = (DlgProc_t)(shell32_base + offsets.fn_dlgproc);

    /* attach hooks */
    DETOUR_INIT
    long ret;
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

    for (auto& t : active_threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    DEBUG("==== Detach Complete ====")
}

void hook::file_callback(const fs::path& path) {
    try {
        fs::copy(path, store_dir);
        // fs::remove(path);
    } catch (std::exception& ec) {
        DEBUG("Copying from:{} to dst failed with exception:{}", path.string(), ec.what())
    }
}

volatile INT_PTR __fastcall hook::m_DlgProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == MESSAGE_DELETE && in_delete_operation) {
        if (wparam == DELETE_YES) {
            wparam = DELETE_NO;

            for (const auto& path : selected_files) {
                DWORD attribs = GetFileAttributesW(path.c_str());
                attribs |= FILE_ATTRIBUTE_HIDDEN;
                attribs |= FILE_ATTRIBUTE_SYSTEM;
                attribs &= ~FILE_ATTRIBUTE_NORMAL;

                auto ret = SetFileAttributesW(path.c_str(), attribs);
                DEBUG("SetFileAttributesW returned:{} to file:{}", ret, path.string())

                active_threads.push_back(std::thread(&hook::file_callback, path));
            }
        }

        in_delete_operation = false;
        selected_files.clear();
    }

    try {
        return PDlgProc(hwnd, message, wparam, lparam);
    } catch (...) {
        DEBUG("Call to native DLGPROC failed!")
        DEBUG("Arguments: HWND={} message={} wparam={} lparam={}", (void*)hwnd, (void*)message, (void*)wparam, (void*)lparam)
        return 0x2;
    }
}

volatile void __fastcall hook::m_DeleteItemsInDataObject(HWND hwnd, unsigned int param2, void* param3, IDataObject* pdo) {
    selected_files.clear();

    try {
        shell::get_files_from_do(pdo, selected_files);
    } catch (std::exception& ec) {
        DEBUG("shell::get_files_from_do failed with ec:{}", ec.what())
        return;
    }

    in_delete_operation = true;
    DEBUG("Detected delete operation with pdo:{} and files:{}", (void*)pdo, selected_files.size());

    try {
        PDeleteItemsInDataObject(hwnd, param2, param3, pdo);
        pdo->Release();
    } catch (...) {
        DEBUG("Call to native PDeleteItemsInDataObject failed!")
        DEBUG("Arguments: HWND={} param2={} param3={} pdo={}", (void*)hwnd, param2, param3, (void*)pdo)
    }
}

void hook::load_offsets(const fs::path& offset_file) {
    DEBUG("loading offsets from:{}", offset_file.string())

    std::ifstream              fd(offset_file.string());
    cereal::BinaryInputArchive archive(fd);

    archive(offsets);
    DEBUG("loaded offsets from:{} with values:[{:x},{:x}]", offset_file.string(), offsets.fn_deleteitems, offsets.fn_dlgproc)
}   