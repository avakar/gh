#include "checkout_filter.h"
#include <algorithm>

checkin_filter::checkin_filter(istream & s)
	: m_s(s)
{
}

size_t checkin_filter::read(uint8_t * p, size_t capacity)
{
	size_t r = 0;
	while (r == 0)
	{
		r = m_s.read(p, capacity);
		if (r == 0)
			break;

		r = std::remove(p, p + r, '\r') - p;
	}

	return r;
}
