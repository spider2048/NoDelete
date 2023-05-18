#include <common.h>
#include <hook.h>

DWORD DllMain(HINSTANCE hInstance, DWORD reason, LPVOID _reserved) {
  if (reason == DLL_PROCESS_ATTACH) {
    std::string tmp_dir = std::getenv("TMP");
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        tmp_dir + "\\inject.log", false);
    logger::set_default_logger(
        std::make_shared<spdlog::logger>("FileLogger", file_sink));

    logger::set_level(logger::level::debug);
    logger::flush_on(logger::level::debug);
    hook::init();

    logger::debug("==== Attach Complete ====");
  } else if (reason == DLL_PROCESS_DETACH) {
    hook::exit();
    logger::debug("==== Detach Complete ====");
  }

  return true;
}