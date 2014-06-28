#include "gitdb.h"
#include "text_reader.h"
#include "file.h"
#include <time.h>
#include <iostream>

#include "cmdline.h"
#include "console.h"

namespace gh_opts {
	enum
	{
		cmd,
		repo,
		time,
		bare,
		wd_dir,
		ref,
	};
};

namespace gh_subparser {
	enum
	{
		none,
		init,
		test_checkout,
		status,
	};
}

static cmdline_entry const cmdline_entries[] =
{
	{ gh_opts::cmd, 0, "command", "", 1, 0, "the command to execute" },
	{ gh_opts::repo, 'R', "--repo", ".", 1, 0, "the path to the repository or inside a working dir" },
	{ gh_opts::time, 0, "--time", "", 0, 0, "print the total time spent executing gh" },

	{ gh_opts::bare, 0, "--bare", "", 0, gh_subparser::init, "create a bare repo" },

	{ gh_opts::wd_dir, 0, "wd_dir", "", 0, gh_subparser::test_checkout, "" },
	{ gh_opts::ref, 0, "ref", "", 0, gh_subparser::test_checkout, "" },
};

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

static int gh_init(cmdline & args)
{
	std::string repo_arg = args.pop_string(gh_opts::repo);
	bool bare = args.pop_switch(gh_opts::bare);

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

static void print_status(std::map<std::string, git_wd::file_status> const & fs, bool untracked)
{
	for (auto && kv : fs)
	{
		char const * stati = untracked? "?DM": "ADM";
		uint8_t const colors[] = { untracked? 0xe: 0xa, 0xa, 0xc };

		console_color_guard concolor(colors[static_cast<int>(kv.second)]);
		std::cout << stati[static_cast<int>(kv.second)] << " " << kv.first << "\n";
	}
}

static int gh_status(cmdline & args)
{
	std::string repo_arg = args.pop_string(gh_opts::repo);

	gitdb db;
	git_wd wd;
	if (!open_wd(db, wd, repo_arg))
	{
		std::cerr << "error: not a git repository: " << repo_arg << "\n";
		return 2;
	}

	std::string real_ref;
	object_id head_oid = db.get_ref("HEAD", real_ref);
	std::cout << "B " << real_ref << "\n";

	std::map<std::string, git_wd::file_status> fs;

	wd.commit_status(fs, head_oid);
	if (!fs.empty())
	{
		print_status(fs, /*untracked=*/false);
		std::cout << "\n";
		fs.clear();
	}

	wd.status(fs);
	print_status(fs, /*untracked=*/true);

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
	cmdline args(cmdline_entries, std::end(cmdline_entries), argc - 1, argv + 1);

	bool profile = args.pop_switch(gh_opts::time);
	std::string cmd = args.pop_string(gh_opts::cmd);

	try
	{
		timer tmr(profile);

		if (cmd == "init")
		{
			return gh_init(args);
		}
		else if (cmd == "test-checkout")
		{
			args.set_subparser(gh_subparser::test_checkout);

			gitdb db0;
			db0.open(args.pop_string(gh_opts::repo, "."));

			object_id head_oid = db0.get_ref(args.pop_string(gh_opts::ref));
			gitdb::commit_t cc = db0.get_commit(head_oid);
			checkout_tree(db0, args.pop_string(gh_opts::wd_dir), db0.get_tree(cc.tree_oid));
			return 0;
		}
		else if (cmd == "st" || cmd == "status")
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
