#pragma once

#include "common.h"

std::wstring s2ws(const std::string &str);
std::string ws2s(const std::wstring &wstr);

template <class X> bool starts_with(const X &src, const X &t);

template <class X> bool ends_with(const X &src, const X &t);