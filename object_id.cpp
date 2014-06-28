#include "object_id.h"
#include "sha1.h"
#include <assert.h>
#include <algorithm>

static uint8_t _from_ch(char digit)
{
	if ('0' <= digit && digit <= '9')
		return digit - '0';
	else if ('a' <= digit && digit <= 'f')
		return digit - 'a' + 10;
	else
		return digit - 'A' + 10;
}

object_id::object_id()
{
	std::fill(id, id + sizeof id, 0);
}

object_id::object_id(uint8_t const * name)
{
	std::copy(name, name + 20, id);
}

object_id::object_id(string_view name)
{
	assert(name.size() == 40);
	for (size_t i = 0; i < 20; ++i)
		id[i] = (_from_ch(name[i * 2]) << 4) | _from_ch(name[i * 2 + 1]);
}

std::string object_id::base16() const
{
	char res[40];
	for (size_t i = 0; i < 20; ++i)
	{
		static char const digits[] = "0123456789abcdef";
		res[i * 2] = digits[id[i] >> 4];
		res[i * 2 + 1] = digits[id[i] & 0xf];
	}

	return std::string(res, res + 40);
}

uint8_t object_id::operator[](size_t i) const
{
	return id[i];
}

bool operator==(object_id const & lhs, object_id const & rhs)
{
	return std::equal(lhs.id, lhs.id + sizeof lhs.id, rhs.id);
}

bool operator!=(object_id const & lhs, object_id const & rhs)
{
	return !(lhs == rhs);
}

bool operator<(object_id const & lhs, object_id const & rhs)
{
	return std::memcmp(lhs.id, rhs.id, sizeof lhs.id) < 0;
}

object_id sha1(string_view data)
{
	uint8_t hash[20];
	sha1(hash, data);
	return object_id(hash);
}

object_id sha1(istream & s)
{
	uint8_t hash[20];
	sha1(hash, s);
	return object_id(hash);
}

uint8_t const * object_id::begin() const
{
	return id;
}

uint8_t const * object_id::end() const
{
	return id + sizeof id;
}
