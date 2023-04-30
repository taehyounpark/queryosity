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

	void      add_part(size_t islot, long long start, long long end);
	void      add_part(const range& range);

	range     get_part(size_t irange) const;

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
	virtual ~dataset() = default;

	// get dataset partition & normalization
	partition allocate();
	virtual double normalize() const;

	// read data for range
  decltype(auto) read_dataset(const range& part) const;

	// run dataset
	virtual void start();
	virtual void finish();

};

template<typename T>
class reader
{

public:

public:
	reader(const range& part);
	~reader() = default;

	// read a column of a data type with given name
	template<typename Val>
  decltype(auto) read_column(const std::string& name) const;

	virtual void begin();
	virtual bool next() = 0;
	virtual void end();

protected:
	range m_part;

};

}

template<typename T> using read_dataset_t = typename decltype(std::declval<T>().read_dataset(std::declval<input::range>()))::element_type;
template<typename T, typename Val> using read_column_t = typename decltype(std::declval<T>().template read_column<Val>(std::declval<std::string>()))::element_type;

template<typename T> struct is_shared_ptr : std::false_type {};
template<typename T> struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};
template <typename T> static constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

}

template<typename T>
ana::input::partition ana::input::dataset<T>::allocate()
{
	return static_cast<const T*>(this)->allocate();
}

template<typename T>
double ana::input::dataset<T>::normalize() const
{
	return 1.0;
}

template<typename T>
void ana::input::dataset<T>::start()
{}

template<typename T>
void ana::input::dataset<T>::finish()
{}

template<typename T>
decltype(auto) ana::input::dataset<T>::read_dataset(const range& part) const
{
	using opened_t = decltype(static_cast<const T*>(this)->open(std::declval<range>()));
	static_assert( is_shared_ptr_v<opened_t>, "dataset must open a std::shared_ptr of its reader" );
	return static_cast<const T*>(this)->open(part);
}

template<typename T>
ana::input::reader<T>::reader(const ana::input::range& part) :
	m_part(part)
{}

template<typename T>
template<typename Val>
decltype(auto) ana::input::reader<T>::read_column(const std::string& name) const
{
	using opened_t = decltype(static_cast<const T*>(this)->template read<Val>(std::declval<std::string>()));
	static_assert( is_shared_ptr_v<opened_t>, "dataset must open a std::shared_ptr of its column reader" );
	return static_cast<const T*>(this)->template read<Val>(name);
}

template<typename T>
void ana::input::reader<T>::begin()
{}

template<typename T>
void ana::input::reader<T>::end()
{}