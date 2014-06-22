#include "gitdb.h"
#include "file.h"
#include "text_reader.h"
#include "zlib_stream.h"
#include <memory>
#include <map>

static size_t get_variant(uint8_t const *& p, uint8_t const * last)
{
	size_t res = 0;
	do
	{
		res = (res << 7) | (*p & 0x7f);
	}
	while (*p++ & 0x80 && p != last);
	return res;
}

class patcher
	: public istream
{
public:
	patcher(istream & base, istream & delta)
		: m_pos(0)
	{
		std::vector<uint8_t> bb = read_all(base);
		std::vector<uint8_t> dd = read_all(delta);

		uint8_t const * p = dd.data();
		uint8_t const * last = dd.data() + dd.size();

#if 0
		size_t src_size = get_variant(p, last);
		size_t dst_size = get_variant(p, last);
#else
		get_variant(p, last);
		get_variant(p, last);
#endif

		while (p != last)
		{
			uint8_t cmd = *p++;
			if (cmd & 0x80)
			{
				uint32_t offs = 0;
				uint32_t size = 0;

				if (cmd & 0x01)
					offs = *p++;
				if (cmd & 0x02)
					offs |= ((uint32_t)*p++ << 8);
				if (cmd & 0x04)
					offs |= ((uint32_t)*p++ << 16);
				if (cmd & 0x08)
					offs |= ((uint32_t)*p++ << 24);

				if (cmd & 0x10)
					size = *p++;
				if (cmd & 0x20)
					size |= ((uint32_t)*p++ << 8);
				if (cmd & 0x40)
					size |= ((uint32_t)*p++ << 16);
				if (size == 0)
					size = 0x10000;

				patched.insert(patched.end(), bb.data() + offs, bb.data() + offs + size);
			}
			else
			{
				patched.insert(patched.end(), p, p + cmd);
				p += cmd;
			}
		}
	}

	size_t read(uint8_t * p, size_t capacity) override
	{
		if (capacity > patched.size() - m_pos)
			capacity = patched.size() - m_pos;

		std::copy(patched.begin() + m_pos, patched.begin() + m_pos + capacity, p);
		m_pos += capacity;
		return capacity;
	}

private:
	std::vector<uint8_t> patched;
	size_t m_pos;
};

namespace {

struct object_pack
{
	file idx;
	file pack;
	uint32_t fanout_table[256];

	gitdb::object get_object(object_id oid, gitdb::object_type req_type);
	gitdb::object get_object(file_offset_t offs, gitdb::object_type req_type);
};

struct loose_stream
	: public istream
{
	loose_stream(file && f)
		: m_file(std::move(f)), m_f(m_file.seekg(0)), z(m_f)
	{
	}

	size_t read(uint8_t * p, size_t capacity) override
	{
		return z.read(p, capacity);
	}

	file m_file;
	file::ifile m_f;
	zlib_istream z;
};

struct packed_stream
	: public istream
{
	explicit packed_stream(file::ifile f)
		: m_f(f), m_z(m_f)
	{
	}

	size_t read(uint8_t * p, size_t capacity) override
	{
		return m_z.read(p, capacity);
	}

	file::ifile m_f;
	zlib_istream m_z;
};

struct patch_stream
	: public istream
{
	patch_stream(std::shared_ptr<istream> base, file::ifile delta)
		: m_base(std::move(base)), m_delta(delta), m_delta_z(m_delta), m_patcher(*m_base, m_delta_z)
	{
	}

	size_t read(uint8_t * p, size_t capacity) override
	{
		return m_patcher.read(p, capacity);
	}

	std::shared_ptr<istream> m_base;
	file::ifile m_delta;
	zlib_istream m_delta_z;
	patcher m_patcher;
};

}

gitdb::object object_pack::get_object(object_id oid, gitdb::object_type req_type)
{
	size_t offs0 = oid[0]? fanout_table[oid[0] - 1]: 0;
	size_t offs1 = fanout_table[oid[0]];

	// naturally, I/O-aware binary search should happen here instead

	auto idx_r = idx.seekg(256 * 4 + 8 + 20 * offs0);

	bool found = false;
	while (!found && offs0 < offs1)
	{
		size_t chunk = (std::min)(offs1 - offs0, size_t(1024));

		uint8_t oids[1024][20];
		read_all(idx_r, oids[0], 20 * chunk);

		for (size_t i = 0; i < chunk; ++i)
		{
			if (oid == oids[i])
			{
				offs0 += i;
				found = true;
				break;
			}
		}

		if (!found)
			offs0 += chunk;
	}

	if (!found)
		return gitdb::object();

	idx_r = idx.seekg(8 + 256 * 4 + fanout_table[0xff] * 24 + 4 * offs0);

	uint8_t offs_buf[4];
	read_all(idx_r, offs_buf, sizeof offs_buf);

	file_offset_t offs = load_be<uint32_t>(offs_buf);
	if (offs & 0x80000000)
	{
		// XXX: todo
	}

	return this->get_object(offs, req_type);
}

gitdb::object object_pack::get_object(file_offset_t offs, gitdb::object_type req_type)
{
	uint8_t buf[32];

	file::ifile packi = pack.seekg(offs);
	read_all(packi, buf, sizeof buf);

	uint8_t * p = buf;
	gitdb::object_type type = static_cast<gitdb::object_type>((buf[0] >> 4) & 7);
	uint32_t size = buf[0] & 15;
	size_t shift = 4;

	while (*p++ & 0x80)
	{
		size |= (*p & 0x7f) << shift;
		shift += 7;
	}

	if (type == gitdb::object_type::ofs_delta)
	{
		uint32_t neg_offset = *p & 0x7f;
		while (*p++ & 0x80)
			neg_offset = ((neg_offset + 1) << 7) | (*p & 0x7f);

		gitdb::object base_obj = this->get_object(offs - neg_offset, req_type);
		if (!base_obj.content)
			return gitdb::object();

		gitdb::object obj;
		obj.content = std::make_shared<patch_stream>(std::move(base_obj.content), pack.seekg(offs + (p - buf)));
		obj.type = base_obj.type;
		return obj;
	}
	else
	{
		if (req_type != gitdb::object_type::none && type != req_type)
			return gitdb::object();

		gitdb::object obj;
		obj.content = std::make_shared<packed_stream>(pack.seekg(offs + (p - buf)));
		obj.type = type;
		return obj;
	}
}

struct gitdb::impl
{
	std::string m_path;

	struct ref_cache_line
	{
		std::string real_ref;
		object_id oid;
	};

	std::map<std::string, ref_cache_line> m_ref_cache;
	bool m_packed_refs_loaded;
	void load_packed_refs();
	void load_packed_refs(istream & fin);

	std::map<std::string, object_pack> m_packs;
	bool m_packs_loaded;
	void load_pack(string_view path);
};

void gitdb::impl::load_pack(string_view path)
{
	object_pack & op = m_packs[path.to_string()];
	op.idx.open(path.to_string() + ".idx", /*readonly=*/true);
	op.pack.open(path.to_string() + ".pack", /*readonly=*/true);

	uint8_t header[8 + 256 * 4];
	file::ifile idxi = op.idx.seekg(0);
	read_all(idxi, header, sizeof header);

	if (header[0] != 0xff || header[1] != 't' || header[2] != 'O' || header[3] != 'c')
		throw std::runtime_error("invalid pack");

	uint32_t ver = load_be<uint32_t>(header + 4);
	if (ver != 2)
		throw std::runtime_error("invalid pack");

	uint8_t const * pfanout = header + 8;
	for (size_t i = 0; i < 256; ++i)
	{
		op.fanout_table[i] = load_be<uint32_t>(pfanout);
		pfanout += 4;
	}
}

void gitdb::impl::load_packed_refs()
{
	file fin;
	if (!fin.try_open(m_path + "/packed-refs", /*readonly=*/true))
		return;

	file::ifile fini = fin.seekg(0);
	this->load_packed_refs(fini);
	m_packed_refs_loaded = true;
}

void gitdb::impl::load_packed_refs(istream & fin)
{
	stream_reader r(fin);

	std::string line;
	while (r.read_line(line))
	{
		if (starts_with(line, "#") || starts_with(line, "^"))
			continue;

		if (line.size() < 42 || line[40] != ' ')
			throw std::runtime_error("XXX invalid packed-refs format");

		ref_cache_line cl;
		cl.real_ref = line.substr(41);
		cl.oid = object_id(string_view(line).substr(0, 40));
		m_ref_cache[cl.real_ref] = cl;
	}
}

gitdb::gitdb(string_view path)
{
	std::unique_ptr<impl> pimpl(new impl());
	pimpl->m_path = path;
	pimpl->m_packed_refs_loaded = false;
	pimpl->m_packs_loaded = false;
	m_pimpl = pimpl.release();
}

gitdb::~gitdb()
{
	delete m_pimpl;
}

std::vector<uint8_t> gitdb::get_object_content(object_id oid, object_type type)
{
	std::shared_ptr<istream> ss = this->get_object_stream(oid, type);
	if (!ss)
		throw std::runtime_error("XXX oid not found");
	return read_all(*ss);
}

std::shared_ptr<istream> gitdb::get_object_stream(object_id oid, object_type req_type)
{
	object obj = this->get_object(oid);
	if (obj.type != req_type)
		return nullptr;
	return obj.content;
}

gitdb::object gitdb::get_object(object_id oid)
{
	std::string s = oid.base16();
	file f;

	if (!f.try_open(m_pimpl->m_path + "/objects/" + s.substr(0, 2) + "/" + s.substr(2), /*readonly=*/true))
	{
		if (!m_pimpl->m_packs_loaded)
		{
			for (auto && name : listdir(m_pimpl->m_path + "/objects/pack", "*.idx"))
				m_pimpl->load_pack(m_pimpl->m_path + "/objects/pack/" + name.substr(0, name.size() - 4));
			m_pimpl->m_packs_loaded = true;
		}

		for (auto && kv: m_pimpl->m_packs)
		{
			gitdb::object os = kv.second.get_object(oid, object_type::none);
			if (os.content)
				return os;
		}

		return object();
	}

	object obj;
	size_t nul_pos;
	{
		file::ifile fi(f.seekg(0));
		zlib_istream z(fi);

		uint8_t buf[64];
		size_t r = read_up_to(z, buf, 64);
		nul_pos = std::find(buf, buf + r, 0) - buf;
		if (nul_pos == r)
			throw std::runtime_error("XXX invalid header");

		r = std::find(buf, buf + nul_pos, ' ') - buf;
		if (r == nul_pos)
			throw std::runtime_error("XXX invalid header");

		string_view type_str((char const *)buf, (char const *)buf + r);
		if (type_str == "commit")
			obj.type = object_type::commit;
		else if (type_str == "tree")
			obj.type = object_type::tree;
		else if (type_str == "blob")
			obj.type = object_type::blob;
		else if (type_str == "tag")
			obj.type = object_type::tag;
		else
			throw std::runtime_error("XXX unknown loose object type");
	}

	obj.content = std::make_shared<loose_stream>(std::move(f));
	skip(*obj.content, nul_pos+1);
	return obj;
}

object_id gitdb::get_ref(string_view ref)
{
	std::string real_ref;
	return this->get_ref(ref, real_ref);
}

object_id gitdb::get_ref(string_view ref, std::string & real_ref)
{
	real_ref = ref;
	for (;;)
	{
		auto cache_it = m_pimpl->m_ref_cache.find(real_ref);
		if (cache_it != m_pimpl->m_ref_cache.end())
		{
			real_ref = cache_it->second.real_ref;
			return cache_it->second.oid;
		}

		file f;
		if (!f.try_open(m_pimpl->m_path + "/" + real_ref, /*readonly=*/true))
		{
			if (m_pimpl->m_packed_refs_loaded)
				throw std::runtime_error("unknown ref XXX");

			m_pimpl->load_packed_refs();
			continue;
		}

		file::ifile fi = f.seekg(0);
		stream_reader r(fi);

		std::string line = r.read_line();
		if (starts_with(line, "ref: "))
		{
			real_ref = line.substr(5);
			continue;
		}

		impl::ref_cache_line cl;
		cl.real_ref = real_ref;
		cl.oid = object_id(line);

		m_pimpl->m_ref_cache[ref.to_string()] = cl;
		m_pimpl->m_ref_cache[real_ref] = cl;
		return cl.oid;
	}
}

static void parse_name_time(string_view name_time, std::string & name, uint32_t & time, int16_t & zone)
{
	int space_count = 0;
	for (size_t i = name_time.size(); i != 0; --i)
	{
		if (name_time[i - 1] == ' ')
		{
			++space_count;

			switch (space_count)
			{
			case 1:
				// zone
				zone = (int16_t)atoi(name_time.data() + i);
				break;
			case 2:
				// time
				time = atoi(name_time.data() + i);
				name.assign(name_time.data(), name_time.data() + i - 1);
				return;
			}
		}
	}

	throw std::runtime_error("XXX malformed name-time");
}

gitdb::commit_t gitdb::get_commit(object_id oid)
{
	gitdb::commit_t res;
	std::shared_ptr<istream> ss = this->get_object_stream(oid, object_type::commit);
	stream_reader sr(*ss);

	std::string line;
	while (sr.read_line(line))
	{
		if (line.empty())
			break;

		string_view l(line);
		if (starts_with(l, "tree "))
			res.tree_oid = object_id(l.substr(5));
		else if (starts_with(l, "parent "))
			res.parent_oids.push_back(object_id(l.substr(7)));
		else if (starts_with(l, "author "))
			parse_name_time(l.substr(7), res.author, res.author_time, res.author_zone);
		else if (starts_with(l, "committer "))
			parse_name_time(l.substr(10), res.committer, res.committer_time, res.committer_zone);
		else
			std::runtime_error("XXX unknown commit object header");
	}

	res.comment = sr.read_rest();
	return res;
}

gitdb::tree_t gitdb::get_tree(object_id oid)
{
	gitdb::tree_t res;

	std::vector<uint8_t> cnt = this->get_object_content(oid, object_type::tree);
	uint8_t const * p = cnt.data();
	uint8_t const * last = p + cnt.size();

	while (p != last)
	{
		uint8_t const * nul_pos = std::find(p, last, 0);
		if (last - nul_pos < 21)
			throw std::runtime_error("XXX malformed tree object");

		uint8_t const * sp_pos = std::find(p, nul_pos, ' ');
		if (sp_pos == nul_pos)
			throw std::runtime_error("XXX malformed tree object");

		tree_entry_t te;
		te.mode = strtol((char const *)p, 0, 8);
		te.name.assign(sp_pos + 1, nul_pos);
		te.oid = object_id(nul_pos + 1);
		res.push_back(std::move(te));

		p = nul_pos + 21;
	}

	return res;
}

std::vector<uint8_t> gitdb::get_blob(object_id oid)
{
	return this->get_object_content(oid, object_type::blob);
}

std::shared_ptr<istream> gitdb::get_blob_stream(object_id oid)
{
	return this->get_object_stream(oid, object_type::blob);
}
