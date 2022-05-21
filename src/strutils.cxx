#include "ana/strutils.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <locale>

#include "ana/vecutils.h"
#include "ana/lexertk.hpp"

std::string str::enclose(const std::string& str, const std::string& lside, const std::string& rside)
{
	std::string enclosed = lside;
	enclosed += str;
	enclosed += rside.empty() ? lside : rside;
	return enclosed;
}

std::string str::repeat(const std::string& str, unsigned int n)
{
	std::string repeated;
	repeated.reserve(str.size() * n);
	for (unsigned int i=0 ; i<n ; ++i) {
		repeated += str;
	}
	return repeated;
}

std::vector<std::string> str::split(const std::string& str, const std::string& delimiter)
{
	// tokenize
	std::vector<std::string> tokens;
	size_t last = 0; size_t next = 0;
	while ((next = str.find(delimiter, last)) != std::string::npos) {
		tokens.push_back(str.substr(last, next-last));
		last = next + 1;
	}
	tokens.push_back(str.substr(last));
	// return split strings
  return tokens;
}

std::string str::join(std::initializer_list<std::string> strs, const std::string& delimiter)
{
	return str::join(std::vector<std::string>(strs.begin(),strs.end()),delimiter);
}

std::string str::join(const std::vector<std::string>& strs, const std::string& delimiter)
{
	std::string joined;
	for (auto&& str : strs) {
		joined += str;
		joined += delimiter;
	}
	joined = str::remove_trailing(joined,delimiter);
	return joined;
}

std::string str::replace_all(const std::string& str, const std::string& from, const std::string& to)
{
	size_t pos = 0;
	std::string replaced = str;
	while((pos = replaced.find(from, pos)) != std::string::npos) {
		replaced.replace(pos, from.length(), to);
		pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
	return replaced;
}

std::string str::remove_all(const std::string& str, const std::string& del)
{
	return str::replace_all(str,del,"");
}

int str::count_instances(const std::string& str, const std::string& substr)
{
	int count = 0;
	std::string::size_type pos = 0;
	while ((pos = str.find(substr, pos )) != std::string::npos) {
		++count;
		pos += substr.length();
	}
	return count;
}

bool str::starts_with(const std::string& str, const std::string& prefix)
{
	return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}

bool str::ends_with(const std::string& str, const std::string& suffix)
{
	return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}

std::string str::replace_leading(const std::string& str, const std::string& from, const std::string& to)
{
	std::string replaced = str;
	while(str::starts_with(replaced,from)) replaced = to+replaced.substr(from.length());
	return replaced;
}

std::string str::replace_trailing(const std::string& str, const std::string& from, const std::string& to)
{
	std::string replaced = str;
	while(str::ends_with(str,from)) replaced = replaced.substr(0,replaced.length()-from.length())+to;
	return replaced;
}

std::string str::remove_leading(const std::string& str, const std::string& substr)
{
	std::string removed = str;
	while (str::starts_with(removed,substr)) removed = removed.substr(substr.length());
	return removed;
}

std::string str::remove_trailing(const std::string& str, const std::string& substr)
{
	std::string removed = str;
	while (str::ends_with(removed,substr)) removed = removed.substr(0,removed.length()-substr.length());
	return removed;
}

std::string str::ensure_leading(const std::string& str, const std::string& prefix)
{
	return str::starts_with(str,prefix) ? str : prefix+str;
}

std::string str::ensure_trailing(const std::string& str, const std::string& suffix)
{
	return str::ends_with(str,suffix) ? str : str+suffix;
}

std::string str::truncate_leading(const std::string& str, size_t width, const std::string& ellipsis)
{
	auto trunced = (str.length() > width) ? ellipsis + str.substr(str.length()-width+ellipsis.length(),str.length()) : str;
	return trunced;
}

std::string str::truncate_trailing(const std::string& str, size_t width, const std::string& ellipsis)
{
	auto trunced = (str.length() > width) ? str.substr(0, width-ellipsis.length()) + ellipsis : str;
	return trunced;
}

std::string str::align_left(const std::string& str, size_t width, char fill)
{
	std::stringstream ss;
	ss
		<< std::setfill(fill)
		<< std::left
		<< std::setw(width)
		<< str;
	return ss.str();
}

std::string str::align_right(const std::string& str, size_t width, char fill)
{
	std::stringstream ss;
	ss
		<< std::setfill(fill)
		<< std::right
		<< std::setw(width)
		<< str;
	return ss.str();
}

std::string str::apply_locale(double num, const std::string& loc)
{
	std::stringstream ss;
	ss.imbue(std::locale(loc.c_str()));
	ss << num;
	return ss.str();
}

std::string str::apply_locale(long long num, const std::string& loc)
{
	std::stringstream ss;
	ss.imbue(std::locale(loc.c_str()));
	ss << num;
	return ss.str();
}

std::vector<std::string> str::tokenize_expression(std::string expression, const std::vector<std::string>& existingColumnNames)
{
  expression = str::replace_all(expression,"\"","'");

	// start with empty list of used column names
	std::vector<std::string> usedColumnNames;
	// C++ expression lexer
	lexertk::generator tokens;
	const auto tokensOk = tokens.process(expression);
	if (!tokensOk) {
		const auto msg = "failed to tokenize expression";
		throw std::runtime_error(msg);
	}
   // iterate over tokens in expression and fill usedColumnNames, varNames and expr_withVars
   const auto nTokens = tokens.size();
   const auto kSymbol = lexertk::token::e_symbol;
   for (auto i = 0u; i < nTokens; ++i) {
      const auto &tok = tokens[i];
      // lexertk classifies '&' as e_symbol for some reason
      if (tok.type != kSymbol || tok.value == "&" || tok.value == "|") {
        // token is not a potential variable name, skip it
        continue;
      }
      // get a list of candidate names
      std::vector<std::string> candidateColumnNames({tok.value});
      // if token is the start of a dot chain (a.b.c...), a.b, a.b.c etc. are also potential column names
      auto dotChainKeepsGoing = [&](unsigned int _i) {
        return _i + 2 <= nTokens && tokens[_i + 1].value == "." && tokens[_i + 2].type == kSymbol;
      };
      while (dotChainKeepsGoing(i)) {
        candidateColumnNames.emplace_back(candidateColumnNames.back() + "." + tokens[i + 2].value);
        i += 2; // consume the tokens we looked at
      }
      // find the longest potential column name that is an actual column name
      // if it's a new match, also add it to usedColumnNames and update varNames
      // potential columns are sorted by length, so we search from the end
      auto isVariable = [&](const std::string &columnName) {
        return (vec::contains(existingColumnNames,columnName));
      };
      const auto longestMatch = std::find_if(candidateColumnNames.crbegin(), candidateColumnNames.crend(), isVariable);
      if (longestMatch != candidateColumnNames.crend() && !vec::contains(usedColumnNames,*longestMatch)) {
				usedColumnNames.emplace_back(*longestMatch);
      }
   }

   // return column names are actually used
   return usedColumnNames;
}