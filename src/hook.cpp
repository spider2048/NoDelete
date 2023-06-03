#include <hook.h>

DlgProc_t                 hook::PDlgProc;
DeleteItemsInDataObject_t hook::PDeleteItemsInDataObject;
std::vector<fs::path>     hook::selected_files;
bool                      hook::in_delete_operation;
fs::path                  hook::dll_dir;
fn_offsets                hook::offsets;
fs::path                  hook::store_dir;
std::vector<std::thread>  hook::active_threads;

void hook::attach() {
    /* dll_dir*/
    HMODULE self = winapi::get_module_handle(TARGET_DLL);
    dll_dir = fs::path(winapi::get_module_file_name(self)).parent_path();

    /* logging - create the log directory before injecting */
    fs::path log_dir = dll_dir / "log";

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>((log_dir / fmt::format("inject_{}.log", GetCurrentProcessId())).string());
    logger::set_default_logger(std::make_shared<spdlog::logger>("FileLogger", file_sink));
    logger::set_level(logger::level::debug);
    logger::flush_on(logger::level::debug);

    /* store files dir */
    store_dir = dll_dir / "store";
    if (!fs::exists(store_dir))
        fs::create_directory(store_dir);

    /* load offsets */
    load_offsets(dll_dir / OFFSET_FILE);
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
		fs::path ctx_store_dir = store_dir / std::to_string(GetTickCount());
		fs::create_directory(ctx_store_dir);
        fs::path new_path = ctx_store_dir / path.filename();
		
        /* copy file */
        fs::copy(path, new_path, fs::copy_options::recursive);

        /* set attributes */
		DWORD attribs = GetFileAttributesW(path.c_str());
        attribs &= ~(FILE_ATTRIBUTE_SYSTEM);
        attribs &= ~(FILE_ATTRIBUTE_HIDDEN);
        attribs |= FILE_ATTRIBUTE_NORMAL;
        SetFileAttributesW(new_path.c_str(), attribs);

        /* cleanup */
        fs::remove(path);
    } catch (std::exception& ec) {
        DEBUG("Copying from:{} to dst failed with exception:{}", path.string(), ec.what())
    }
}

volatile INT_PTR __fastcall hook::m_DlgProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    __try { /* Prevent most of the crashes */
	if (message == MESSAGE_DELETE && in_delete_operation) {
		if (wparam == DELETE_YES) {
			wparam = DELETE_NO;
			hide_files();
		}

		in_delete_operation = false;
		selected_files.clear();
	}

        /* native dlgproc */
        return PDlgProc(hwnd, message, wparam, lparam);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        DEBUG("Call to native DLGPROC failed with code:{:x}", GetExceptionCode())
        DEBUG("Arguments: HWND={} message={} wparam={} lparam={}", (void*)hwnd, (void*)message, (void*)wparam, (void*)lparam)
        return 0x2;
    }
}

volatile void __fastcall hook::m_DeleteItemsInDataObject(HWND hwnd, unsigned int param2, void* param3, IDataObject* pdo) {
    __try { /* Prevent most of the crashes */
    selected_files.clear();
    shell::get_files_from_do(pdo, selected_files);

    in_delete_operation = true;
    DEBUG("Detected delete operation with pdo:{} and files:{}", (void*)pdo, selected_files.size());

        PDeleteItemsInDataObject(hwnd, param2, param3, pdo);
        pdo->Release();  // free the data object
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        DEBUG("Call to native PDeleteItemsInDataObject failed with code:{:x}", GetExceptionCode())
        DEBUG("Arguments: HWND={} param2={} param3={} pdo={}", (void*)hwnd, param2, param3, (void*)pdo)
    }
}

void hook::load_offsets(const fs::path& offset_file) {
    if (!fs::exists(offset_file))
        CRITICAL("{} file doesn't exist!", OFFSET_FILE)

    std::ifstream           fd(offset_file);
    cereal::XMLInputArchive archive(fd);

    try {
        archive(offsets);
        DEBUG("loaded offsets from:{} with values:[{:x},{:x}]", offset_file.string(), offsets.fn_deleteitems, offsets.fn_dlgproc)
    } catch (std::exception& ec) {
        CRITICAL("Failed to load offsets!")
    }
}

void hook::hide_files() {
    for (const auto& path : selected_files) {
        DWORD attribs = GetFileAttributesW(path.c_str());
        attribs |= FILE_ATTRIBUTE_HIDDEN;
        attribs |= FILE_ATTRIBUTE_SYSTEM;
        attribs &= ~FILE_ATTRIBUTE_NORMAL;

        if (SetFileAttributesW(path.c_str(), attribs) != 0) {
            active_threads.push_back(std::thread(&hook::file_callback, path));
        } else {
            DEBUG("SetFileAttributesW failed on path:{}", path.string())
        }
    }
}