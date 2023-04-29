#pragma once

#include <string>
#include <vector>
#include <memory>
#include <exception>
#include <type_traits>
#include <functional>

#include "ana/input.h"
#include "ana/column.h"
#include "ana/term.h"
#include "ana/constant.h"
#include "ana/equation.h"

namespace ana 
{

template <typename T> struct is_column_calculator : std::false_type {};
template <typename T> struct is_column_calculator<column::calculator<T>> : std::true_type {};
template <typename T> constexpr bool is_column_calculator_v = is_column_calculator<T>::value;

constexpr std::true_type check_column(const column&);
constexpr std::false_type check_column(...);
template <typename T> constexpr bool is_column_v = decltype(check_column(std::declval<T>()))::value;

template <typename T>
constexpr std::true_type check_column_reader(typename column::reader<T> const&);
constexpr std::false_type check_column_reader(...);
template <typename T> constexpr bool is_column_reader_v = decltype(check_column_reader(std::declval<T>()))::value;

template <typename T>
constexpr std::true_type check_column_constant(typename column::constant<T> const&);
constexpr std::false_type check_column_constant(...);
template <typename T> constexpr bool is_column_constant_v = decltype(check_column_constant(std::declval<T>()))::value;

template <typename T>
constexpr std::true_type check_column_equation(typename column::equation<T> const&);
constexpr std::false_type check_column_equation(...);
template <typename T> constexpr bool is_column_equation_v = decltype(check_column_equation(std::declval<T>()))::value;

template <typename T>
constexpr std::true_type check_column_definition(typename column::definition<T> const&);
constexpr std::false_type check_column_definition(...);
template <typename T> constexpr bool is_column_definition_v = decltype(check_column_definition(std::declval<T>()))::value;

template <typename T>
using calculated_column_t = typename T::column_type;

template <typename T>
class column::computation
{

public:
	computation(input::reader<T>& reader);
	virtual ~computation() = default;

public:
	template <typename Val>
	auto read(const std::string& name) -> std::shared_ptr<input::read_column_t<T,Val>>;

	template <typename Val>
	auto constant(const Val& val) -> std::shared_ptr<column::constant<Val>>;

	template <typename Def, typename... Args>
	auto define(const Args&... vars) const -> std::shared_ptr<calculator<Def>>;

	template <typename F>
	auto define(F callable) const -> std::shared_ptr<calculator<equation_t<F>>>;

	template <typename Def, typename... Cols>
	auto compute(column::calculator<Def>& calc, Cols const&... columns) -> std::shared_ptr<Def>;

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
auto ana::column::computation<T>::read(const std::string& name) -> std::shared_ptr<input::read_column_t<T,Val>>
{
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
template <typename F>
// auto ana::column::computation<T>::evaluate(F callable, Vars&... vars) -> std::shared_ptr<ana::term<std::decay_t<typename decltype(std::function(std::declval<F>()))::result_type>>>
auto ana::column::computation<T>::define(F callable) const -> std::shared_ptr<calculator<equation_t<F>>>
{
	auto eqn = std::make_shared<calculator<equation_t<F>>>(callable);
	return eqn;
}

template <typename T>
template <typename Def, typename... Cols>
auto ana::column::computation<T>::compute(column::calculator<Def>& calc, Cols const&... columns) -> std::shared_ptr<Def>
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