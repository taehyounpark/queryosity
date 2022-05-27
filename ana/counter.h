#pragma once

#include <vector>
#include <functional>

#include "ana/action.h"
#include "ana/concurrent.h"
#include "ana/cell.h"
#include "ana/column.h"
#include "ana/strutils.h"

namespace ana
{

class selection;

class counter : public action
{

public:
	template <typename T>
	class implementation;

	template <typename T>
	class bookkeeper;

	class experiment;

public:
	counter(const std::string& name);
	virtual ~counter() = default;

	void set_scale(double scale);
	void use_weight(bool use=true);

	void set_selection(const selection& selection);

	virtual void initialize() override;
	virtual void execute() override;
	virtual void finalize() override;

	virtual void count(double w) = 0;

	std::string path() const;
	std::string full_path() const;

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

public:
	implementation(const std::string& name);
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
	fillable(const std::string& name);
	virtual ~fillable() = default;

	template <typename... Vals>
	void enter(const column<Vals>&... cols);

	virtual void count(double w) override;
	virtual void fill(ana::observable<Obs>... observables, double w) = 0;

protected:
	std::vector<obstup_type> m_fills;

};

template <typename T>
class counter::bookkeeper
{

public:
	using counter_type = T;

public:
	template <typename... Args>
	bookkeeper(const std::string& name, const Args&... args);
	~bookkeeper() = default;

	template <typename... Vals> 
	void enter( const column<Vals>&... cols );

	std::shared_ptr<T> book_selection(const selection& sel) const;
	std::shared_ptr<T> get_counter(const std::string& path) const;

protected:
	mutable std::function<std::shared_ptr<T>()>  m_call_make;
	mutable std::vector<std::function<void(T&)>> m_call_fills;
	mutable std::unordered_map<std::string,std::shared_ptr<T>> m_booked_counter_map;

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

template <typename Cnt>
struct is_counter_bookkeeper: std::false_type {};
template <typename Cnt>
struct is_counter_bookkeeper<counter::bookkeeper<Cnt>>: std::true_type {};
template <typename Cnt>
constexpr bool is_counter_bookkeeper_v = is_counter_bookkeeper<Cnt>::value;


}

#include "ana/column.h"
#include "ana/selection.h"

template <typename T>
ana::counter::implementation<T>::implementation(const std::string& name) :
	counter(name),
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
ana::counter::implementation<T>::fillable<Obs...>::fillable(const std::string& name) :
	counter::implementation<T>(name)
{}

template <typename T>
template <typename... Obs>
template <typename... Vals>
void ana::counter::implementation<T>::fillable<Obs...>::enter(const column<Vals>&... cols)
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

template <typename T>
template <typename... Args>
ana::counter::bookkeeper<T>::bookkeeper(const std::string& name, const Args&... args) :
	m_call_make( std::bind([](const std::string& name, const Args&... args){return std::make_shared<T>(name,args...);}, name,args...) )
{}

template <typename T>
template <typename... Vals>
void ana::counter::bookkeeper<T>::enter(const column<Vals>&... columns)
{
	m_call_fills.push_back(std::bind( [](T& cnt, const column<Vals>&... cols){ cnt.enter(cols...);}, std::placeholders::_1, std::ref(columns)...));
}

template <typename T>
std::shared_ptr<T> ana::counter::bookkeeper<T>::book_selection(const selection& sel) const
{
	// call constructor
	auto cnt = m_call_make();

	// fill columns (if set)
	for (const auto& call_fill : m_call_fills) {
		call_fill(*cnt);
	}

	// book cnt at the selection
	cnt->set_selection(sel);

	// save booked selection/counter
	m_booked_counter_map[sel.path()] = cnt;

	// return booked & filled cnt
	return cnt;
}

template <typename T>
std::shared_ptr<T> ana::counter::bookkeeper<T>::get_counter(const std::string& sel_path) const
{
	return m_booked_counter_map[sel_path];
}