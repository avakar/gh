#ifndef UTF_H
#define UTF_H

#include "string_view.h"
#include <string>

std::wstring to_utf16(string_view str);
std::string from_utf16(wchar_t const * str);
std::string from_utf16(wchar_t const * str, size_t size);

#endif // UTF_H
