#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>

class console_color_guard
{
public:
	explicit console_color_guard(uint8_t color);
	~console_color_guard();

private:
	uint8_t m_prev_color;
};

uint8_t set_console_color(uint8_t color);

#endif // CONSOLE_H
