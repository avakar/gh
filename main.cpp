#include "gitdb.h"
#include "text_reader.h"
#include "file.h"
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

		volatile int mm = te.mode;

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

int main(int argc, char * argv[])
{
	if (argc < 4)
	{
		std::cout << "Usage: gh <repo> <ref> <checkout-target>" << std::endl;
		return 1;
	}


	gitdb db0(argv[1]);
	object_id head_oid = db0.get_ref(argv[2]);
	gitdb::commit_t cc = db0.get_commit(head_oid);
	checkout_tree(db0, argv[3], db0.get_tree(cc.tree_oid));
	return 0;
}
