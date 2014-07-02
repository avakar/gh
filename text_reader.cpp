#include "text_reader.h"
#include <algorithm>

stream_reader::stream_reader(istream & s)
	: m_s(s)
{
}

std::string stream_reader::read_line()
{
	std::string res;
	this->read_line(res);
	return res;
}

bool stream_reader::read_line(std::string & line)
{
	for (;;)
	{
		auto eol_it = std::find(m_buffer.begin(), m_buffer.end(), '\n');
		if (eol_it != m_buffer.end())
		{
			line.assign(m_buffer.begin(), eol_it);
			line.erase(
				std::remove(line.begin(), line.end(), '\r'),
				line.end());
			m_buffer.erase(m_buffer.begin(), std::next(eol_it));
			return true;
		}

		size_t buf_size = m_buffer.size();
		m_buffer.resize(buf_size + 1024);

		size_t r = m_s.read(m_buffer.data() + buf_size, m_buffer.size() - buf_size);
		if (r == 0)
		{
			line.assign(m_buffer.begin(), m_buffer.begin() + buf_size);
			line.erase(
				std::remove(line.begin(), line.end(), '\r'),
				line.end());
			m_buffer.clear();
			return !line.empty();
		}

		m_buffer.resize(buf_size + r);
	}
}

std::string stream_reader::read_rest()
{
	std::string res;
	res.append(m_buffer.begin(), m_buffer.end());
	m_buffer.clear();

	for (;;)
	{
		uint8_t buf[1024];
		size_t r = m_s.read(buf, sizeof buf);
		if (r == 0)
			break;
		res.append(buf, buf + r);
	}

	return res;
}
