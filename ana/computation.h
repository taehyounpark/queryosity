#pragma once

#include <string>
#include <vector>
#include <unordered_map>
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

template <typename T>
class column::computation
{

public:

public:
	computation(input::reader<T>& reader);
	virtual ~computation() = default;

public:
	template <typename Val>
	// auto read(const std::string& name) -> decltype(std::declval<input::reader<T>>().template read_column<Val>(name));
	auto read(const std::string& name) -> std::shared_ptr<input::read_column_t<T,Val>>;

	template <typename Val>
	std::shared_ptr<column::constant<Val>> constant(const Val& val);

	template <typename Def, typename... Args>
	std::shared_ptr<Def> define(const Args&... vars);

	template <typename F, typename... Vars>
	// auto evaluate(F callable, Vars&... vars) -> std::shared_ptr<term<std::decay_t<typename decltype(std::function(std::declval<F>()))::result_type>>>;
	auto evaluate(F callable, Vars&... vars) -> std::shared_ptr<equation_t<F>>;

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
std::shared_ptr<ana::column::constant<Val>> ana::column::computation<T>::constant(const Val& val)
{
	auto cnst = std::make_shared<typename column::constant<Val>>(val);
	this->add_column(*cnst);
	return cnst;
}

template <typename T>
template <typename Def, typename... Args>
std::shared_ptr<Def> ana::column::computation<T>::define(const Args&... args)
{
	auto defn = std::make_shared<Def>(args...);
	this->add_column(*defn);
	return defn;
}

template <typename T>
template <typename F, typename... Vars>
// auto ana::column::computation<T>::evaluate(F callable, Vars&... vars) -> std::shared_ptr<ana::term<std::decay_t<typename decltype(std::function(std::declval<F>()))::result_type>>>
auto ana::column::computation<T>::evaluate(F callable, Vars&... vars) -> std::shared_ptr<equation_t<F>>
{
	auto eqn = ana::make_equation(std::function(callable));
	eqn->set_evaluation(callable);
	if constexpr(sizeof...(Vars)>0) eqn->set_arguments(vars...);
	this->add_column(*eqn);
	return eqn;
}

template <typename T>
void ana::column::computation<T>::add_column(column& column)
{
	m_columns.push_back(&column);
}