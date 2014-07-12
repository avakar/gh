#ifndef WIN_ERROR_H
#define WIN_ERROR_H

#include <stdint.h>
#include <stdexcept>

class windows_error
	: public std::runtime_error
{
public:
	windows_error(uint32_t lasterror)
		: std::runtime_error("windows_error"), lasterror(lasterror)
	{
	}

private:
	uint32_t lasterror;
};

#endif // WIN_ERROR_H
