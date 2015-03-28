#ifndef PATH_H
#define PATH_H

#include <string>
#include "string_view.h"

void clean_path(std::string & path);
std::string clean_path(string_view path);

std::string join_paths(string_view lhs, string_view rhs);

std::string absolute_path(string_view path);
std::string normalize_path(string_view path);
std::string relative_path(string_view path);
std::string relative_path(string_view path, string_view base);
std::string cannonical_path(string_view path);
std::string find_path(string_view base_dir, string_view path);

string_view path_root(string_view path);
string_view path_head(string_view path);
string_view path_tail(string_view path);

string_view split_path_left(string_view & path);
string_view split_path_right(string_view & path);

std::string current_dir();

#endif // PATH_H
