#pragma once

#include <vector>
#include <functional>

#include "ana/action.h"
#include "ana/concurrent.h"
#include "ana/cell.h"
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
	class booker;

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
	implementation(const std::string& name);
	virtual ~implementation() = default;

	// --------------------------------------------------------------------------
	// CRPT dispatch

	template <typename... Vars>
	void fill_columns(Vars&... vars);

	decltype(auto) get_result() const;

	// --------------------------------------------------------------------------
	// virtual -> CRTP dispatch

	virtual void merge_result(const counter& incoming) override;

};

template <typename T>
class counter::booker
{

public:
	using counterType = T;

public:
	template <typename... Args>
	booker(const std::string& name, const Args&... args);
	~booker() = default;

	template <typename... Cols> 
	void fill_columns( Cols&... columns );

	std::shared_ptr<T> book_selection(const selection& selection) const;

protected:
	mutable std::function<std::shared_ptr<T>()>  m_call_make;
	mutable std::vector<std::function<void(T&)>> m_call_fills;

};

template<class T, class Enable=void>
struct is_counter_booker: std::false_type {};

template<class T>
struct is_counter_booker<counter::booker<T>>: std::true_type {};

}

#include "ana/selection.h"

template <typename T>
ana::counter::implementation<T>::implementation(const std::string& name) :
	counter(name)
{}

template <typename T>
template <typename... Vars>
void ana::counter::implementation<T>::fill_columns(Vars&... vars)
{
	if constexpr( (std::is_base_of_v<variable,Vars> && ...) ) {
		static_cast<T*>(this)->fill(vars...);
	} else {
		static_assert( (std::is_base_of_v<variable,Vars> && ...), "cannot fill counter with non-variables" );
	}
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
template <typename... Args>
ana::counter::booker<T>::booker(const std::string& name, const Args&... args) :
	m_call_make( std::bind([](const std::string& name, const Args&... args){return std::make_shared<T>(name,args...);}, name,args...) )
{}

template <typename T>
template <typename... Cols>
void ana::counter::booker<T>::fill_columns(Cols&... columns)
{
	m_call_fills.push_back(std::bind( [](T& cnt, Cols&... cols){ cnt.fill_columns(cols...);}, std::placeholders::_1, std::ref(columns)...));
}

template <typename T>
std::shared_ptr<T> ana::counter::booker<T>::book_selection(const selection& sel) const
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