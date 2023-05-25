#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <cassert>

namespace ana
{

template<typename T>
class sample;

namespace input
{

struct range
{
	range(size_t slot, long long begin, long long end);
	~range() = default;

	range  operator+ (const range& next);
	range& operator+=(const range& next);

	long long entries() const;

	size_t    slot;
	unsigned long long begin;
	unsigned long long end;
};

struct partition
{
	partition() = default;
	~partition() = default;

	void      add_part(size_t islot, long long begin, long long end);

	range     get_part(size_t irange) const;
	range     total() const;

	size_t    size() const;

	void truncate(long long max_entries=-1);
	void merge(size_t max_parts);

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
	virtual ~dataset() = default;

	// allocation partitioning for multithreading
	partition allocate_partition();

	// normalize scale for all entries by some logic
	double normalize_scale() const;
	// do nothing by default
	double normalize() const {return 1.0;}

	// read data for range
  decltype(auto) read_dataset() const;

	void start_dataset();
	void finish_dataset();

	// run dataset
	void start();
	void finish();

};

template<typename T>
class reader
{

public:

public:
	reader() = default;
	~reader() = default;

	// read a column of a data type with given name
	template<typename Val>
  decltype(auto) read_column(const range& part, const std::string& name) const;

	void start_part(const range& part);
	void read_entry(const range& part, unsigned long long entry);
	void finish_part(const range& part);

};

}

template<typename T> using read_dataset_t = typename decltype(std::declval<T>().read_dataset())::element_type;
template<typename T, typename Val> using read_column_t = typename decltype(std::declval<T>().template read_column<Val>(std::declval<input::range const&>(),std::declval<std::string const&>()))::element_type;

template<typename T> struct is_shared_ptr : std::false_type {};
template<typename T> struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};
template <typename T> static constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

}

#include "ana/column.h"

template<typename T>
ana::input::partition ana::input::dataset<T>::allocate_partition()
{
	return static_cast<T*>(this)->allocate();
}

template<typename T>
double ana::input::dataset<T>::normalize_scale() const
{
	return static_cast<const T*>(this)->normalize();
}

template<typename T>
void ana::input::dataset<T>::start_dataset()
{
	static_cast<T*>(this)->start();
}

template<typename T>
void ana::input::dataset<T>::finish_dataset()
{
	static_cast<T*>(this)->finish();
}

template<typename T>
void ana::input::dataset<T>::start()
{
	// nothing to do (yet)
}

template<typename T>
void ana::input::dataset<T>::finish()
{
	// nothing to do (yet)
}

template<typename T>
decltype(auto) ana::input::dataset<T>::read_dataset() const
{
	using result_type = decltype(static_cast<const T*>(this)->read());
	using reader_type = typename result_type::element_type;
	static_assert( is_shared_ptr_v<result_type>, "not a std::shared_ptr of ana::input::reader<T>" );
	static_assert( std::is_base_of_v<input::reader<reader_type>, reader_type>, "not an implementation of ana::input::reader<T>" );
	return static_cast<const T*>(this)->read();
}

template<typename T>
template<typename Val>
decltype(auto) ana::input::reader<T>::read_column(const range& part, const std::string& name) const
{
	using result_type = decltype(static_cast<const T*>(this)->template read<Val>(part,name));
	using reader_type = typename result_type::element_type;
	static_assert( is_shared_ptr_v<result_type>, "must be a std::shared_ptr of ana::column::reader<T>" );
	static_assert( is_column_reader_v<reader_type>, "not an implementation of ana::column::reader<T>" );
	return static_cast<const T*>(this)->template read<Val>(part, name);
}

template<typename T>
void ana::input::reader<T>::start_part(const ana::input::range& part)
{
	static_cast<T*>(this)->start(std::cref(part));
}

template<typename T>
void ana::input::reader<T>::read_entry(const ana::input::range& part, unsigned long long entry)
{
	static_cast<T*>(this)->next(std::cref(part),entry);
}

template<typename T>
void ana::input::reader<T>::finish_part(const ana::input::range& part)
{
	static_cast<T*>(this)->finish(std::cref(part));
}