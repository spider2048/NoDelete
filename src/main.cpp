#include <common.h>
#include <inject.h>

int main(int argc, char** argv) {
    try {
        inject::init();
        for (auto pid : inject::get_explorer_pids()) {
            inject::inject_to_pid(pid);
        }
    } catch (const std::exception& ec) {
        MessageBoxA(NULL, fmt::format("{} (injection) failed with {}", argv[0], ec.what()).c_str(), "Error Injecting to process", MB_OK | MB_ICONERROR);
    }
}
