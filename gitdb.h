#ifndef GITDB_H
#define GITDB_H

#include "string_view.h"
#include "object_id.h"
#include "stream.h"
#include "ignore.h"
#include <memory>
#include <vector>
#include <map>
#include <stdint.h>

class gitdb
{
public:
	enum class object_type
	{
		none = 0,
		commit = 1,
		tree = 2,
		blob = 3,
		tag = 4,
		ofs_delta = 6,
		ref_delta = 7,
	};

	struct object
	{
		object_type type;
		size_t size;
		std::shared_ptr<istream> content;
	};

	struct commit_t
	{
		object_id tree_oid;
		std::vector<object_id> parent_oids;

		std::string author;
		uint32_t author_time;
		int16_t author_zone;

		std::string committer;
		uint32_t committer_time;
		int16_t committer_zone;

		std::string comment;
	};

	struct tree_entry_t
	{
		uint32_t mode;
		std::string name;
		object_id oid;

		tree_entry_t()
			: mode(0)
		{
		}

		tree_entry_t(tree_entry_t && o)
			: mode(o.mode), name(std::move(o.name)), oid(o.oid)
		{
		}
	};

	typedef std::vector<tree_entry_t> tree_t;

	gitdb();
	gitdb(gitdb && o);
	~gitdb();
	gitdb & operator=(gitdb && o);

	void open(string_view path);
	static void create(string_view path);

	std::vector<uint8_t> get_object_content(object_id oid, object_type req_type = object_type::none);
	object get_object(object_id oid);
	std::shared_ptr<istream> get_object_stream(object_id oid, object_type req_type = object_type::none);

	commit_t get_commit(object_id oid);
	tree_t get_tree(object_id oid);

	std::vector<uint8_t> get_blob(object_id oid);
	std::shared_ptr<istream> get_blob_stream(object_id oid);

	object_id get_ref(string_view ref);
	object_id get_ref(string_view ref, std::string & real_ref);

private:
	struct impl;
	impl * m_pimpl;

	gitdb(gitdb const &);
	gitdb & operator=(gitdb const &);
};

class git_wd
{
public:
	git_wd();
	~git_wd();
	git_wd(git_wd && o);
	git_wd & operator=(git_wd && o);

	void open(gitdb & db, string_view path);

	enum class file_status
	{
		added,
		deleted,
		modified,
		none,
	};

	std::string os_path_to_repo_path(string_view os_path);

	typedef std::map<std::string, file_status> status_t;

	void status(status_t & st, git_ignore const & ign);
	void commit_status(status_t & st, object_id const & commit_oid);
	void tree_status(status_t & st, object_id const & tree_oid);

	struct stage_tree
	{
		object_id root_tree;
		std::map<object_id, gitdb::tree_t> trees;
	};

	void make_stage_tree(stage_tree & st);

	string_view path() const;

private:
	struct impl;
	impl * m_pimpl;

	git_wd(git_wd const &);
	git_wd & operator=(git_wd const &);
};

object_id sha1(gitdb::object_type type, file_offset_t size, istream & s);
object_id sha1(gitdb::object obj);

#endif // GITDB_H
