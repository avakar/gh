#include "zlib_stream.h"
#include <stdexcept>

class zlib_error
	: public std::runtime_error
{
public:
	zlib_error(int rc)
		: std::runtime_error("zlib_error"), m_rc(rc)
	{
	}

private:
	int m_rc;
};

zlib_istream::zlib_istream(istream & s)
	: m_s(s), m_z(), m_done(false)
{
	int r = inflateInit(&m_z);
	if (r != Z_OK)
		throw zlib_error(r);
}

zlib_istream::~zlib_istream()
{
	inflateEnd(&m_z);
}

size_t zlib_istream::read(uint8_t * p, size_t capacity)
{
	m_z.next_out = p;
	m_z.avail_out = capacity;

	while (!m_done && m_z.next_out == p)
	{
		if (m_z.avail_in == 0)
		{
			m_inbuf_size = m_s.read(m_inbuf, sizeof m_inbuf);
			m_z.next_in = m_inbuf;
			m_z.avail_in = m_inbuf_size;
		}

		int r = inflate(&m_z, 0);
		switch (r)
		{
		case Z_OK:
			break;
		case Z_STREAM_END:
			m_done = true;
			break;
		default:
			throw zlib_error(r);
		}
	}

	return m_z.next_out - p;
}
