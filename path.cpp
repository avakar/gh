#include "path.h"
#include "utf.h"
#include "win_error.h"
#include "file.h"
#include <assert.h>
#include <windows.h>

template <typename Iter>
Iter clean_path_impl(Iter first, Iter last)
{
	Iter out = first;

	enum { s_start, s_mid, s_slash } state = s_start;
	for (; first != last; ++first)
	{
		typename std::iterator_traits<Iter>::value_type ch = *first;
		if (ch == '\\')
			ch = '/';

		if (ch != '/' || (state == s_start || state == s_mid))
			*out++ = ch;

		if (ch != '/')
			state = s_mid;
		else if (state != s_start)
			state = s_slash;
	}

	return out;
}

void clean_path(std::string & path)
{
	path.erase(
		clean_path_impl(path.begin(), path.end()),
		path.end());

	if (path.empty())
		path = "./";
}

std::string clean_path(string_view path)
{
	std::string res(path);
	clean_path(res);
	return res;
}

std::string current_dir()
{
	WCHAR buf[MAX_PATH + 1];
	DWORD res = ::GetCurrentDirectoryW(sizeof buf / sizeof buf[0], buf);
	if (res == 0)
		throw windows_error(::GetLastError());

	if (res <= MAX_PATH)
	{
		return from_utf16(
			buf,
			clean_path_impl(buf, buf + res) - buf);
	}

	std::vector<WCHAR> dynbuf(res);
	for (;;)
	{
		DWORD res = ::GetCurrentDirectoryW(dynbuf.size(), dynbuf.data());
		if (res == 0)
			throw windows_error(::GetLastError());

		if (res < dynbuf.size())
			return from_utf16(dynbuf.data(), clean_path_impl(dynbuf.begin(), dynbuf.begin() + res) - dynbuf.begin());

		assert(res != dynbuf.size());
		dynbuf.resize(res);
	}
}

string_view path_root(string_view path)
{
	enum { st_start, st_slash1, st_slash2, st_host, st_slash3, st_share, st_drive, st_colon } state = st_start;

	char const * first = path.begin();
	char const * last = path.end();
	for (; first != last; ++first)
	{
		char ch = *first;

		switch (state)
		{
		case st_start:
			if (ch == '/')
				state = st_slash1;
			else if (('A' <= ch && ch <= 'Z') || ('a' <= ch && ch <= 'z'))
				state = st_drive;
			else
				return string_view();
			break;
		case st_slash1:
			if (ch != '/')
				return string_view();
			state = st_slash2;
			break;
		case st_slash2:
			if (ch == '/')
				return string_view();
			state = st_host;
			break;
		case st_host:
			if (ch == '/')
				state = st_slash3;
			break;
		case st_slash3:
			if (ch == '/')
				return string_view();
			state = st_share;
			break;
		case st_share:
			if (ch == '/')
				return string_view(path.begin(), first);
			break;
		case st_drive:
			if (ch != ':')
				return string_view();
			state = st_colon;
			break;
		case st_colon:
			if (ch != '/')
				return string_view();
			return string_view(path.begin(), first);
		}
	}

	if (state != st_share && state != st_colon)
		return string_view();

	return path;
}

std::string join_paths(string_view lhs, string_view rhs)
{
	assert(!lhs.empty());
	assert(!rhs.empty());

	if ((rhs.size() >= 2 && rhs[0] == '/' && rhs[1] == '/')
		|| (rhs.size() >= 3 && rhs[1] == ':' && rhs[2] == '/'))
	{
		// `rhs` is absolute
		return rhs;
	}

	if (rhs[0] == '/')
	{
		// `rhs` is semi-absolute, e.g. `\Windows`.
		return path_root(lhs) + rhs;
	}

	if (lhs.back() == '/')
		return lhs + rhs;
	else
		return lhs + "/" + rhs;
}

string_view path_head(string_view path)
{
	char const * p = path.end();
	for (; p != path.begin(); --p)
	{
		if (p[-1] == '/')
			return string_view(path.begin(), p - 1);
	}
	return string_view();
}

string_view path_tail(string_view path)
{
	char const * p = path.end();
	for (; p != path.begin(); --p)
	{
		if (p[-1] == '/')
			return string_view(p, path.end());
	}
	return path;
}

std::string absolute_path(string_view path)
{
	return normalize_path(join_paths(current_dir(), path));
}

std::string normalize_path(string_view path)
{
	std::string res;
	res.reserve(path.size());

	char const * first = path.begin();
	char const * last = path.end();

	char const * comp_start = first;

	auto append = [&]() {
		string_view comp(comp_start, first);
		if (comp == ".")
		{
		}
		else if (comp == "..")
		{
			size_t pos = res.rfind('/');
			if (pos == std::string::npos)
				res.append("../");
			else
				res.resize(pos + 1);
		}
		else
		{
			res.append(comp);
			res.append("/");
		}

	};

	for (; first != last; ++first)
	{
		if (*first != '/')
			continue;

		append();
		comp_start = first + 1;
	}

	append();

	if (!res.empty())
		res.pop_back();
	return res;
}

std::string relative_path(string_view path)
{
	return relative_path(path, current_dir());
}

std::string relative_path(string_view path, string_view base)
{
	if (base.empty())
		return path;

	std::string nbase = normalize_path(absolute_path(base));
	std::string npath = normalize_path(absolute_path(path));

	std::string cbase = cannonical_path(nbase);
	std::string cpath = cannonical_path(npath);

	cbase.push_back('/');

	base = cbase;
	path = cpath;

	size_t common_len = 0;
	for (size_t i = 0; i < base.size(); ++i)
	{
		if (path.size() == i || base[i] != path[i])
			break;

		if (base[i] == '/')
			common_len = i + 1;
	}

	size_t rem_comps = std::count(base.begin() + common_len, base.end(), '/');

	std::string res;
	for (; rem_comps != 0; --rem_comps)
		res += "../";

	res += npath.substr(common_len);
	return std::move(res);
}

std::string cannonical_path(string_view path)
{
	assert(!path.empty());

	std::string res;
	res.resize(path.size());

	char const * first = path.begin();
	char const * last = path.end();
	char * out = &res[0];
	while (first != last)
	{
		char ch = *first++;
		if (ch & 0x80)
			throw std::runtime_error("XXX unicode not supported yet: " + path);
		if ('a' <= ch && ch <= 'z')
			ch -= 'a' - 'A';
		else if (ch == '\\')
			ch = '/';
		*out++ = ch;
	}

	return res;
}

string_view split_path_right(string_view & path)
{
	assert(!path.empty());

	char const * p = path.end();
	if (p != path.begin() && p[-1] == '/')
		--p;

	for (; p != path.begin(); --p)
	{
		if (p[-1] == '/')
		{
			string_view res(p, path.end());
			path = string_view(path.begin(), p);
			return res;
		}
	}

	string_view res;
	std::swap(res, path);
	return res;
}

string_view split_path_left(string_view & path)
{
	assert(!path.empty());

	string_view root = path_root(path);
	if (!root.empty())
	{
		path = path.substr(root.size());
		return root;
	}

	char const * p = path.begin();
	while (p != path.end() && *p == '/')
		++p;

	for (; p != path.end(); ++p)
	{
		if (*p == '/')
		{
			string_view res(path.begin(), p + 1);
			path = string_view(p + 1, path.end());
			return res;
		}
	}

	string_view res;
	std::swap(res, path);
	return res;
}

std::string find_path(string_view base_dir, string_view path)
{
	assert(!base_dir.empty());

	std::string cur = base_dir;
	if (cur.back() != '/')
		cur += "/";

	size_t base_len = cur.size();
	while (!path.empty())
	{
		string_view comp = split_path_left(path);
		bool trailing_slash = !comp.empty() && comp.back() == '/';
		if (trailing_slash)
			comp = comp.trim_right(1);

		auto des = listdir(cur, comp);
		if (des.empty())
			return std::string();

		assert(des.size() == 1);
		auto && de = des.front();

		if (trailing_slash && de.type() != dir_entry_type::directory)
			return std::string();

		assert(cur.empty() || cur.back() == '/');
		cur += de.name;
		if (trailing_slash)
			cur += '/';
	}

	return string_view(cur).substr(base_len);
}
