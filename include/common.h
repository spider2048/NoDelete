#pragma once

#include <algorithm>
#include <codecvt>
#include <iostream>
#include <locale>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <thread>

#include <windows.h>
#include <detours.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <TlHelp32.h>
#include <psapi.h>
#include <cereal/cereal.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/archives/binary.hpp>

#include <util.h>
#include <winapi_helper.h>

#define W(X) const_cast<char *>(X.c_str())
#define WW(X) const_cast<wchar_t *>(X.c_str())
#define _DETOUR_TARGET(X) P##X
#define _DETOUR_DEST(X) m_##X
#define DETOUR_INIT           \
    DetourTransactionBegin(); \
    DetourUpdateThread(GetCurrentThread());
#define DETOUR_ATTACH(X)   \
    _DETOUR_TARGET(X) = X; \
    ret = DetourAttach((void **)&_DETOUR_TARGET(X), _DETOUR_DEST(X));

#define DETOUR_DETACH(X) DetourDetach((void **)&_DETOUR_TARGET(X), _DETOUR_DEST(X));
#define DETOUR_COMMIT DetourTransactionCommit();

#define DETOUR_ATTACHEX(X)                                            \
    ret = DetourAttach((void **)&_DETOUR_TARGET(X), _DETOUR_DEST(X)); \
    logger::debug("DetourAttach to {} returned:{}", #X, ret);

#define DEBUG(FMT, ...) logger::debug("[{}:{} l{} E{}] " FMT, __FILE__, __func__, __LINE__, GetLastError(), __VA_ARGS__);
#define CRITICAL(FMT, ...)                                                                                                   \
    {                                                                                                                        \
        std::string errmsg = fmt::format("[{}:{} l{} E{}] " FMT, __FILE__, __func__, __LINE__, GetLastError(), __VA_ARGS__); \
        logger::error(errmsg);                                                                                               \
        throw std::exception(errmsg.c_str());                                                                                \
    }

namespace logger = spdlog;
namespace fs = std::filesystem;

#define MESSAGE_DELETE 0x111
#define DELETE_YES 0x6
#define DELETE_NO 0x2
#define RC_DELETE 0x4203

LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS *ex);

struct fn_offsets {
    uint64_t fn_deleteitems, fn_dlgproc;

    template <class Ar>
    void serialize(Ar &ar) {
        ar(fn_deleteitems, fn_dlgproc);
    }
};

#define CREATE_ERROR throw std::runtime_error(fmt::format("I'm an error at {}:{}", __FILE__, __LINE__));