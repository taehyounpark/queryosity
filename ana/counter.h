#pragma once

#include <vector>
#include <functional>

#include "ana/action.h"
#include "ana/concurrent.h"
#include "ana/cell.h"
#include "ana/strutils.h"

namespace ana
{

template <typename T>
class column;

template <typename T>
class variable;

template <typename T>
class observable;

class selection;

class counter : public action
{

public:
	template <typename T>
	class implementation;

	template <typename T>
	class manager;

	class experiment;

public:
	counter(const std::string& name);
	virtual ~counter() = default;

	void set_selection(const selection& selection);

	void set_scale(double scale);
	void use_weight(bool use=true);

	virtual void initialize() override;
	virtual void execute() override;
	virtual void finalize() override;

	virtual void count(double w) = 0;
	virtual void merge_result(const counter& incoming) = 0;

	std::string path() const;
	std::string full_path() const;

protected:
	const selection* m_selection;
	double m_scale;
	bool   m_raw;

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

	// --------------------------------------------------------------------------
	// CRPT dispatch

	template <typename... Vals>
	void fill_columns(const ana::column<Vals>&... cols);

	decltype(auto) get_result() const;

	// --------------------------------------------------------------------------
	// virtual -> CRTP dispatch

	virtual void merge_result(const counter& incoming) override;

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
	void fill(const column<Vals>&... cols);

	virtual void count(double w) override;
	virtual void enter(ana::observable<Obs>... observables, double w) = 0;

protected:
	std::vector<obstup_type> m_fills;

};

template <typename T>
class counter::manager
{

public:
	using implementation_type = T;

public:
	template <typename... Args>
	manager(const std::string& name, const Args&... args);
	~manager() = default;

	template <typename... Vals> 
	void fill_columns( const column<Vals>&... columns );

	std::shared_ptr<T> book_selection(const selection& selection) const;

	std::vector<std::string> booked_selection_paths() const;
	std::vector<std::shared_ptr<T>> booked_counters() const;

	// auto get_result(const std::string& path) -> typename decltype(std::declval<T>().result()) const;
	// auto get_results() -> std::vector<typename decltype(std::declval<T>().result())> const;

protected:
	mutable std::function<std::shared_ptr<T>()>  m_call_make;
	mutable std::vector<std::function<void(T&)>> m_call_fills;

	std::vector<std::string> m_booked_selections;
	std::unordered_map<std::string,std::shared_ptr<T>> m_booked_counter_map;

};

// FUTURE: replace with concepts (C++20)

template <typename Out> 
constexpr std::true_type check_counter_implementation(const counter::implementation<Out>&);
template <typename Out> 
constexpr std::false_type check_counter_implementation(const Out&);
template <typename T> 
constexpr bool is_counter_implementation_v = decltype(check_counter_implementation(std::declval<const T&>()))::value;

template <typename Out> 
constexpr std::false_type check_counter_fillable(const Out&);
template <typename Out, typename... Vals> 
constexpr std::true_type check_counter_fillable(const typename counter::implementation<Out>::template fillable<Vals...>&);
template <typename T> 
constexpr bool is_counter_fillable_v = decltype(check_counter_fillable(std::declval<const T&>()))::value;

template <typename Cnt>
struct is_counter_manager: std::false_type {};
template <typename Cnt>
struct is_counter_manager<counter::manager<Cnt>>: std::true_type {};
template <typename Cnt>
constexpr bool is_counter_manager_v = is_counter_manager<Cnt>::value;


}

#include "ana/column.h"
#include "ana/selection.h"

template <typename T>
ana::counter::implementation<T>::implementation(const std::string& name) :
	counter(name)
{}

template <typename T>
template <typename... Vals>
void ana::counter::implementation<T>::fill_columns(const ana::column<Vals>&... cols)
{
	static_cast<T*>(this)->fill(cols...);
}

template <typename T>
decltype(auto) ana::counter::implementation<T>::get_result() const
{
	return static_cast<const T*>(this)->result();
}

template <typename T>
void ana::counter::implementation<T>::merge_result(const counter& incoming)
{
	static_cast<T*>(this)->merge(static_cast<const T&>(incoming).get_result());
}

template <typename T>
template <typename... Obs>
ana::counter::implementation<T>::fillable<Obs...>::fillable(const std::string& name) :
	counter::implementation<T>(name)
{}

template <typename T>
template <typename... Obs>
template <typename... Vals>
void ana::counter::implementation<T>::fillable<Obs...>::fill(const column<Vals>&... cols)
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
				this->enter(obs..., w);
			}, m_fills[ifill]
		);
	}
}

template <typename T>
template <typename... Args>
ana::counter::manager<T>::manager(const std::string& name, const Args&... args) :
	m_call_make( std::bind([](const std::string& name, const Args&... args){return std::make_shared<T>(name,args...);}, name,args...) )
{}

template <typename T>
template <typename... Vals>
void ana::counter::manager<T>::fill_columns(const column<Vals>&... columns)
{
	m_call_fills.push_back(std::bind( [](T& cnt, const column<Vals>&... cols){ cnt.fill_columns(cols...);}, std::placeholders::_1, std::ref(columns)...));
}

template <typename T>
std::shared_ptr<T> ana::counter::manager<T>::book_selection(const selection& sel) const
{
	// call constructor
	auto cnt = m_call_make();

	// fill columns (if set)
	for (const auto& call_fill : m_call_fills) {
		call_fill(*cnt);
	}

	// book cnt at the selection
	cnt->set_selection(sel);

	// return booked & filled cnt
	return cnt;
}