#ifndef STRING_VIEW_H
#define STRING_VIEW_H

#include <string>
#include <string.h>
#include <stddef.h>

class string_view
{
public:
	static size_t const npos = (size_t)-1;

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

	std::string to_string() const
	{
		return std::string(first, last);
	}

	operator std::string() const
	{
		return std::string(first, last);
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
