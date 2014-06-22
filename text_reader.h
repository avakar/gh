#ifndef TEXT_READER_H
#define TEXT_READER_H

#include "stream.h"
#include <string>
#include <vector>

class stream_reader
{
public:
	stream_reader(istream & s);
	std::string read_line();
	bool read_line(std::string & line);
	std::string read_rest();

private:
	istream & m_s;
	std::vector<uint8_t> m_buffer;

	stream_reader(stream_reader const &);
	stream_reader & operator=(stream_reader const &);
};

#endif // TEXT_READER_H
