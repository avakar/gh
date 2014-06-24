#include "sha1.h"
#include "stream.h"

static uint32_t lrot(uint32_t x, int s)
{
	return _rotl(x, s);
}

static void transform_block(uint32_t (&h)[5], uint8_t const block[64])
{
	uint32_t w[80];
	for (int i = 0; i < 16; ++i)
	{
		w[i] = load_be<uint32_t>(block);
		block += 4;
	}

	for (int i = 16; i < 80; ++i)
		w[i] = lrot((w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]), 1);

	uint32_t a = h[0];
	uint32_t b = h[1];
	uint32_t c = h[2];
	uint32_t d = h[3];
	uint32_t e = h[4];

	for (int i = 0; i < 80; ++i)
	{
		uint32_t f, k;

		if (i < 20)
		{
			f = (b & c) | (~b & d);
			k = 0x5A827999;
		}
		else if (i < 40)
		{
			f = b ^ c ^ d;
			k = 0x6ED9EBA1;
		}
		else if (i < 60)
		{
			f = (b & c) | (b & d) | (c & d);
			k = 0x8F1BBCDC;
		}
		else if (i < 80)
		{
			f = b ^ c ^ d;
			k = 0xCA62C1D6;
		}

		uint32_t temp = lrot(a, 5) + f + e + k + w[i];
		e = d;
		d = c;
		c = lrot(b, 30);
		b = a;
		a = temp;
	}

	h[0] += a;
	h[1] += b;
	h[2] += c;
	h[3] += d;
	h[4] += e;
}

sha1_state::sha1_state()
{
	this->reset();
}

void sha1_state::reset()
{
	m_h[0] = 0x67452301;
	m_h[1] = 0xEFCDAB89;
	m_h[2] = 0x98BADCFE;
	m_h[3] = 0x10325476;
	m_h[4] = 0xC3D2E1F0;
	m_message_len = 0;
}

void sha1_state::add(uint8_t const * first, uint8_t const * last)
{
	size_t len = last - first;
	uint8_t rem = 64 - (m_message_len & 0x3f);
	m_message_len += len;

	uint8_t * block_start = m_block + 64 - rem;
	if (len < rem)
	{
		std::copy(first, last, block_start);
		return;
	}

	std::copy(first, first + rem, block_start);
	first += rem;
	transform_block(m_h, m_block);

	while (last - first >= 64)
	{
		transform_block(m_h, first);
		first += 64;
	}

	std::copy(first, last, m_block);
}

void sha1_state::finish(uint8_t * hash)
{
	uint8_t sublen = m_message_len & 0x3f;
	m_block[sublen++] = 0x80;
	if (sublen > 56)
	{
		std::fill(m_block + sublen, m_block + 64, 0);
		transform_block(m_h, m_block);
		sublen = 0;
	}

	std::fill(m_block + sublen, m_block + 56, 0);
	store_be(m_block + 56, m_message_len * 8);
	transform_block(m_h, m_block);

	store_be(hash, m_h[0]);
	store_be(hash + 4, m_h[1]);
	store_be(hash + 8, m_h[2]);
	store_be(hash + 12, m_h[3]);
	store_be(hash + 16, m_h[4]);
}

void sha1(uint8_t * hash, string_view data)
{
	sha1_state ss;
	ss.add((uint8_t const *)data.begin(), (uint8_t const *)data.end());
	ss.finish(hash);
}

void sha1(uint8_t * hash, istream & s)
{
	uint8_t buf[4096];
	sha1_state ss;
	for (;;)
	{
		size_t r = s.read(buf, sizeof buf);
		if (r == 0)
			break;
		ss.add(buf, buf + r);
	}
	ss.finish(hash);
}
