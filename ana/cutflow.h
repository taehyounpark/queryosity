#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include "ana/selection.h"

namespace ana
{

class selection::cutflow
{

public:
	cutflow();
	~cutflow() = default;

public:
	cutflow& at(const selection& current) noexcept;

	template <typename Sel, typename F, typename... Vars>
	std::shared_ptr<selection> filter(const std::string& name, F callable, Vars&... vars);

	template <typename Sel, typename F, typename... Vars>
	std::shared_ptr<selection> channel(const std::string& name, F callable, Vars&... vars);

protected:
	void add(selection& selection);

protected:
	const selection* m_latest;
	std::vector<selection*> m_selections;

};

}

#include "ana/cut.h"
#include "ana/weight.h"
#include "ana/term.h"
#include "ana/counter.h"
#include "ana/equation.h"

template <typename Sel, typename F, typename... Vars>
std::shared_ptr<ana::selection> ana::selection::cutflow::filter(const std::string& name, F callable, Vars&... vars)
{
	using return_type = typename std::invoke_result_t<F, typename std::decay_t<decltype(std::declval<Vars>().value())>...>;
	auto eqn = std::make_shared<column::equation<return_type(std::decay_t<decltype(std::declval<Vars>().value())>...)>>(name);
	eqn->set_evaluation(callable);
	if constexpr(sizeof...(Vars)>0) eqn->set_arguments(vars...);
	auto flt = std::make_shared<Sel>(name);
	if (m_latest) flt->set_previous(*m_latest);
	flt->set_decision(std::static_pointer_cast<ana::term<return_type>>(eqn));
	this->add(*flt);
	return flt;
}

template <typename Sel, typename F, typename... Vars>
std::shared_ptr<ana::selection> ana::selection::cutflow::channel(const std::string& name, F callable, Vars&... vars)
{
	using return_type = typename std::invoke_result_t<F, typename std::decay_t<decltype(std::declval<Vars>().value())>...>;
	auto eqn = std::make_shared<column::equation<return_type(std::decay_t<decltype(std::declval<Vars>().value())>...)>>(name);
	eqn->set_evaluation(callable);
	if constexpr(sizeof...(Vars)>0) eqn->set_arguments(vars...);
	auto flt = std::make_shared<Sel>(name);
	flt->set_channel(true);
	flt->set_decision(std::static_pointer_cast<ana::term<return_type>>(eqn));
	if (m_latest) flt->set_previous(*m_latest);
	this->add(*flt);
	return flt;
}