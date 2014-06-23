#include "file.h"
#include "utf.h"
#include <memory>
#include <windows.h>
#include <assert.h>

class windows_error
	: public std::runtime_error
{
public:
	windows_error(DWORD lasterror)
		: std::runtime_error("windows_error"), lasterror(lasterror)
	{
	}

private:
	DWORD lasterror;
};

file::file()
	: m_fd(0)
{
}

file::file(string_view path, bool readonly)
	: m_fd(0)
{
	this->open(path, readonly);
}

file::file(file && o)
	: m_fd(o.m_fd)
{
	o.m_fd = 0;
}

file::~file()
{
	this->close();
}

file & file::operator=(file && o)
{
	std::swap(m_fd, o.m_fd);
	return *this;
}

void file::open(string_view path, bool readonly)
{
	if (!this->try_open(path, readonly))
		throw windows_error(ERROR_FILE_NOT_FOUND);
}

bool file::try_open(string_view path, bool readonly)
{
	HANDLE hFile = ::CreateFileW(to_utf16(path).c_str(), readonly? GENERIC_READ: GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, readonly? OPEN_EXISTING: OPEN_ALWAYS, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		DWORD dwError = ::GetLastError();
		if (dwError == ERROR_FILE_NOT_FOUND || dwError == ERROR_PATH_NOT_FOUND)
			return false;
		throw windows_error(dwError);
	}
	m_fd = (intptr_t)hFile;
	return true;
}

void file::close()
{
	if (m_fd)
	{
		::CloseHandle((HANDLE)m_fd);
		m_fd = 0;
	}
}

bool file::is_open() const
{
	return m_fd != 0;
}

file::ifile file::seekg(file_offset_t pos)
{
	return file::ifile(this, pos);
}

file::ofile file::seekp(file_offset_t pos)
{
	return file::ofile(this, pos);
}

size_t file::read_abs(file_offset_t pos, uint8_t * p, size_t capacity)
{
	OVERLAPPED o = {};
	o.Offset = (DWORD)pos;
	o.OffsetHigh = (DWORD)(pos >> 32);

	DWORD dwRead;
	if (!::ReadFile((HANDLE)m_fd, p, capacity, &dwRead, &o))
	{
		DWORD dwError = ::GetLastError();
		if (dwError == ERROR_HANDLE_EOF)
			return 0;
		throw windows_error(dwError);
	}
	return dwRead;
}

struct dir_enum_proxy::impl
{
	HANDLE hFind;
	WIN32_FIND_DATAW wfd;
};

dir_enum_proxy::dir_enum_proxy(string_view path, string_view mask)
	: m_pimpl(0)
{
	std::unique_ptr<impl> pimpl(new impl());

	pimpl->hFind = ::FindFirstFileExW(to_utf16(path.to_string() + "/" + mask.to_string()).c_str(), FindExInfoBasic, &pimpl->wfd, FindExSearchNameMatch, 0, 0);
	if (pimpl->hFind == INVALID_HANDLE_VALUE)
	{
		DWORD dwError = ::GetLastError();
		if (dwError == ERROR_NO_MORE_FILES)
			return;
		throw windows_error(dwError);
	}

	m_pimpl = pimpl.release();
}

dir_enum_proxy::dir_enum_proxy(dir_enum_proxy && o)
	: m_pimpl(o.m_pimpl)
{
	o.m_pimpl = 0;
}

dir_enum_proxy::~dir_enum_proxy()
{
	if (m_pimpl)
	{
		FindClose(m_pimpl->hFind);
		delete m_pimpl;
	}
}

dir_enum_proxy::iterator dir_enum_proxy::begin()
{
	return iterator(this);
}

dir_enum_proxy::iterator dir_enum_proxy::end()
{
	return iterator(0);
}

dir_enum_proxy::iterator::iterator(dir_enum_proxy * pimpl)
	: m_pimpl(pimpl)
{
}

std::string dir_enum_proxy::iterator::operator*() const
{
	assert(m_pimpl->m_pimpl);
	return from_utf16(m_pimpl->m_pimpl->wfd.cFileName);
}

dir_enum_proxy::iterator & dir_enum_proxy::iterator::operator++()
{
	assert(m_pimpl->m_pimpl);
	if (!::FindNextFileW(m_pimpl->m_pimpl->hFind, &m_pimpl->m_pimpl->wfd))
	{
		DWORD dwError = ::GetLastError();
		if (dwError != ERROR_NO_MORE_FILES)
			throw windows_error(dwError);

		FindClose(m_pimpl->m_pimpl->hFind);
		delete m_pimpl->m_pimpl;
		m_pimpl->m_pimpl = 0;
	}
	return *this;
}

dir_enum_proxy::iterator dir_enum_proxy::iterator::operator++(int)
{
	return ++*this;
}

size_t file::write_abs(file_offset_t pos, uint8_t const * p, size_t capacity)
{
	OVERLAPPED o = {};
	o.Offset = (DWORD)pos;
	o.OffsetHigh = (DWORD)(pos >> 32);

	DWORD written;
	if (!::WriteFile((HANDLE)m_fd, p, capacity, &written, &o))
		throw windows_error(::GetLastError());

	return written;
}

bool make_directory(string_view path)
{
	if (!::CreateDirectoryW(to_utf16(path).c_str(), 0))
	{
		DWORD dwError = ::GetLastError();
		if (dwError == ERROR_ALREADY_EXISTS)
			return false;
		throw windows_error(dwError);
	}

	return true;
}
