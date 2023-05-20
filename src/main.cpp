#include <common.h>
#include <inject.h>

LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* ex) {
    switch (ex->ExceptionRecord->ExceptionCode) {
        CASE(EXCEPTION_ACCESS_VIOLATION)
        CASE(EXCEPTION_ARRAY_BOUNDS_EXCEEDED)
        CASE(EXCEPTION_BREAKPOINT)
        CASE(EXCEPTION_DATATYPE_MISALIGNMENT)
        CASE(EXCEPTION_FLT_DENORMAL_OPERAND)
        CASE(EXCEPTION_FLT_DIVIDE_BY_ZERO)
        CASE(EXCEPTION_FLT_INEXACT_RESULT)
        CASE(EXCEPTION_FLT_INVALID_OPERATION)
        CASE(EXCEPTION_FLT_OVERFLOW)
        CASE(EXCEPTION_FLT_STACK_CHECK)
        CASE(EXCEPTION_FLT_UNDERFLOW)
        CASE(EXCEPTION_ILLEGAL_INSTRUCTION)
        CASE(EXCEPTION_IN_PAGE_ERROR)
        CASE(EXCEPTION_INT_DIVIDE_BY_ZERO)
        CASE(EXCEPTION_INT_OVERFLOW)
        CASE(EXCEPTION_INVALID_DISPOSITION)
        CASE(EXCEPTION_NONCONTINUABLE_EXCEPTION)
        CASE(EXCEPTION_PRIV_INSTRUCTION)
        CASE(EXCEPTION_SINGLE_STEP)
        CASE(EXCEPTION_STACK_OVERFLOW)
    }

    auto rip = ex->ExceptionRecord->ExceptionAddress;
    auto base = winapi::get_module_base_address(
        winapi::get_process_name(GetCurrentProcess())
    );

    std::cout << " at RIP: 0x" << rip << " loaded @base address: 0x" << base << std::endl;
    std::cout << "Offset: 0x" << std::hex << (uint64_t)rip - (uint64_t)base << std::endl;

    std::cout << "******\nPlease report this!\n******";
    return EXCEPTION_CONTINUE_SEARCH;
}

int main(int argc, char** argv) {
    SetUnhandledExceptionFilter(ExceptionHandler);
    try {
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>((fs::path(winapi::get_cwd()) / "base.log").string());

        logger::set_default_logger(std::make_shared<spdlog::logger>("FileLogger", file_sink));
        logger::set_level(logger::level::debug);
        logger::flush_on(logger::level::debug);

        inject::init();
        for (auto pid : inject::get_explorer_pids()) {
            inject::inject_to_pid(pid);
        }
    } catch (std::exception& ec) {
        MessageBoxA(NULL, fmt::format("{} failed with {}", argv[0], ec.what()).c_str(), "Message", MB_OK);
    }
}
