#include "stream.h"
#include <stdexcept>
#include <algorithm>

isubstream::isubstream(istream & s, file_offset_t size)
	: m_s(s), m_size(size)
{
}

size_t isubstream::read(uint8_t * p, size_t capacity)
{
	if (m_size == 0)
		return 0;

	size_t r = m_s.read(p, (size_t)(std::min)(m_size, (file_offset_t)capacity));
	m_size -= r;
	return r;
}

size_t read_up_to(istream & s, uint8_t * p, size_t size)
{
	size_t read = 0;
	while (read != size)
	{
		size_t r = s.read(p + read, size - read);
		if (r == 0)
			return read;
		read += r;
	}
	return read;
}

void read_all(istream & s, uint8_t * p, size_t size)
{
	while (size != 0)
	{
		size_t r = s.read(p, size);
		if (r == 0)
			throw std::runtime_error("XXX failed read");
		p += r;
		size -= r;
	}
}

std::vector<uint8_t> read_all(istream & s, size_t size)
{
	std::vector<uint8_t> res;

	while (res.size() != size)
	{
		uint8_t buf[1024];
		size_t r = s.read(buf, (std::min)(size - res.size(), sizeof buf));
		if (r == 0)
			break;

		res.insert(res.end(), buf, buf + r);
	}

	return res;
}

std::vector<uint8_t> read_all(istream & s)
{
	std::vector<uint8_t> res;

	for (;;)
	{
		uint8_t buf[1024];
		size_t r = s.read(buf, sizeof buf);
		if (r == 0)
			break;

		res.insert(res.end(), buf, buf + r);
	}

	return res;
}

bool skip(istream & s, size_t len)
{
	uint8_t buf[1024];
	while (len)
	{
		size_t r = s.read(buf, (std::min)(len, sizeof buf));
		if (r == 0)
			return false;
		len -= r;
	}
	return true;
}

void write_all(ostream & s, uint8_t const * p, size_t size)
{
	while (size)
	{
		size_t w = s.write(p, size);
		p += w;
		size -= w;
	}
}
