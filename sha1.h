#ifndef SHA1_H
#define SHA1_H

#include "string_view.h"
#include "stream.h"
#include <stdint.h>

class sha1_state
{
public:
	sha1_state();

	void reset();
	void add(uint8_t const * first, uint8_t const * last);
	void finish(uint8_t * hash);

private:
	uint32_t m_h[5];
	uint8_t m_block[64];
	uint64_t m_message_len;
};

void sha1(uint8_t * hash, string_view data);
void sha1(uint8_t * hash, istream & s);

#endif // SHA1_H
