#include <common.h>
#include <dia2.h>
#include <inject.h>

#define CHECK_FAIL(X) \
    if (FAILED(hr))   \
    CRITICAL(X " failed with hr:{:x}", (void*)hr)

fs::path inject::dll_path;

void inject::init() {
    dll_path = winapi::get_cwd() / TARGET_DLL;
    fs::path log_dir = winapi::get_cwd() / "log";
    if (!fs::exists(log_dir))
        fs::create_directory(log_dir);

    try {
        inject::save_offsets("./shell32.pdb", (dll_path.parent_path() / OFFSET_FILE).string());
    } catch (std::exception& e) {
        CRITICAL("Failed to save offsets to shell32.pdb!")
    }
}

void inject::inject_to_pid(DWORD pid) {
    HANDLE hProcess = winapi::remote::open(pid);
    try {
        winapi::find_module_by_name(hProcess, dll_path.string());
    } catch (std::exception& ec) {
        DEBUG("Already injected into pid:{}", pid)
        CloseHandle(hProcess);
        return;
    }

    void* remote_mem = winapi::remote::alloc(hProcess, dll_path.string().size());
    winapi::remote::write(hProcess, remote_mem, W(dll_path.string()), dll_path.string().size());

    HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)LoadLibraryA, remote_mem, NULL, NULL);
    if (!hRemoteThread) {
        CRITICAL("CreateRemoteThreaad into hProcess:{} failed!", hProcess);
    }

    WaitForSingleObject(hRemoteThread, INFINITE);
    winapi::remote::free(hProcess, remote_mem);
    winapi::remote::close(hProcess);
    DEBUG("Done injecting into PID:{}", pid);
}

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

void inject::download_pdb_file(const std::string& dest) {
    DEBUG("Downloading the pdb file to:{}", dest)

    fs::path      shell32 = winapi::get_windows_dir() / "System32" / "shell32.dll";
    std::ifstream dll_file(shell32.string(), std::ios::binary);
    size_t        file_sz = fs::file_size(shell32);

    std::string dll_data;
    dll_data.resize(file_sz);
    dll_file.read(W(dll_data), file_sz);

    auto loc_shell32pdb = dll_data.find("shell32.pdb");
    if (loc_shell32pdb == -1) {
        CRITICAL("shell32.pdb not found in shell32.dll")
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

    std::string cmd;
    cmd += "curl -L http://msdl.microsoft.com/download/symbols/shell32.pdb/" + guidstr + "/shell32.pdb -o " + dest;
    DEBUG("downloading shell32.pdb cmd:{}", cmd)

    auto ret = system(cmd.c_str());
    if (ret != 0) {
        CRITICAL("Failed to download the pdb file to dest:{dest}")
    }

    DEBUG("Downloaded the pdb file to {}", dest)
}

void inject::save_offsets(const std::string& pdb_path, const std::string& output_archive) {
    if (!fs::exists(fs::path(pdb_path))) {
        inject::download_pdb_file(pdb_path);
    }

    std::wstring fn_dialog = L"?s_ConfirmDialogProc@CConfirmationDlgBase@@CA_JPEAUHWND__@@I_K_J@Z";
    std::wstring fn_deleteitems = L"DeleteItemsInDataObject";
    DWORD        dialog_off = -1, deleteitems_off = -1;

    CoInitialize(NULL);
    HRESULT hr;

    IDiaDataSource* pDataSource;
    hr = CoCreateInstance(CLSID_DiaSource, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDataSource));
    CHECK_FAIL("CoCreateInstance")

    hr = pDataSource->loadDataFromPdb(s2ws(pdb_path).c_str());
    CHECK_FAIL("pDataSource->loadDataFromPdb")

    IDiaSession* pSession;
    hr = pDataSource->openSession(&pSession);
    CHECK_FAIL("pDataSource->openSession")

    IDiaSymbol* pGlobalScope;
    hr = pSession->get_globalScope(&pGlobalScope);
    CHECK_FAIL("pSession->get_globalScope")

    IDiaEnumSymbols* pFunctions;
    hr = pGlobalScope->findChildren(SymTagPublicSymbol, NULL, nsNone, &pFunctions);
    CHECK_FAIL("pGlobalScope->findChildren")

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
        CRITICAL("All functions are not found: off1:{:x} off2:{:x}", dialog_off, deleteitems_off)
    }

    dialog_off += 0x1000;
    deleteitems_off += 0x1000;

    try {
        std::ofstream               fd(output_archive);
        cereal::BinaryOutputArchive archive(fd);
        fn_offsets                  o = {.fn_deleteitems = deleteitems_off, .fn_dlgproc = dialog_off};
        archive(o);
    } catch (std::exception& ec) {
        CRITICAL("Failed to save offsets to {}", OFFSET_FILE)
    }

    DEBUG("Finished saving offsets to {} [{:x}, {:x}]", output_archive, dialog_off, deleteitems_off);
}
