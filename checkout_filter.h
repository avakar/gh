#ifndef CHECKOUT_FILTER_H
#define CHECKOUT_FILTER_H

#include "stream.h"

class checkin_filter
	: public istream
{
public:
	explicit checkin_filter(istream & s);

	size_t read(uint8_t * p, size_t capacity) override;

private:
	istream & m_s;

	checkin_filter(checkin_filter const &);
	checkin_filter & operator=(checkin_filter const &);
};

#endif // CHECKOUT_FILTER_H
