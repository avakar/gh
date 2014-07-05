#include "utf.h"
#include <assert.h>
#include <windows.h>
#include <stdint.h>

std::wstring to_utf16(string_view str)
{
	char const * first = str.begin();
	char const * last = str.end();

	size_t res_size = 0;
	while (first != last)
	{
		char ch = *first++;

		if (ch < 0x80)
			++res_size;
		else if (ch < 0xc0)
			;
		else if (ch < 0xf0)
			++res_size;
		else
			res_size += 2;
	}

	std::wstring res;
	if (res_size == 0)
		return res;

	res.resize(res_size);

	first = str.begin();
	wchar_t * res_first = &res[0];
	wchar_t * out = res_first;
	while (first != last)
	{
		char ch = *first++;
		if (ch < 0x80)
		{
			*out++ = ch;
		}
		else if (ch < 0xc0)
		{
			throw std::runtime_error("XXX");
		}
		else if (ch < 0xe0)
		{
			*out++ = ((ch & 0x1f) << 6) | (*first++ & 0x3f);
		}
		else if (ch < 0xf0)
		{
			*out++ = ((ch & 0xf) << 12) | ((first[0] & 0x3f) << 6) | (first[1] & 0x3f);
			first += 2;
		}
		else if (ch < 0xf8)
		{
			*out++ = 0xd800 + (((ch & 0x3) << 8) | ((first[0] & 0x3f) << 2) | ((first[1] & 0x30) >> 4));
			*out++ = 0xdc00 + (((first[1] & 0xf) << 6) | (first[2] & 0x3f));
			first += 3;
		}
		else
		{
			throw std::runtime_error("XXX");
		}
	}

	if (out - res_first != res_size)
		throw std::runtime_error("XXX");

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
