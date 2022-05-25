#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <exception>
#include <type_traits>
#include <functional>

#include "ana/table.h"
#include "ana/cell.h"
#include "ana/column.h"

namespace ana 
{

template <typename T>
class variable::computation
{

public:
	computation(table::reader<T>& reader);
	virtual ~computation() = default;

public:
	template <typename Col, typename... Args>
	std::shared_ptr<column<Col>> read(const std::string& name, const Args&... args);

	template <typename Val>
	std::shared_ptr<column<Val>> constant(const std::string& name, const Val& val);

	template <typename Def, typename... Args>
	std::shared_ptr<Def> define(const std::string& name, const Args&... vars);

	template <typename F, typename... Vars>
	auto evaluate(const std::string& name, F callable, Vars&... vars) -> std::shared_ptr<column<std::decay_t<typename decltype(std::function(std::declval<F>()))::result_type>>>;

protected:
	void add(variable& column);

protected:
	table::reader<T>* m_reader;
	std::vector<variable*> m_columns;

};

}

#include "ana/constant.h"
#include "ana/equation.h"

template <typename T>
ana::variable::computation<T>::computation(table::reader<T>& reader) :
	m_reader(&reader)
{}

template <typename T>
template <typename Val, typename... Args>
std::shared_ptr<ana::column<Val>> ana::variable::computation<T>::read(const std::string& name, const Args&... args)
{
	auto rdr = m_reader->template read_column<Val>(name, args...);
	this->add(*rdr);
	return rdr;
}

template <typename T>
template <typename Val>
std::shared_ptr<ana::column<Val>> ana::variable::computation<T>::constant(const std::string& name, const Val& val)
{
	auto cnst = std::make_shared<typename column<Val>::constant>(name,val);
	this->add(*cnst);
	return cnst;
}

template <typename T>
template <typename Def, typename... Args>
std::shared_ptr<Def> ana::variable::computation<T>::define(const std::string& name, const Args&... args)
{
	auto defn = std::make_shared<Def>(name, args...);
	this->add(*defn);
	return defn;
}

template <typename T>
template <typename F, typename... Vars>
auto ana::variable::computation<T>::evaluate(const std::string& name, F callable, Vars&... vars) -> std::shared_ptr<ana::column<std::decay_t<typename decltype(std::function(std::declval<F>()))::result_type>>>
{
	auto eqn = ana::make_equation(name,std::function(callable));
	eqn->set_evaluation(callable);
	if constexpr(sizeof...(Vars)) eqn->input_arguments(vars...);
	this->add(*eqn);
	return eqn;
}

template <typename T>
void ana::variable::computation<T>::add(variable& column)
{
	m_columns.push_back(&column);
}