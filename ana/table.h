#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <cassert>

namespace ana
{

template<typename T>
class sample;

namespace table
{

struct range
{
	range();
	range(size_t slot, long long start, long long end);
	~range() = default;

	range  operator+ (const range& next);
	range& operator+=(const range& next);

	long long entries() const;

	size_t    slot;
	long long begin;
	long long end;
};


struct partition
{
	partition() = default;
	~partition() = default;

	void      add(size_t islot, long long start, long long end);
	void      add(const range& range);
	range     part(size_t irange) const;
	size_t    size() const;
	range     total() const;
	partition truncate(long long max_entries=-1) const;
	partition merge(size_t max_parts) const;

	bool               fixed = false;
	std::vector<range> parts = {};
};

class progress
{

public:
	progress(long long total);
	~progress() = default;

	void      reset();
	progress& operator++();

	double percent() const;
	bool   done() const;

protected:
	std::atomic<long long> prog;
	const long long tot;

};

template<typename T>
class dataset
{

public:
	dataset() = default;
	~dataset() = default;

	// get dataset partition & normalization
	virtual partition allocate();
	virtual double normalize() const;

	// read data for range
  decltype(auto) open_reader(const range& part) const;

	// run dataset
	virtual void start();
	virtual void finish();

};

template<typename T>
class reader
{

public:
	reader(const range& part);
	~reader() = default;

	// open data range for each range
	template<typename U, typename... Args>
  decltype(auto) read_column(const std::string& name, Args&&... args) const;

	virtual void begin();
	virtual bool next() = 0;
	virtual void end();

protected:
	range m_part;

};

template<typename T>
class processor;

template<typename T>
using read_t = typename decltype(std::declval<T>().open_reader(std::declval<const table::range&>()))::element_type;

}

}

#include "ana/column.h"

template<typename T>
double ana::table::dataset<T>::normalize() const
{
	return 1.0;
}

template<typename T>
void ana::table::dataset<T>::start()
{}

template<typename T>
void ana::table::dataset<T>::finish()
{}

template<typename T>
decltype(auto) ana::table::dataset<T>::open_reader(const range& part) const
{
	return static_cast<const T*>(this)->open(part);
}

template<typename T>
ana::table::reader<T>::reader(const ana::table::range& part) :
	m_part(part)
{}

template<typename T>
template<typename U, typename... Args>
decltype(auto) ana::table::reader<T>::read_column(const std::string& name, Args&&... args) const
{
	return static_cast<const T*>(this)->template read<U>(name, std::forward<Args>(args)...);
}

template<typename T>
void ana::table::reader<T>::begin()
{}

template<typename T>
void ana::table::reader<T>::end()
{}