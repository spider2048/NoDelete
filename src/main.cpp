#include <common.h>
#include <hook.h>
#include <inject.h>

int main(int argc, char **argv) {
  std::string cwd;
  winapi::get_cwd(cwd);

  auto file_sink =
      std::make_shared<spdlog::sinks::basic_file_sink_mt>("./base.log", true);
  logger::set_default_logger(
      std::make_shared<spdlog::logger>("FileLogger", file_sink));

  logger::set_level(logger::level::debug);
  logger::flush_on(logger::level::debug);

  inject::init();
  auto pidlist = inject::get_explorer_pids();

  for (auto p : pidlist) {
    inject::inject_to_pid(p);
  }
}
