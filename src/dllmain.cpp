#include <common.h>
#include <hook.h>

DWORD DllMain(HINSTANCE hInstance, DWORD reason, LPVOID _reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        try {
            hook::attach();
        } catch (std::exception& ec) {
            MessageBoxA(NULL, fmt::format("Attaching to PID:{} failed! with exception:{}", GetCurrentProcessId(), ec.what()).c_str(), "Hook attach error", MB_ICONERROR);
        }

    } else if (reason == DLL_PROCESS_DETACH) {
        hook::detach();
    }

    return true;
}