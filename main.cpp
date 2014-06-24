#include "gitdb.h"
#include "text_reader.h"
#include "file.h"
#include <time.h>
#include <iostream>

void print_stream(istream & s)
{
	stream_reader sr(s);

	std::string line;
	while (sr.read_line(line))
	{
		std::cout << line << '\n';
	}
}

static void checkout_tree(gitdb & db, string_view dir, gitdb::tree_t const & t)
{
	for (auto && te: t)
	{
		std::string name = dir.to_string() + "/" + te.name;

		if ((te.mode & 0xe000) == 0xe000)
		{
			// XXX gitlink
		}
		else if (te.mode & 0x4000)
		{
			make_directory(name);
			checkout_tree(db, name, db.get_tree(te.oid));
		}
		else
		{
			file ff(name, /*readonly=*/false);

			std::vector<uint8_t> v = db.get_blob(te.oid);

			file::ofile fo = ff.seekp(0);
			write_all(fo, v.data(), v.size());
		}
	}
}

typedef std::vector<std::string> args_t;

static std::string parse_cmd(args_t & args)
{
	bool parse_switches = true;
	for (size_t i = 0; i < args.size(); ++i)
	{
		string_view arg = args[i];

		if (arg == "--")
		{
			parse_switches = false;
		}
		else if (!parse_switches || !starts_with(arg, "-"))
		{
			std::string res = arg;
			args.erase(args.begin() + i);
			return res;
		}
	}

	return "help";
}

static bool pop_arg_value(std::string & res, args_t & args, char short_name, string_view long_name)
{
	for (size_t i = 0; i < args.size(); ++i)
	{
		string_view arg = args[i];
		if (arg == "--")
			return false;

		if (arg == long_name || (arg.size() == 2 && arg[0] == '-' && arg[1] == short_name))
		{
			if (i + 1 >= args.size())
				return false;

			res = args[i + 1];
			args.erase(args.begin() + i, args.begin() + i + 2);
			return true;
		}
	}

	return false;
}

static std::string pop_arg_value(args_t & args, char short_name, string_view long_name , string_view def)
{
	std::string res;
	if (!pop_arg_value(res, args, short_name, long_name))
		return def;
	return res;
}

static bool pop_switch(args_t & args, char short_name, string_view long_name)
{
	for (size_t i = 0; i < args.size(); ++i)
	{
		std::string & arg = args[i];
		if (arg == "--")
			return false;

		if (arg == long_name)
		{
			args.erase(args.begin() + i);
			return true;
		}

		if (starts_with(arg, "-") && !starts_with(arg, "--"))
		{
			auto it = std::find(arg.begin(), arg.end(), short_name);
			if (it != arg.end())
			{
				if (arg.size() == 2)
					args.erase(args.begin() + i);
				else
					arg.erase(it);
				return true;
			}
		}
	}

	return false;
}

static int gh_init(args_t & args)
{
	std::string repo_arg = pop_arg_value(args, 'R', "--repo", ".");
	bool bare = pop_switch(args, 0, "--bare");

	if (!bare)
	{
		std::string path = repo_arg + "/.git";
		if (!make_directory(path))
		{
			std::cerr << "error: the directory already exists: " << path << "\n";
			return 1;
		}

		gitdb::create(path);
	}
	else
	{
		gitdb::create(repo_arg);
	}

	return 0;
}

/*static std::string find_repo(string_view path)
{
	while (!path.empty())
	{
		std::string nonbare_path = path + "/.git";
		if (file::is_directory(nonbare_path))
			return nonbare_path;

		if (file::is_file(path + "/HEAD") && file::is_directory(path + "/objects") && file::is_directory(path + "/refs"))
			return path;

		path = get_path_head(path);
	}

	return path;
}

static bool open_repo(gitdb & db, string_view path)
{
	std::string repo_path = find_repo(path);
	if (repo_path.empty())
		return false;

	db.open(repo_path);
	return true;
}*/

static bool open_wd(gitdb & db, git_wd & wd, string_view path)
{
	while (!path.empty())
	{
		std::string nonbare_path = path + "/.git";
		if (file::is_directory(nonbare_path))
		{
			db.open(nonbare_path);
			wd.open(db, path);
			return true;
		}
	}
	return false;
}

static int gh_status(args_t & args)
{
	std::string repo_arg = pop_arg_value(args, 'R', "--repo", ".");

	gitdb db;
	git_wd wd;
	if (!open_wd(db, wd, repo_arg))
	{
		std::cerr << "error: not a git repository: " << repo_arg << "\n";
		return 2;
	}

	wd.status();

	return 0;
}

class timer
{
public:
	explicit timer(bool active)
	{
		m_start_time = clock();
		m_active = active;
	}

	~timer()
	{
		if (m_active)
			std::cout << "Spent time: " << (double)(clock() - m_start_time) / CLOCKS_PER_SEC << "\n";
	}

private:
	bool m_active;
	clock_t m_start_time;
};

int main(int argc, char * argv[])
{
	args_t args(&argv[1], &argv[argc]);
	std::string cmd = parse_cmd(args);

	bool profile = pop_switch(args, 0, "--time");

	try
	{
		timer tmr(profile);

		if (cmd == "init")
		{
			return gh_init(args);
		}
		else if (cmd == "test-checkout")
		{
			gitdb db0;
			db0.open(args[0]);
			object_id head_oid = db0.get_ref(args[1]);
			gitdb::commit_t cc = db0.get_commit(head_oid);
			checkout_tree(db0, args[2], db0.get_tree(cc.tree_oid));
			return 0;
		}
		else if (cmd == "status")
		{
			return gh_status(args);
		}

		return 1;
	}
	catch (std::exception const & e)
	{
		std::cerr << "error: " << e.what() << "\n";
		return 1;
	}

	return 0;
}
