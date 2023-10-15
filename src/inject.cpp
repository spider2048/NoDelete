#include <common.h>
#include <dia2.h>
#include <inject.h>

/* throws exception when `hr` is a failed one */
#define CHECK_FAIL(X) \
    if (FAILED(hr))   \
    CRITICAL(X " failed with hr:{:x}", hr)

fs::path inject::dll_path;

/* initialize some variables */
void inject::init() {
    fs::path cwd = winapi::get_cwd();
    fs::path log_dir = cwd / "log";
    dll_path = cwd / "NoDeleteH.dll";

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>((log_dir / "base.log").string());
    logger::set_default_logger(std::make_shared<spdlog::logger>("NoDelete", file_sink));

#ifdef DEBUG
    logger::set_level(logger::level::debug);
    logger::flush_on(logger::level::debug);
#else
    logger::set_level(logger::level::info);
#endif

    if (!fs::exists(log_dir))
        fs::create_directory(log_dir);

    inject::save_offsets("./shell32.pdb", cwd / "offsets.xml");
}

/* injects the helper dll into the pid */
void inject::inject_to_pid(DWORD pid) {
    HANDLE hProcess = winapi::remote::open(pid);
    try {
        winapi::find_module_by_name(hProcess, dll_path.string());
    } catch (std::exception& ec) {
        DEBUG("Already injected into pid:{}", pid)
        CloseHandle(hProcess);
        return;
    }

    try {
        void* remote_mem = winapi::remote::alloc(hProcess, dll_path.string().size());
        winapi::remote::write(hProcess, remote_mem, W(dll_path.string()), dll_path.string().size());
        HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)LoadLibraryA, remote_mem, NULL, NULL);
        WaitForSingleObject(hRemoteThread, INFINITE);
        winapi::remote::free(hProcess, remote_mem);
        winapi::remote::close(hProcess);
        INFO("Done injecting into PID:{}", pid);
    } catch (std::exception& ec) {
        DEBUG("Failed to inject to PID:{} with error:{}", pid, ec.what())
    }
}

/* enumerates all explorer.exe PIDs */
std::vector<DWORD> inject::get_explorer_pids() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        CRITICAL("CreateToolhelp32Snapshot failed");
    }

    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &processEntry)) {
        CloseHandle(hSnapshot);
        CRITICAL("Process32First failed");
    }

    std::vector<DWORD> pidlist;
    do {
        HANDLE hProcess = winapi::remote::open(processEntry.th32ProcessID);
        if (hProcess == 0)
            continue;

        if (winapi::get_process_name(hProcess).find("C:\\Windows\\explorer.exe") != -1) {
            pidlist.push_back(processEntry.th32ProcessID);
        }

        winapi::remote::close(hProcess);
    } while (Process32Next(hSnapshot, &processEntry));

    CloseHandle(hSnapshot);
    return pidlist;
}

/* downloads the pdb file using curl.exe */
void inject::download_pdb_file(const fs::path& dest) {
    fs::path      shell32 = winapi::get_windows_dir() / "System32" / "shell32.dll";
    std::ifstream dll_file(shell32.c_str(), std::ios::binary);
    size_t        file_sz = fs::file_size(shell32);

    std::string dll_data;
    dll_data.resize(file_sz);
    dll_file.read(W(dll_data), file_sz);

    auto loc_shell32pdb = dll_data.find("shell32.pdb");
    if (loc_shell32pdb == -1) {
        CRITICAL("`shell32.pdb` not found in shell32.dll")
    }

    auto        loc_guid = loc_shell32pdb - 20;
    std::string _guid = dll_data.substr(loc_guid, 16);
    dll_data.clear();

    GUID guid;
    memcpy(&guid, _guid.data(), 16);

    std::wstring _guidstr;
    _guidstr.resize(39);
    StringFromGUID2(guid, WW(_guidstr), _guidstr.size());
    std::string guidstr = ws2s(_guidstr) + std::to_string(dll_data[loc_guid + 16]);

    std::erase(guidstr, '{');
    std::erase(guidstr, '}');
    std::erase(guidstr, '-');
    std::erase(guidstr, '\x00');

    std::string cmd = fmt::format("curl -qL http://msdl.microsoft.com/download/symbols/shell32.pdb/{}/shell32.pdb -o {}", guidstr, dest.string());
    INFO("Downloading shell32.pdb")

    auto ret = system(cmd.c_str());
    if (ret != 0) {
        CRITICAL("Failed to download shell32.pdb file")
    }

    INFO("Downloaded the pdb file to {}", dest.string())
}

/* saves the function offsets to offsets (usually offsets.xml) file */
void inject::save_offsets(const fs::path& pdb_path, const fs::path& output_archive) {
    if (!fs::exists(pdb_path)) {
        inject::download_pdb_file(pdb_path);
    }

    DEBUG("Acquiring offsets from pdb file ...")
    std::wstring fn_dialog = L"?s_ConfirmDialogProc@CConfirmationDlgBase@@CA_JPEAUHWND__@@I_K_J@Z";
    std::wstring fn_deleteitems = L"DeleteItemsInDataObject";
    DWORD        dialog_off = -1, deleteitems_off = -1;

    HRESULT hr;
    hr = CoInitialize(NULL);
    CHECK_FAIL("CoInitialize")

    IDiaDataSource* pDataSource;
    hr = CoCreateInstance(CLSID_DiaSource, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDataSource));
    // CHECK_FAIL("CoCreateInstance")

    hr = pDataSource->loadDataFromPdb(pdb_path.c_str());
    CHECK_FAIL("pDataSource->loadDataFromPdb")

    /* the remaining functions won't fail */
    IDiaSession* pSession;
    hr = pDataSource->openSession(&pSession);
    // CHECK_FAIL("pDataSource->openSession")

    IDiaSymbol* pGlobalScope;
    hr = pSession->get_globalScope(&pGlobalScope);
    // CHECK_FAIL("pSession->get_globalScope")

    IDiaEnumSymbols* pFunctions;
    hr = pGlobalScope->findChildren(SymTagPublicSymbol, NULL, nsNone, &pFunctions);
    // CHECK_FAIL("pGlobalScope->findChildren")

    IDiaSymbol* pFunction = nullptr;
    ULONG       celt = 0;
    while (SUCCEEDED(pFunctions->Next(1, &pFunction, &celt)) && celt == 1) {
        BSTR fnname;
        pFunction->get_name(&fnname);
        std::wstring fn(fnname);
        SysFreeString(fnname);

        if (fn.find(fn_dialog) != -1)
            pFunction->get_addressOffset(&dialog_off);
        else if (fn.find(fn_deleteitems) != -1)
            pFunction->get_addressOffset(&deleteitems_off);
        pFunction->Release();
    }

    pFunctions->Release();
    pGlobalScope->Release();
    pSession->Release();
    pDataSource->Release();
    CoUninitialize();

    if (dialog_off == -1 || deleteitems_off == -1) {
        MessageBoxA(NULL, "All functions are not found!", "Error Injecting to process", MB_OK | MB_ICONERROR);
        CRITICAL("all functions are not found")
    }

    dialog_off += 0x1000;
    deleteitems_off += 0x1000;

    DEBUG("saving offsets ...")
    try {
        std::ofstream            fd(output_archive);
        cereal::XMLOutputArchive archive(fd);
        fn_offsets               o = {.fn_deleteitems = deleteitems_off, .fn_dlgproc = dialog_off};
        archive(o);
    } catch (std::exception& ec) {
        CRITICAL("Failed to save offsets to: {}", output_archive.string())
    }

    INFO("Finished saving offsets", output_archive.string(), dialog_off, deleteitems_off);
}
