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

	std::string current_name;

	impl()
		: hFind(0)
	{
	}

	~impl()
	{
		if (hFind)
			FindClose(hFind);
	}
};

bool dir_enum_proxy::find_next(impl & pimpl)
{
	if (!::FindNextFileW(pimpl.hFind, &pimpl.wfd))
	{
		DWORD dwError = ::GetLastError();
		if (dwError != ERROR_NO_MORE_FILES)
			throw windows_error(dwError);
		return false;
	}
	else
	{
		pimpl.current_name = from_utf16(pimpl.wfd.cFileName);
		return true;
	}
}

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

	if ((pimpl->wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
	{
		pimpl->current_name = from_utf16(pimpl->wfd.cFileName);
	}
	else
	{
		do
		{
			if (!this->find_next(*pimpl))
				return;
		}
		while (pimpl->wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
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

directory_entry dir_enum_proxy::iterator::operator*() const
{
	assert(m_pimpl->m_pimpl);

	directory_entry res;
	res.name = m_pimpl->m_pimpl->current_name;
	res.mtime = (uint32_t)(((uint64_t const &)m_pimpl->m_pimpl->wfd.ftLastWriteTime - 116444736000000000) / 10000000);
	res.mode = 0x8000;
	return res;
}

dir_enum_proxy::iterator & dir_enum_proxy::iterator::operator++()
{
	assert(m_pimpl->m_pimpl);

	do
	{
		if (!dir_enum_proxy::find_next(*m_pimpl->m_pimpl))
		{
			delete m_pimpl->m_pimpl;
			m_pimpl->m_pimpl = 0;
			break;
		}
	}
	while (m_pimpl->m_pimpl->wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

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
	std::wstring wpath = to_utf16(path);
	if (!::CreateDirectoryW(wpath.c_str(), 0))
	{
		DWORD dwError = ::GetLastError();
		if (dwError == ERROR_ALREADY_EXISTS)
			return false;
		throw windows_error(dwError);
	}

	size_t slash_pos = wpath.find_last_of(L"/\\");
	if (slash_pos < wpath.size() - 1 && wpath[slash_pos + 1] == '.')
		::SetFileAttributesW(wpath.c_str(), FILE_ATTRIBUTE_HIDDEN);

	return true;
}

void file::create(string_view path, string_view content)
{
	file fout;
	fout.open(path, /*readonly=*/false);

	ofile of = fout.seekp(0);
	write_all(of, (uint8_t const *)content.begin(), content.size());
}

string_view get_path_head(string_view path)
{
	char const * p = path.end();
	for (; p != path.begin(); --p)
	{
		if (p[-1] == '/' || p[-1] == '\\')
			return string_view(path.begin(), p - 1);
	}
	return string_view();
}

string_view get_path_tail(string_view path)
{
	char const * p = path.end();
	for (; p != path.begin(); --p)
	{
		if (p[-1] == '/' || p[-1] == '\\')
			return string_view(p, path.end());
	}
	return path;
}

bool file::exists(string_view path)
{
	DWORD attrs = ::GetFileAttributesW(to_utf16(path).c_str());

	if (attrs == INVALID_FILE_ATTRIBUTES)
	{
		DWORD dwError = ::GetLastError();
		if (dwError != ERROR_FILE_NOT_FOUND && dwError != ERROR_PATH_NOT_FOUND)
			throw windows_error(dwError);
		return false;
	}

	return true;
}

bool file::is_file(string_view path)
{
	DWORD attrs = ::GetFileAttributesW(to_utf16(path).c_str());

	if (attrs == INVALID_FILE_ATTRIBUTES)
	{
		DWORD dwError = ::GetLastError();
		if (dwError != ERROR_FILE_NOT_FOUND && dwError != ERROR_PATH_NOT_FOUND)
			throw windows_error(dwError);
		return false;
	}

	return (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool file::is_directory(string_view path)
{
	DWORD attrs = ::GetFileAttributesW(to_utf16(path).c_str());

	if (attrs == INVALID_FILE_ATTRIBUTES)
	{
		DWORD dwError = ::GetLastError();
		if (dwError != ERROR_FILE_NOT_FOUND && dwError != ERROR_PATH_NOT_FOUND)
			throw windows_error(dwError);
		return false;
	}

	return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

std::vector<directory_entry> listdir(string_view path, string_view mask)
{
	std::vector<directory_entry> res;

	WIN32_FIND_DATAW wfd;
	HANDLE hFind = ::FindFirstFileExW(to_utf16(path.to_string() + "/" + mask.to_string()).c_str(), FindExInfoBasic, &wfd, FindExSearchNameMatch, 0, 0);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		DWORD dwError = ::GetLastError();
		if (dwError == ERROR_NO_MORE_FILES)
			return res;
		throw windows_error(dwError);
	}

	for (;;)
	{
		if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 || wfd.cFileName[0] != '.' || (wfd.cFileName[1] != 0 && (wfd.cFileName[1] != '.' || wfd.cFileName[2] != 0)))
		{
			res.emplace_back(
				from_utf16(wfd.cFileName),
				(uint32_t)(((uint64_t const &)wfd.ftLastWriteTime - 116444736000000000) / 10000000),
				(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)? 0x4000: 0x8000);
		}

		if (!::FindNextFileW(hFind, &wfd))
		{
			DWORD dwError = ::GetLastError();
			::FindClose(hFind);
			if (dwError == ERROR_NO_MORE_FILES)
				return res;
			throw windows_error(dwError);
		}
	}
}

dir_entry_type directory_entry::type() const
{

	if ((mode & 0xe000) == 0x4000)
		return dir_entry_type::directory;
	else if ((mode & 0xe000) == 0xe000)
		return dir_entry_type::gitlink;
	else
		return dir_entry_type::file;
}

struct UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
};

typedef LONG __stdcall RtlCompareUnicodeString_t(UNICODE_STRING const * String1, UNICODE_STRING const * String2, BOOLEAN CaseInSensitive);
RtlCompareUnicodeString_t * g_RtlCompareUnicodeString = 0;

int compare_filenames(string_view lhs, string_view rhs)
{
	// Normally, we'd use CompareStringOrdinal, but unfortunately, it's Vista+ only.

	RtlCompareUnicodeString_t * RtlCompareUnicodeString = static_cast<RtlCompareUnicodeString_t * volatile &>(g_RtlCompareUnicodeString);
	if (RtlCompareUnicodeString == 0)
	{
		HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
		RtlCompareUnicodeString = (RtlCompareUnicodeString_t *)GetProcAddress(hNtdll, "RtlCompareUnicodeString");
		static_cast<RtlCompareUnicodeString_t * volatile &>(g_RtlCompareUnicodeString) = RtlCompareUnicodeString;
	}

	std::wstring wlhs = to_utf16(lhs);
	std::wstring wrhs = to_utf16(rhs);

	UNICODE_STRING ulhs;
	ulhs.Buffer = (PWSTR)wlhs.data();
	ulhs.Length = (USHORT)(wlhs.size() * 2);

	UNICODE_STRING urhs;
	urhs.Buffer = (PWSTR)wrhs.data();
	urhs.Length = (USHORT)(wrhs.size() * 2);

	return RtlCompareUnicodeString(&ulhs, &urhs, TRUE);
}
