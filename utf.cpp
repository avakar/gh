#include "utf.h"
#include <assert.h>
#include <windows.h>
#include <stdint.h>

std::wstring to_utf16(string_view str)
{
	std::wstring res;
	if (str.empty())
		return res;

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

	if (res_size == 0)
		throw std::runtime_error("XXX");

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
			if (last - first < 1)
				throw std::runtime_error("XXX");
			*out++ = ((ch & 0x1f) << 6) | (*first++ & 0x3f);
		}
		else if (ch < 0xf0)
		{
			if (last - first < 2)
				throw std::runtime_error("XXX");
			*out++ = ((ch & 0xf) << 12) | ((first[0] & 0x3f) << 6) | (first[1] & 0x3f);
			first += 2;
		}
		else if (ch < 0xf8)
		{
			if (last - first < 3)
				throw std::runtime_error("XXX");
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

static std::string from_utf16_impl(wchar_t const * str_first, wchar_t const * last)
{
	std::string res;
	if (str_first == last || (last == 0 && *str_first == 0))
		return res;

	wchar_t const * first = str_first;

	size_t res_size = 0;
	while (first != last)
	{
		wchar_t ch = *first++;
		if (ch == 0 && last == 0)
			break;
		else if (ch < 0x80)
			res_size += 1;
		else if (ch < 0x800)
			res_size += 2;
		else if (ch < 0xD800)
			res_size += 3;
		else if (ch < 0xDC00)
			res_size += 4;
		else if (ch < 0xE000)
			;
		else
			res_size += 3;
	}

	if (res_size == 0)
		throw std::runtime_error("XXX");

	res.resize(res_size);

	char * res_first = &res[0];
	char * out = res_first;

	first = str_first;
	while (first != last)
	{
		wchar_t ch = *first++;
		if (ch == 0 && last == 0)
		{
			break;
		}
		if (ch < 0x80)
		{
			*out++ = (char)ch;
		}
		else if (ch < 0x800)
		{
			*out++ = 0xc0 | (ch >> 6);
			*out++ = 0x80 | (ch & 0x3f);
		}
		else if (ch < 0xD800)
		{
			*out++ = 0xe0 | (ch >> 12);
			*out++ = 0x80 | ((ch >> 6) & 0x3f);
			*out++ = 0x80 | (ch & 0x3f);
		}
		else if (ch < 0xDC00)
		{
			if (first == last)
				throw std::runtime_error("XXX");
			wchar_t ch2 = *first++;
			if (ch2 < 0xDC00 || ch2 >= 0xE000)
				throw std::runtime_error("XXX");

			*out++ = 0xf0 | ((ch >> 8) & 0x3);
			*out++ = 0x80 | ((ch >> 2) & 0x3f);
			*out++ = 0x80 | ((ch << 4) & 0x30) | ((ch2 >> 6) & 0xf);
			*out++ = 0x80 | (ch2 & 0x3f);
		}
		else if (ch < 0xE000)
		{
			throw std::runtime_error("XXX");
		}
		else
		{
			*out++ = 0xe0 | (ch >> 12);
			*out++ = 0x80 | ((ch >> 6) & 0x3f);
			*out++ = 0x80 | (ch & 0x3f);
		}
	}

	if (out - res_first != res_size)
		throw std::runtime_error("XXX");

	return res;
}

std::string from_utf16(wchar_t const * str)
{
	return from_utf16_impl(str, 0);
}

std::string from_utf16(wchar_t const * str, size_t size)
{
	return from_utf16_impl(str, str + size);
}
