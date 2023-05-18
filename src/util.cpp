#include <cwctype>
#include <util.h>

std::wstring s2ws(const std::string &str) {
  using convert_typeX = std::codecvt_utf8<wchar_t>;
  std::wstring_convert<convert_typeX, wchar_t> converterX;

  return converterX.from_bytes(str);
}

std::string ws2s(const std::wstring &wstr) {
  using convert_typeX = std::codecvt_utf8<wchar_t>;
  std::wstring_convert<convert_typeX, wchar_t> converterX;

  return converterX.to_bytes(wstr);
}

template <class X> bool starts_with(const X &src, const X &t) {
  return std::distance(std::find(src.begin(), src.end(), t), src.begin()) == 0;
}

template <class X> bool ends_with(const X &src, const X &t) {
  return std::distance(std::find(src.rbegin(), src.rend(), t), src.rbegin()) ==
         0;
}

void lowercase(std::string &src) {
  for (auto &ch : src) {
    ch = tolower(ch);
  }
}

void lowercase(std::wstring &src) {
  for (auto &ch : src) {
    ch = std::towlower(ch);
  }
}