#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <functional>
#include <utility>

#include "ana/action.h"

namespace ana
{

template <typename T>
class term;

template <typename T>
class cell;

template <typename T>
class variable;

template <typename T>
class observable;

class selection;


class counter : public action
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

	void apply_scale(double scale);
	void use_weight(bool use=true);

	void set_selection(const selection& selection);
	const selection* get_selection() const;

	virtual void initialize() override;
	virtual void execute() override;

	virtual void count(double w) = 0;

protected:
	bool             m_raw;
	double           m_scale;
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

	/**
	 * @brief Create the result of the counter.
	 * @return The result.
	*/
	virtual T result() const = 0;

	/**
	 * @brief Merge the results from concurrent slots into one.
	 * @param results Incoming results.
	 * @return The merged result.
	*/
	virtual T merge(std::vector<T> results) const = 0;

	/**
	 * @details Set the result of the counter.
	*/
	virtual void finalize() override;

	T get_result() const;
	T operator->() const
	{
		return this->get_result();
	}

	bool is_merged() const;
	void merge_results(std::vector<T> results);

protected:
	void set_merged(bool merged=true);

protected:
	T m_result;
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
	void enter_columns(term<Vals> const&... cols);

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
	booker(Args... args);

	// use default copy-constructor
	booker(booker const&) = default;
	booker& operator=(booker const&) = default;

	~booker() = default;

	template <typename... Vals> 
	auto book_fill(term<Vals> const&... cols) const -> std::shared_ptr<booker<T>>;

	auto book_selection(const selection& sel) const -> std::shared_ptr<T>;
	template <typename... Sels> 
	auto book_selections(Sels const&... sels) const -> std::shared_ptr<booker<T>>;

	std::vector<std::string> list_selection_paths() const;
	std::shared_ptr<T> get_counter(const std::string& path) const;

protected:
	template <typename... Vals> 
	void fill_counter(term<Vals> const&... cols);
	void make_counter(const selection& sel);

protected:
	std::function<std::shared_ptr<T>()>                m_make_counter_call;
	std::vector<std::function<void(T&)>>               m_fill_counter_calls;
	std::vector<std::string>                           m_booked_selection_paths;
	std::unordered_map<std::string,std::shared_ptr<T>> m_booked_counter_map;

};

template <typename T>
class counter::summary
{

public:

	// version for delayed<counter>
	template <typename Res>
	void record(const std::string& selection_path, std::decay_t<Res> counter_result)
	{
		static_cast<T*>(this)->record(selection_path, counter_result);	
	}

	// version for varied<counter>
	template <typename Res>
	void record(const std::string& variation_name, const std::string& selection_path, std::decay_t<Res> counter_result)
	{
		static_cast<T*>(this)->record(variation_name, selection_path, counter_result);	
	}

	template <typename Dest>
	void output(Dest& destination)
	{
		static_cast<T*>(this)->output(destination);	
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

#include "ana/column.h"
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
void ana::counter::implementation<T>::finalize()
{
	m_result = this->result();
}

template <typename T>
T ana::counter::implementation<T>::get_result() const
{
	return m_result;
}

template <typename T>
void ana::counter::implementation<T>::merge_results(std::vector<T> results)
{
	if (!results.size()) {
		throw std::logic_error("merging requires at least one result");
	}
	m_result = this->merge(results);
	this->set_merged(true);
}

template <typename T>
template <typename... Obs>
ana::counter::implementation<T>::fillable<Obs...>::fillable() :
	counter::implementation<T>()
{}

template <typename T>
template <typename... Obs>
template <typename... Vals>
void ana::counter::implementation<T>::fillable<Obs...>::enter_columns(term<Vals> const&... cols)
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
ana::counter::booker<T>::booker(Args... args) :
	m_make_counter_call(std::bind([](Args... args){return std::make_shared<T>(args...);}, args...))
{}

template <typename T>
template <typename... Vals>
auto ana::counter::booker<T>::book_fill(term<Vals> const&... columns) const -> std::shared_ptr<booker<T>>
{
	// use a snapshot of its current calls
	auto filled = std::make_shared<booker<T>>(*this);
	filled->fill_counter(columns...);
	return filled;
}

template <typename T>
template <typename... Vals>
void ana::counter::booker<T>::fill_counter(term<Vals> const&... columns)
{
	// use a snapshot of its current calls
	m_fill_counter_calls.push_back(std::bind([](T& cnt, term<Vals> const&... cols){cnt.enter_columns(cols...);}, std::placeholders::_1, std::ref(columns)...));
}

template <typename T>
template <typename... Sels>
auto ana::counter::booker<T>::book_selections(const Sels&... sels) const -> std::shared_ptr<booker<T>>
{
	// use a snapshot of its current calls
	auto counted = std::make_shared<booker<T>>(*this);
	(counted->make_counter(sels),...);
	// return a new booker with the selections added
	return counted;
}

template <typename T>
auto ana::counter::booker<T>::book_selection(const selection& sel) const -> std::shared_ptr<T>
{
	// use a snapshot of its current calls
	auto counted = std::make_shared<booker<T>>(*this);
	counted->make_counter(sel);
	// return a new booker with the selection added
	return counted->get_counter(sel.get_path());
}

template <typename T>
void ana::counter::booker<T>::make_counter(const selection& sel)
{
	// check if booking makes sense
	if (m_booked_counter_map.find(sel.get_path())!=m_booked_counter_map.end()) {
		throw std::logic_error("counter already booked at selection");
	}

	// call constructor
	auto cnt = m_make_counter_call();

	// map selection path & counter
	m_booked_selection_paths.push_back(sel.get_path());
	m_booked_counter_map.insert( std::make_pair(sel.get_path(),cnt) );

	// fill columns (if set)
	for (const auto& fill_counter_call : m_fill_counter_calls) {
		fill_counter_call(*cnt);
	}

	// book cnt at the selection
	cnt->set_selection(sel);
}

template <typename T>
std::vector<std::string> ana::counter::booker<T>::list_selection_paths() const
{
	return m_booked_selection_paths;
}

template <typename T>
std::shared_ptr<T> ana::counter::booker<T>::get_counter(const std::string& sel_path) const
{
	if (m_booked_counter_map.find(sel_path)==m_booked_counter_map.end()) {
		throw std::out_of_range("counter not booked at selection path");
		// return nullptr;
	}
	return m_booked_counter_map.at(sel_path);
}