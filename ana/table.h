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

using FilePath = std::string;
using FileList = std::vector<std::string>;

template<typename Dat>
using Range = typename Dat::Range;

template<typename Dat, typename Val>
using Reader = typename Dat::template Reader<Val>;

struct range
{
	range();
	range(size_t slot, long long start, long long end);
	~range() = default;

	range  operator+ (const range& next);
	range& operator+=(const range& next);

	long long entries() const;

	size_t    slot;
	long long start;
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

	void      start();
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

	// partition data
	partition partition_data();
	partition allocate();

	// (optional) calculateion data normalization factor
	double normalize_data() const;
	double normalize() const;

	// open data range for each range
  decltype(auto) open_range() const;

	// open data range for each range
  // decltype(auto) allocate_range(const range& p) const;

	// initialize & finalze run
	void start_run();
	void start();

	void end_run();
	void end();

	// getters
	const partition& getpartition();	
	long long getEntries() const;
	double    get_weight() const;

};

template<typename T>
class reader
{

public:
	reader() = default;
	~reader() = default;

	// open data range for each range
	template<typename U, typename... Args>
  decltype(auto) read_column(const std::string& name, Args&&... args) const;

	template<typename U>
	void connectcolumn(U& column);

	void start_range(const range& range);

	bool next_entry();

	void end_range();
	void end();
	
protected:
	range m_part;

};

template<typename T>
class Column
{

public:
	Column() = default;
	~Column() = default;

	template<typename U>
	void setData(U& dataRange)
	{
		dataRange.connectcolumn(*static_cast<T*>(this));
	}

	void open_column() const
	{
		static_cast<const T*>(this)->open();
	}
	void open() const
	{}

	void next_entry() const
	{
		static_cast<const T*>(this)->next();
	}
	void next() const
	{}

	decltype(auto) get_entry() const
	{
		return static_cast<const T*>(this)->entry();
	}

	void close_column() const
	{
		static_cast<const T*>(this)->close();
	}
	void close() const
	{}

};

template<typename T>
class processor;

}

}

#include "ana/column.h"

template<typename T>
ana::table::partition ana::table::dataset<T>::partition_data()
{
	return static_cast<T*>(this)->allocate();
}

template<typename T>
double ana::table::dataset<T>::normalize_data() const
{
	return static_cast<const T*>(this)->normalize();
}

template<typename T>
double ana::table::dataset<T>::normalize() const
{
	return 1.0;
}

template<typename T>
void ana::table::dataset<T>::start_run()
{
  static_cast<T*>(this)->start();
}

template<typename T>
void ana::table::dataset<T>::end_run()
{
  static_cast<T*>(this)->end();
}

template<typename T>
decltype(auto) ana::table::dataset<T>::open_range() const
{
	return static_cast<const T*>(this)->open();
}

template<typename T>
void ana::table::dataset<T>::start()
{}

template<typename T>
void ana::table::dataset<T>::end()
{}

template<typename T>
template<typename U>
void ana::table::reader<T>::connectcolumn(U& column)
{
	static_cast<T*>(this)->connect(column);
}

// template<typename T>
// template<typename U, typename... Args>
// decltype(auto) ana::table::reader<T>::read_column(const std::string& name, Args&&... args)
// {
// 	return static_cast<T*>(this)->read(name, std::forward<Args>(args)...);
// }

template<typename T>
void ana::table::reader<T>::start_range(const range& range)
{
	static_cast<T*>(this)->start(std::cref(range));
}

template<typename T>
bool ana::table::reader<T>::next_entry()
{
	return static_cast<T*>(this)->next();
}

template<typename T>
void ana::table::reader<T>::end_range()
{
	static_cast<T*>(this)->end();
}

template<typename T>
void ana::table::reader<T>::end()
{}