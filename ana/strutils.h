#pragma once

#include <string>
#include <vector>

namespace str
{

int count_instances(const std::string& str, const std::string& substr);

bool starts_with(const std::string& str, const std::string& prefix);
bool ends_with(const std::string& str, const std::string& suffix);

std::string enfinish(const std::string& str, const std::string& lside, const std::string& rside="");

std::string repeat(const std::string& str, unsigned int n=1);

std::vector<std::string> split(const std::string& str, const std::string& delim=",");
std::string join(std::initializer_list<std::string> str, const std::string& delim=",");
std::string join(const std::vector<std::string>& str, const std::string& delim=",");

std::string replace_all(const std::string& str, const std::string& from, const std::string& to);
std::string remove_all(const std::string& str, const std::string& substr);

std::string replace_leading(const std::string& str, const std::string& from, const std::string& to);
std::string replace_trailing(const std::string& str, const std::string& from, const std::string& to);

std::string remove_leading(const std::string& str, const std::string& prefix);
std::string remove_trailing(const std::string& str, const std::string& suffix);

std::string ensure_leading(const std::string& str, const std::string& prefix);
std::string ensure_trailing(const std::string& str, const std::string& suffix);

std::string truncate_leading(const std::string& str, size_t width, const std::string& ellipsis="...");
std::string truncate_trailing(const std::string& str, size_t width, const std::string& ellipsis="...");

std::string align_left(const std::string& str, size_t width, char fill=' ');
std::string align_center(const std::string& str, size_t width, char fill=' ');
std::string align_right(const std::string& str, size_t width, char fill=' ');

std::string apply_locale(double num, const std::string& loc="");
std::string apply_locale(long long num, const std::string& loc="");

}