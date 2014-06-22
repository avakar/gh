#ifndef ZLIB_STREAM_H
#define ZLIB_STREAM_H

#include "stream.h"
#include <zlib.h>

class zlib_istream
	: public istream
{
public:
	zlib_istream(istream & s);
	~zlib_istream();

	size_t read(uint8_t * p, size_t capacity) override;

private:
	istream & m_s;
	z_stream m_z;

	uint8_t m_inbuf[1024];
	size_t m_inbuf_size;

	bool m_done;

	zlib_istream(zlib_istream const &);
	zlib_istream & operator=(zlib_istream const &);
};

#endif // ZLIB_STREAM_H
