#ifndef GITDB_H
#define GITDB_H

#include "string_view.h"
#include "object_id.h"
#include "stream.h"
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
	};

	typedef std::vector<tree_entry_t> tree_t;

	explicit gitdb(string_view path);
	~gitdb();

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
};

#endif // GITDB_H