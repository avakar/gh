#ifndef FILE_H
#define FILE_H

#include "string_view.h"
#include "stream.h"
#include <stdint.h>

class file
{
public:
	file();
	file(string_view path, bool readonly);
	file(file && o);
	~file();
	file & operator=(file && o);

	void open(string_view path, bool readonly);
	bool try_open(string_view path, bool readonly);
	void close();
	bool is_open() const;

	class ifile
		: public istream
	{
	public:
		ifile(file * f, file_offset_t pos)
			: m_file(f), m_pos(pos)
		{
		}

		size_t read(uint8_t * p, size_t capacity) override
		{
			size_t r = m_file->read_abs(m_pos, p, capacity);
			m_pos += r;
			return r;
		}

	private:
		file * m_file;
		file_offset_t m_pos;
	};

	class ofile
		: public ostream
	{
	public:
		ofile(file * f, file_offset_t pos)
			: m_file(f), m_pos(pos)
		{
		}

		size_t write(uint8_t const * p, size_t size) override
		{
			size_t r = m_file->write_abs(m_pos, p, size);
			m_pos += r;
			return r;
		}

		void flush() override
		{
		}

	private:
		file * m_file;
		file_offset_t m_pos;
	};

	size_t read_abs(file_offset_t pos, uint8_t * p, size_t capacity);
	size_t write_abs(file_offset_t pos, uint8_t const * p, size_t capacity);
	ifile seekg(file_offset_t pos);
	ofile seekp(file_offset_t pos);

private:
	intptr_t m_fd;

	file(file const &);
	file & operator=(file const &);
};

class dir_enum_proxy
{
public:
	class iterator
	{
	public:
		explicit iterator(dir_enum_proxy * pimpl);

		std::string operator*() const;
		iterator & operator++();
		iterator operator++(int);

		bool operator==(iterator rhs)
		{
			auto * l = m_pimpl? m_pimpl->m_pimpl: 0;
			auto * r = rhs.m_pimpl? rhs.m_pimpl->m_pimpl: 0;
			return l == r;
		}

		bool operator!=(iterator rhs)
		{
			return !(*this == rhs);
		}

	private:
		dir_enum_proxy * m_pimpl;
	};

	dir_enum_proxy(string_view path, string_view mask);
	dir_enum_proxy(dir_enum_proxy && o);
	~dir_enum_proxy();

	iterator begin();
	iterator end();

private:
	struct impl;
	impl * m_pimpl;

	dir_enum_proxy(dir_enum_proxy const &);
	dir_enum_proxy & operator=(dir_enum_proxy const &);
};

inline dir_enum_proxy listdir(string_view path, string_view mask = "*")
{
	return dir_enum_proxy(path, mask);
}

bool make_directory(string_view path);

#endif // FILE_H
