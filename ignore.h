#ifndef IGNORE_H
#define IGNORE_H

#include "string_view.h"
#include "stream.h"
#include <vector>
#include <string>

class git_ignore
{
public:
	explicit git_ignore(git_ignore const * parent = 0);

	void load(string_view prefix, istream & fin);
	void add_pattern(string_view prefix, string_view pat);
	bool match(string_view path) const;

private:
	git_ignore const * m_parent;
	std::vector<std::string> m_patterns;
};

#endif // IGNORE_H
