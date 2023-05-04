#pragma once

#include <string>
#include <vector>
#include <memory>
#include <exception>
#include <type_traits>
#include <functional>

#include "ana/input.h"
#include "ana/column.h"
#include "ana/constant.h"
#include "ana/equation.h"

namespace ana 
{

template <typename T>
class column::computation
{

public:
	computation(input::reader<T>& reader);
	virtual ~computation() = default;

public:
	template <typename Val>
	auto read(const std::string& name) -> std::shared_ptr<read_column_t<T,Val>>;

	template <typename Val>
	auto constant(const Val& val) -> std::shared_ptr<column::constant<Val>>;

	template <typename Def, typename... Args>
	auto define(const Args&... vars) const -> std::shared_ptr<calculator<Def>>;

	template <typename Fn>
	auto define(Fn const& fn) const -> std::shared_ptr<calculator<ana::equation_t<Fn>>>;

	template <typename Def, typename... Cols>
	auto evaluate_column(column::calculator<Def>& calc, Cols const&... columns) -> std::shared_ptr<Def>;

	template <typename Def, typename... Args>
	auto vary_column(column::calculator<Def> const& calc, Args&&... args) const -> std::shared_ptr<column::calculator<Def>>;

protected:
	void add_column(column& column);

protected:
	input::reader<T>* m_reader;
	std::vector<column*> m_columns;

};

}

template <typename T>
ana::column::computation<T>::computation(input::reader<T>& reader) :
	m_reader(&reader)
{}

template <typename T>
template <typename Val>
// auto ana::column::computation<T>::read(const std::string& name) -> decltype(std::declval<input::reader<T>>().template read_column<Val>(name))
auto ana::column::computation<T>::read(const std::string& name) -> std::shared_ptr<read_column_t<T,Val>>
{
	using read_t = decltype(m_reader->template read_column<Val>(std::declval<std::string>()));
	static_assert( is_shared_ptr_v<read_t>, "dataset must open a std::shared_ptr of its column reader" );
	auto rdr = m_reader->template read_column<Val>(name);
	this->add_column(*rdr);
	return rdr;
}

template <typename T>
template <typename Val>
auto ana::column::computation<T>::constant(const Val& val) -> std::shared_ptr<ana::column::constant<Val>>
{
	auto cnst = std::make_shared<typename column::constant<Val>>(val);
	this->add_column(*cnst);
	return cnst;
}

template <typename T>
template <typename Def, typename... Args>
auto ana::column::computation<T>::define(const Args&... args) const -> std::shared_ptr<calculator<Def>>
{
	auto defn = std::make_shared<calculator<Def>>(args...);
	return defn;
}

template <typename T>
template <typename Fn>
auto ana::column::computation<T>::define(Fn const& fn) const -> std::shared_ptr<calculator<ana::equation_t<Fn>>>
{
	auto eqn = std::make_shared<calculator<ana::equation_t<Fn>>>(fn);
	return eqn;
}

template <typename T>
template <typename Def, typename... Cols>
auto ana::column::computation<T>::evaluate_column(column::calculator<Def>& calc, Cols const&... columns) -> std::shared_ptr<Def>
{
	// use the calculator to actually make the column
	auto defn = calc.calculate_from(columns...);
	// and add it
	this->add_column(*defn);
	return defn;
}

template <typename T>
template <typename Def, typename... Args>
auto ana::column::computation<T>::vary_column(column::calculator<Def> const& calc, Args&&... args) const -> std::shared_ptr<calculator<Def>>
{
	return std::make_shared<calculator<Def>>(std::forward<Args>(args)...);
}

template <typename T>
void ana::column::computation<T>::add_column(column& column)
{
	m_columns.push_back(&column);
}