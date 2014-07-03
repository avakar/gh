#include "ignore.h"
#include "text_reader.h"

git_ignore::git_ignore(git_ignore const * parent)
	: m_parent(parent)
{
}

void git_ignore::load(string_view prefix, istream & fin)
{
	stream_reader sr(fin);

	std::string line;
	while (sr.read_line(line))
	{
		this->add_pattern(prefix, line);
	}
}

static bool fnmatch(string_view pattern, string_view str)
{
	char const * pat_first = pattern.begin();
	char const * pat_last = pattern.end();

	char const * str_first = str.begin();
	char const * str_last = str.end();

	for (; pat_first != pat_last; ++pat_first)
	{
		if (*pat_first == '*')
		{
			++pat_first;
			if (pat_first != pat_last && *pat_first == '*')
			{
				if (pat_first == pat_last)
					return true;

				for (char const * cur = str_first; cur != str_last; ++cur)
				{
					if (fnmatch(string_view(pat_first + 1, pat_last), string_view(cur, str_last)))
						return true;
				}
			}
			else
			{
				if (pat_first == pat_last)
					return true;

				for (char const * cur = str_first; cur != str_last && *cur != '/'; ++cur)
				{
					if (fnmatch(string_view(pat_first, pat_last), string_view(cur, str_last)))
						return true;
				}
			}

			return false;
		}

		if (str_first == str_last || *pat_first != *str_first)
			return false;

		++str_first;
	}

	return str_first == str_last;
}

bool git_ignore::match(string_view path) const
{
	for (auto && pattern : m_patterns)
	{
		if (fnmatch(pattern, path))
			return true;
	}

	if (m_parent)
		return m_parent->match(path);

	return false;
}

void git_ignore::add_pattern(string_view prefix, string_view pat)
{
	if (pat.empty() || pat[0] == '#')
		return;

	bool invert = false;
	if (pat[0] == '!')
	{
		invert = true;
		pat = pat.substr(1);
	}

	std::string unescaped;
	char const * first = pat.data();
	char const * last = first + pat.size();
	for (char const * cur = first; cur != last; ++cur)
	{
		if (*cur == '\\' && cur + 1 != last)
		{
			unescaped.append(first, cur);
			unescaped.append(1, cur[1]);
			first = cur + 2;
			++cur;
		}
	}

	while (first != last && last[-1] == ' ')
		--last;
	unescaped.append(first, last);

	if (unescaped.empty())
		return;

	if (unescaped[0] != '/')
	{
		m_patterns.push_back(prefix + unescaped);
		if (unescaped.back() != '/')
			m_patterns.push_back(prefix + unescaped + "/");

		unescaped = "**/" + unescaped;
		m_patterns.push_back(prefix + unescaped);
		if (unescaped.back() != '/')
			m_patterns.push_back(prefix + unescaped + "/");

		return;
	}

	unescaped.erase(unescaped.begin());

	if (unescaped.empty())
		return;

	m_patterns.push_back(prefix + unescaped);
	if (unescaped.back() != '/')
		m_patterns.push_back(prefix + unescaped + "/");
}
