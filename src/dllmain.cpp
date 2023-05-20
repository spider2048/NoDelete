#include <common.h>
#include <hook.h>

DWORD DllMain(HINSTANCE hInstance, DWORD reason, LPVOID _reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        hook::attach();
    } else if (reason == DLL_PROCESS_DETACH) {
        hook::detach();
    }

    return true;
}