#include "cmdline.h"
#include "assert.h"

cmdline::cmdline(cmdline_entry const * entry_first, cmdline_entry const * entry_last, int argc, char const * const argv[])
	: m_entry_first(entry_first), m_entry_last(entry_last)
{
	bool switches_enabled = true;
	for (int i = 0; i < argc; ++i)
	{
		string_view arg = argv[i];

		if (arg == "--")
		{
			switches_enabled = false;
			continue;
		}

		if (switches_enabled && starts_with(arg, "--"))
		{
			bool handled = false;
			for (cmdline_entry const * entry = entry_first; !handled && entry != entry_last; ++entry)
			{
				if (!entry->long_name.empty() && arg == entry->long_name)
				{
					std::vector<std::string> & opts = m_opts[entry->id];
					for (int j = 0; j < entry->arg_count; ++j)
					{
						if (++i >= argc)
							throw std::runtime_error("XXX " + arg + " needs an argument");
						opts.push_back(argv[i]);
					}

					handled = true;
				}
			}

			if (!handled)
				throw std::runtime_error("XXX unknown option: " + arg);

			continue;
		}

		if (switches_enabled && arg.size() > 1 && starts_with(arg, "-"))
		{
			for (auto ch: arg.substr(1))
			{
				bool handled = false;
				for (cmdline_entry const * entry = entry_first; !handled && entry != entry_last; ++entry)
				{
					if (entry->short_name && entry->short_name == ch)
					{
						std::vector<std::string> & opts = m_opts[entry->id];
						for (int j = 0; j < entry->arg_count; ++j)
						{
							if (++i >= argc)
								throw std::runtime_error(std::string("XXX -") + ch + " needs an argument");
							opts.push_back(argv[i]);
						}

						handled = true;
					}
				}

				if (!handled)
					throw std::runtime_error("XXX unknown option: -" + ch);
			}

			continue;
		}

		bool handled = false;
		for (cmdline_entry const * entry = entry_first; !handled && entry != entry_last; ++entry)
		{
			if (entry->subparser == 0 && entry->short_name == 0 && !starts_with(entry->long_name, "--") && m_opts.find(entry->id) == m_opts.end())
			{
				assert(entry->arg_count == 1);
				m_opts[entry->id].push_back(arg);
				handled = true;
			}
		}

		if (!handled)
			m_additional_opts.push_back(arg);
	}
}

size_t cmdline::has_arg(int id) const
{
	auto it = m_opts.find(id);
	if (it == m_opts.end())
		return 0;
	return it->second.size();
}

std::string cmdline::pop_string(int id)
{
	auto it = m_opts.find(id);
	if (it == m_opts.end() || it->second.empty())
		throw std::runtime_error("XXX missing option: " + m_entry_first[id].long_name);

	std::vector<std::string> & opts = m_opts[id];
	std::string res = opts.front();
	opts.erase(opts.begin());
	return res;
}

bool cmdline::pop_string(int id, std::string & res)
{
	auto it = m_opts.find(id);
	if (it == m_opts.end() || it->second.empty())
		return false;

	std::vector<std::string> & opts = m_opts[id];
	res = opts.front();
	opts.erase(opts.begin());
	return true;
}

std::string cmdline::pop_string(int id, string_view def)
{
	std::string res;
	if (this->pop_string(id, res))
		return res;
	return def;
}

bool cmdline::pop_switch(int id)
{
	auto it = m_opts.find(id);
	if (it == m_opts.end())
		return false;

	assert(it->second.empty());
	m_opts.erase(it);
	return true;
}

void cmdline::set_subparser(int subparser_id)
{
	size_t cur_arg = 0;
	for (cmdline_entry const * entry = m_entry_first; entry != m_entry_last; ++entry)
	{
		if (entry->subparser == subparser_id && entry->short_name == 0 && !starts_with(entry->long_name, "--"))
		{
			if (cur_arg < m_additional_opts.size())
				m_opts[entry->id].push_back(m_additional_opts[cur_arg]);
			else
				throw std::runtime_error("XXX missing argument: " + entry->long_name);
		}
	}

	m_additional_opts.erase(m_additional_opts.begin(), m_additional_opts.begin() + cur_arg);
}

std::vector<std::string> const & cmdline::args() const
{
	return m_additional_opts;
}
