#include "utf.h"
#include <assert.h>
#include <windows.h>

std::wstring to_utf16(string_view str)
{
	std::wstring res;
	if (str.empty())
		return res;

	int needed = ::MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), 0, 0);
	assert(needed > 0);

	res.resize(needed);
	int written = ::MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), &res[0], needed);
	assert(written == needed);
	return res;
}

std::string from_utf16(wchar_t const * str)
{
	return from_utf16(str, wcslen(str));
}

std::string from_utf16(wchar_t const * str, size_t size)
{
	std::string res;
	if (size == 0)
		return res;

	int needed = ::WideCharToMultiByte(CP_UTF8, 0, str, size, 0, 0, 0, 0);
	assert(needed > 0);

	res.resize(needed);
	int written = ::WideCharToMultiByte(CP_UTF8, 0, str, size, &res[0], needed, 0, 0);
	assert(written == needed);
	return res;
}
