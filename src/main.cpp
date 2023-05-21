#include <common.h>
#include <inject.h>

int main(int argc, char** argv) {
    fs::path log_dir = winapi::get_cwd() / "log";
    if (!fs::exists(log_dir))
        fs::create_directory(log_dir);

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>((log_dir / "base.log").string());
    logger::set_default_logger(std::make_shared<spdlog::logger>("FileLogger", file_sink));
    logger::set_level(logger::level::debug);
    logger::flush_on(logger::level::debug);

    try {
        inject::init();
        for (auto pid : inject::get_explorer_pids()) {
            inject::inject_to_pid(pid);
        }
    } catch (const std::exception& ec) {
        MessageBoxA(NULL, fmt::format("{} (injection) failed with {}", argv[0], ec.what()).c_str(), "Error Injecting to procss", MB_OK | MB_ICONERROR);
    }
}
