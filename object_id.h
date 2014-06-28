#ifndef OBJECT_ID_H
#define OBJECT_ID_H

#include "string_view.h"
#include "stream.h"
#include <string>
#include <stdint.h>

class object_id
{
public:
	object_id();
	object_id(uint8_t const * name);
	object_id(string_view name);

	std::string base16() const;
	uint8_t operator[](size_t i) const;

	uint8_t const * begin() const;
	uint8_t const * end() const;

	friend bool operator==(object_id const & lhs, object_id const & rhs);
	friend bool operator!=(object_id const & lhs, object_id const & rhs);

	friend bool operator<(object_id const & lhs, object_id const & rhs);


private:
	uint8_t id[20];
};

object_id sha1(string_view data);
object_id sha1(istream & s);

#endif // OBJECT_ID_H
