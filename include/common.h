#pragma once

#include <algorithm>
#include <codecvt>
#include <iostream>
#include <locale>
#include <string>
#include <vector>
#include <filesystem>

#include <windows.h>
#include <detours.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <TlHelp32.h>
#include <psapi.h>

#include <util.h>

#define W(X) const_cast<char *>(X.c_str())
#define WW(X) const_cast<wchar_t *>(X.c_str())
#define _DETOUR_TARGET(X) hook::P##X
#define _DETOUR_DEST(X) hook::m_##X
#define DETOUR_INIT                                                            \
  DetourTransactionBegin();                                                    \
  DetourUpdateThread(GetCurrentThread());
#define DETOUR_ATTACH(X)                                                       \
  _DETOUR_TARGET(X) = X;                                                       \
  ret = DetourAttach((void **)&_DETOUR_TARGET(X), _DETOUR_DEST(X));       \

#define DETOUR_DETACH(X)                                                       \
  DetourDetach((void **)&_DETOUR_TARGET(X), _DETOUR_DEST(X));
#define DETOUR_COMMIT DetourTransactionCommit();

#define DETOUR_ATTACHEX(X)\
  ret = DetourAttach((void **)&_DETOUR_TARGET(X), _DETOUR_DEST(X)); \
  logger::debug("DetourAttach to {} returned:{}", #X, ret);

#define DEBUG(FMT, ...) logger::debug("[{}:{} l{}] " FMT, __FILE__, __func__, __LINE__, __VA_ARGS__);
#define CRITICAL(FMT, ...) logger::error("[{}:{} l{} E{}] " FMT, __FILE__, __func__, __LINE__, GetLastError(), __VA_ARGS__);

namespace logger = spdlog;

#define DLGPROC_OFFSET 0x427c60
#define DELETEITEMS_OFFSET 0x25f940
#define MESSAGE_DELETE 0x111
#define DELETE_YES 0x6
#define DELETE_NO 0x2
#define RC_DELETE 0x4203