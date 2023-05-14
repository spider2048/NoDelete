#include <common.h>
#include <hook.h>

#define LOGFILE "D:\\Cpp\\No Delete\\Tracelog.txt"

DWORD DllMain(HINSTANCE hInstance, DWORD reason, LPVOID _reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(LOGFILE);
        spdlog::set_default_logger(std::make_shared<spdlog::logger>("FileLogger", file_sink));
        logger::set_level(logger::level::debug);
        hook::init();
    } else if (reason == DLL_PROCESS_DETACH) {
        hook::cleanup();
    }

    return true;
}
