#pragma once

#include <algorithm>
#include <codecvt>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>
#include <unordered_map>

#include <windows.h>
#include <detours.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <winternl.h>

#include "util.h"

namespace logger = spdlog;

#define STRING(X) #X
#define W(X) const_cast<char *>(X)
#define WW(X) const_cast<wchar_t *>(X)
#define T(X) L#X
#define CASE_STR(X)                                                            \
  case X:                                                                      \
    return #X;

extern "C" {
NTSYSAPI NTSTATUS NTAPI NtDeleteFile(POBJECT_ATTRIBUTES ObjectAttributes);
}

#define RC_DELETE 0x4203
#define BUFMAX 256

#define _DETOUR_TARGET(X) hook::P##X
#define _DETOUR_DEST(X) hook::m_##X
#define DETOUR_INIT                                                            \
  DetourTransactionBegin();                                                    \
  DetourUpdateThread(GetCurrentThread());
#define DETOUR_ATTACH(X)                                                       \
  _DETOUR_TARGET(X) = X;                                                       \
  DetourAttach((void **)&_DETOUR_TARGET(X), _DETOUR_DEST(X));
#define DETOUR_DETACH(X)                                                       \
  DetourDetach((void **)&_DETOUR_TARGET(X), _DETOUR_DEST(X));
#define DETOUR_COMMIT DetourTransactionCommit();