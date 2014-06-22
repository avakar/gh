#ifndef STREAM_H
#define STREAM_H

#include <stdlib.h>
#include <stdint.h>
#include <vector>

typedef uint64_t file_offset_t;

class istream
{
public:
	virtual size_t read(uint8_t * p, size_t capacity) = 0;
};

class ostream
{
public:
	virtual size_t write(uint8_t const * p, size_t size) = 0;
	virtual void flush() = 0;
};

size_t read_up_to(istream & s, uint8_t * p, size_t size);
void read_all(istream & s, uint8_t * p, size_t size);
std::vector<uint8_t> read_all(istream & s, size_t size);
std::vector<uint8_t> read_all(istream & s);
bool skip(istream & s, size_t len);

void write_all(ostream & s, uint8_t const * p, size_t size);

struct isubstream
	: public istream
{
public:
	isubstream(istream & s, file_offset_t size);

	size_t read(uint8_t * p, size_t capacity) override;

private:
	istream & m_s;
	file_offset_t m_size;

	isubstream(isubstream const &);
	isubstream & operator=(isubstream const &);
};

template <typename T>
T load_be(uint8_t const * p)
{
	T res = 0;
	for (size_t i = 0; i < sizeof(T); ++i)
		res = (res << 8) | *p++;
	return res;
}

#endif // STREAM_H
