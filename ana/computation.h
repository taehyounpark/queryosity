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

namespace ana 
{

template <typename T>
class column::computation
{

public:
	computation(input::reader<T>& reader);
	virtual ~computation() = default;

public:
	template <typename Col, typename... Args>
	std::shared_ptr<term<Col>> read(const std::string& name, const Args&... args);

	template <typename Val>
	std::shared_ptr<term<Val>> constant(const std::string& name, const Val& val);

	template <typename Def, typename... Args>
	std::shared_ptr<Def> define(const std::string& name, const Args&... vars);

	template <typename F, typename... Vars>
	auto evaluate(const std::string& name, F callable, Vars&... vars) -> std::shared_ptr<term<std::decay_t<typename decltype(std::function(std::declval<F>()))::result_type>>>;

protected:
	void add(column& column);

protected:
	input::reader<T>* m_reader;
	std::vector<column*> m_columns;

};

}

#include "ana/constant.h"
#include "ana/equation.h"

template <typename T>
ana::column::computation<T>::computation(input::reader<T>& reader) :
	m_reader(&reader)
{}

template <typename T>
template <typename Val, typename... Args>
std::shared_ptr<ana::term<Val>> ana::column::computation<T>::read(const std::string& name, const Args&... args)
{
	auto rdr = m_reader->template read_column<Val>(name, args...);
	this->add(*rdr);
	return rdr;
}

template <typename T>
template <typename Val>
std::shared_ptr<ana::term<Val>> ana::column::computation<T>::constant(const std::string& name, const Val& val)
{
	auto cnst = std::make_shared<typename column::constant<Val>>(name,val);
	this->add(*cnst);
	return cnst;
}

template <typename T>
template <typename Def, typename... Args>
std::shared_ptr<Def> ana::column::computation<T>::define(const std::string& name, const Args&... args)
{
	auto defn = std::make_shared<Def>(name, args...);
	this->add(*defn);
	return defn;
}

template <typename T>
template <typename F, typename... Vars>
auto ana::column::computation<T>::evaluate(const std::string& name, F callable, Vars&... vars) -> std::shared_ptr<ana::term<std::decay_t<typename decltype(std::function(std::declval<F>()))::result_type>>>
{
	auto eqn = ana::make_equation(name,std::function(callable));
	eqn->set_evaluation(callable);
	if constexpr(sizeof...(Vars)>0) eqn->set_arguments(vars...);
	this->add(*eqn);
	return eqn;
}

template <typename T>
void ana::column::computation<T>::add(column& column)
{
	m_columns.push_back(&column);
}