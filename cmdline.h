#ifndef CMDLINE_H
#define CMDLINE_H

#include "string_view.h"
#include <map>
#include <vector>

struct cmdline_entry
{
	int id;
	char short_name;
	string_view long_name;
	int arg_count;
	int subparser;
	string_view desc;
};

class cmdline
{
public:
	cmdline(cmdline_entry const * entry_first, cmdline_entry const * entry_last, int argc, char const * const argv[]);

	size_t has_arg(int id) const;
	std::string pop_string(int id);
	bool pop_string(int id, std::string & res);
	std::string pop_string(int id, string_view def);
	bool pop_switch(int id);

	void set_subparser(int subparser_id);

	std::vector<std::string> const & args() const;

private:
	cmdline_entry const * m_entry_first;
	cmdline_entry const * m_entry_last;
	std::map<int, std::vector<std::string> > m_opts;
	std::vector<std::string> m_additional_opts;
};

#endif // CMDLINE_H
