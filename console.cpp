#include "console.h"
#include <windows.h>

console_color_guard::console_color_guard(uint8_t color)
{
	m_prev_color = set_console_color(color);
}

console_color_guard::~console_color_guard()
{
	set_console_color(m_prev_color);
}

uint8_t set_console_color(uint8_t color)
{
	HANDLE hOutput = ::GetStdHandle(STD_OUTPUT_HANDLE);
	if (((uint32_t)hOutput & 3) != 3)
		return 0;

	CONSOLE_SCREEN_BUFFER_INFO csbi = { sizeof csbi };
	if (!::GetConsoleScreenBufferInfo(hOutput, &csbi))
		return 0;

	::SetConsoleTextAttribute(hOutput, (csbi.wAttributes & ~(WORD)0xf) | (color & 0xf));
	return csbi.wAttributes & 0xf;
}
