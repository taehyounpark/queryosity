#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <functional>

#include "ana/routine.h"
#include "ana/concurrent.h"
#include "ana/column.h"
#include "ana/term.h"
#include "ana/strutils.h"

namespace ana
{

class selection;

class counter : public routine
{

public:
	// simplest implementation of a counter that user can implement
	template <typename T>
	class implementation;

	// optional method to "fill" with values of additional columns
	template <typename T>
	class logic;

	// manager of filled columns
	template <typename T>
	class filler;

	// manager of booked selections
	template <typename T>
	class booker;

	// results organization
	template <typename T>
	class summary;

	// run all counters
	class experiment;

public:
	counter();
	virtual ~counter() = default;

	void set_scale(double scale);
	void use_weight(bool use=true);

	void set_selection(const selection& selection);
	const selection* get_selection() const;

	virtual void initialize() override;
	virtual void execute() override;
	virtual void finalize() override;

	virtual void count(double w) = 0;

protected:
	double m_scale;
	bool   m_raw;

	const selection* m_selection;

};

template <typename T>
class counter::implementation : public counter
{

public:
	template <typename... Obs>
	class fillable;

	using result_type = T;

public:
	implementation();
	virtual ~implementation() = default;

	bool is_merged() const;
	void set_merged(bool merged=true);

	virtual T result() const = 0;
	virtual void merge(T res) = 0;

protected:
	bool m_merged;

};

template <typename T>
template <typename... Obs>
class counter::implementation<T>::fillable : public counter::implementation<T>
{

public:
	using obstup_type = std::tuple<ana::variable<Obs>...>;

public:
	fillable();
	virtual ~fillable() = default;

	template <typename... Vals>
	void fill_columns(term<Vals> const&... cols);

	virtual void count(double w) override;
	virtual void fill(ana::observable<Obs>... observables, double w) = 0;

protected:
	std::vector<obstup_type> m_fills;

};

template <typename T, typename... Obs>
class counter::logic<T(Obs...)> : public counter::implementation<T>::template fillable<Obs...>
{

public:
	logic();
	virtual ~logic() = default;

};

template <typename T>
class counter::booker
{

public:
	using counter_type = T;

public:
	template <typename... Args>
	booker(Args&&... args);
	~booker() = default;

	template <typename... Vals> 
	void fill_columns( term<Vals> const&... cols );

	std::shared_ptr<T> book_counter_at(const selection& sel);
	std::shared_ptr<T> get_counter_at(const std::string& path) const;

protected:
	std::function<std::shared_ptr<T>()>                m_make_counter;
	std::vector<std::function<void(T&)>>               m_fill_columns;
	std::unordered_map<std::string,std::shared_ptr<T>> m_booked_counter_map;

};

template <typename T>
class counter::summary
{

public:

	template <typename Result>
	void record(const std::string& selection_path, std::decay_t<Result> counter_result)
	{
		static_cast<T*>(this)->record(selection_path, counter_result);	
	}

	template <typename Destination>
	void report(const std::string& counter_name, Destination& destination)
	{
		static_cast<T*>(this)->report(counter_name, destination);	
	}

};

// FUTURE (C++20): use concepts

template <typename Out> 
constexpr std::true_type check_counter_implemented(const counter::implementation<Out>&);
constexpr std::false_type check_counter_implemented(...);
template <typename T> 
constexpr bool is_counter_implemented_v = decltype(check_counter_implemented(std::declval<T>()))::value;

template <typename Out, typename... Vals> 
constexpr std::true_type check_counter_fillable(const typename counter::implementation<Out>::template fillable<Vals...>&);
constexpr std::false_type check_counter_fillable(...);
template <typename T> 
constexpr bool is_counter_fillable_v = decltype(check_counter_fillable(std::declval<T>()))::value;

}

#include "ana/term.h"
#include "ana/selection.h"

template <typename T>
ana::counter::implementation<T>::implementation() :
	counter(),
	m_merged(false)
{}

template <typename T>
bool ana::counter::implementation<T>::is_merged() const
{
	return m_merged;
}

template <typename T>
void ana::counter::implementation<T>::set_merged(bool merged)
{
	m_merged = merged;
}

template <typename T>
template <typename... Obs>
ana::counter::implementation<T>::fillable<Obs...>::fillable() :
	counter::implementation<T>()
{}

template <typename T>
template <typename... Obs>
template <typename... Vals>
void ana::counter::implementation<T>::fillable<Obs...>::fill_columns(term<Vals> const&... cols)
{
	static_assert(sizeof...(Obs)==sizeof...(Vals), "dimension mis-match between filled variables & columns.");
	m_fills.emplace_back(cols...);
}

template <typename T>
template <typename... Obs>
void ana::counter::implementation<T>::fillable<Obs...>::count(double w)
{
	for (unsigned int ifill=0 ; ifill<m_fills.size() ; ++ifill) {
		std::apply(
			[this,w](const variable<Obs>&... obs) {
				this->fill(obs..., w);
			}, m_fills[ifill]
		);
	}
}

template <typename T, typename... Obs>
ana::counter::logic<T(Obs...)>::logic() :
	counter::implementation<T>::template fillable<Obs...>()
{}

template <typename T>
template <typename... Args>
ana::counter::booker<T>::booker(Args&&... args) :
	m_make_counter(std::bind([](Args&&... args){return std::make_shared<T>(std::forward<Args>(args)...);}, std::forward<Args>(args)...))
{}

template <typename T>
template <typename... Vals>
void ana::counter::booker<T>::fill_columns(term<Vals> const&... columns)
{
	m_fill_columns.push_back(std::bind([](T& cnt, term<Vals> const&... cols){cnt.fill_columns(cols...);}, std::placeholders::_1, std::ref(columns)...));
}

template <typename T>
std::shared_ptr<T> ana::counter::booker<T>::book_counter_at(const selection& sel)
{
	// call constructor
	auto cnt = m_make_counter();

	// fill columns (if set)
	for (const auto& call_fill : m_fill_columns) {
		call_fill(*cnt);
	}

	// book cnt at the selection
	cnt->set_selection(sel);

	// save booked selection/counter
	if (m_booked_counter_map.find(sel.get_path())!=m_booked_counter_map.end()) {
		throw std::logic_error("counter already booked at selection");
	}
	m_booked_counter_map.insert( std::make_pair(sel.get_path(),cnt) );

	// return booked & filled cnt
	return cnt;
}

template <typename T>
std::shared_ptr<T> ana::counter::booker<T>::get_counter_at(const std::string& sel_path) const
{
	if (m_booked_counter_map.find(sel_path)==m_booked_counter_map.end()) {
		throw std::logic_error("counter is not booked at the specified selection path");
		// return nullptr;
	}
	return m_booked_counter_map.at(sel_path);
}