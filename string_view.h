#ifndef STRING_VIEW_H
#define STRING_VIEW_H

#include <string>
#include <string.h>
#include <stddef.h>
#include <vector>

class string_view
{
public:
	static size_t const npos = (size_t)-1;

	string_view()
		: first(0), last(0)
	{
	}

	string_view(char const * str)
		: first(str), last(str + strlen(str))
	{
	}

	string_view(std::string const & str)
		: first(str.data()), last(str.data() + str.size())
	{
	}

	string_view(char const * first, char const * last)
		: first(first), last(last)
	{
	}

	char const * data() const
	{
		return first;
	}

	size_t size() const
	{
		return last - first;
	}

	bool empty() const
	{
		return first == last;
	}

	char const * begin() const
	{
		return first;
	}

	char const * end() const
	{
		return last;
	}

	char operator[](size_t i) const
	{
		return first[i];
	}

	size_t find(char ch) const
	{
		char const * p = first;
		for (; p != last; ++p)
		{
			if (*p == ch)
				break;
		}
		return p - first;
	}

	size_t rfind(char ch) const
	{
		char const * p = last;
		for (; p != first; --p)
		{
			if (p[-1] == ch)
				return p - first - 1;
		}
		return this->size();
	}

	string_view substr(size_t pos = 0, size_t n = npos) const
	{
		size_t s = this->size();
		if (pos > s)
			pos = s;
		if (n > s - pos)
			n = s - pos;

		char const * p = first + pos;
		return string_view(p, p + n);
	}

	std::vector<string_view> split(char ch)
	{
		std::vector<string_view> res;

		char const * start = first;
		for (char const * p = first; p != last; ++p)
		{
			if (*p == ch)
			{
				res.push_back(string_view(start, p));
				start = p + 1;
			}
		}

		res.push_back(string_view(start, last));
		return res;
	}

	std::string to_string() const
	{
		return std::string(first, last);
	}

	operator std::string() const
	{
		return std::string(first, last);
	}

	friend std::string operator+(string_view const & lhs, string_view const & rhs)
	{
		std::string res;
		res.resize(lhs.size() + rhs.size());
		std::copy(lhs.begin(), lhs.end(), &res[0]);
		std::copy(rhs.begin(), rhs.end(), &res[lhs.size()]);
		return res;
	}

	friend bool operator==(string_view const & lhs, string_view const & rhs)
	{
		return lhs.size() == rhs.size() && std::equal(lhs.first, lhs.last, rhs.first);
	}

	friend bool operator!=(string_view const & lhs, string_view const & rhs)
	{
		return !(lhs == rhs);
	}

private:
	char const * first;
	char const * last;
};

#include <algorithm>
inline bool starts_with(string_view s, string_view prefix)
{
	return prefix.size() <= s.size() && std::equal(prefix.begin(), prefix.end(), s.begin());
}

#endif // STRING_VIEW_H
