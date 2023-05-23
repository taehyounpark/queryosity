#pragma once

#include <string>
#include <vector>
#include <memory>


#include "ana/selection.h"
#include "ana/cut.h"
#include "ana/weight.h"

namespace ana
{

class selection::cutflow
{

public:
	cutflow();
	~cutflow() = default;

public:
	template <typename Sel, typename Fn>
	auto filter(const std::string& name, Fn fn) const -> std::shared_ptr<evaluator<ana::equation_t<Fn>>>;

	template <typename Sel, typename Fn>
	auto channel(const std::string& name, Fn fn) const -> std::shared_ptr<evaluator<ana::equation_t<Fn>>>;

	template <typename Sel, typename Fn>
	auto filter(selection const& prev, const std::string& name, Fn fn) const -> std::shared_ptr<evaluator<ana::equation_t<Fn>>>;

	template <typename Sel, typename Fn>
	auto channel(selection const& prev, const std::string& name, Fn fn) const -> std::shared_ptr<evaluator<ana::equation_t<Fn>>>;

	template <typename Sel, typename... Cols>
	auto evaluate_selection(evaluator<Sel> const& calc, Cols const&... columns) -> std::shared_ptr<selection>;

protected:
	void add_selection(selection& selection);

protected:
	// const selection* m_latest;
	std::vector<selection*> m_selections;

};

}

#include "ana/column.h"
#include "ana/counter.h"
#include "ana/equation.h"

template <typename Sel, typename Fn>
auto ana::selection::cutflow::filter(const std::string& name, Fn fn) const -> std::shared_ptr<evaluator<ana::equation_t<Fn>>>
{
	auto eqn = ana::make_equation(fn);
	auto calc = std::make_shared<evaluator<ana::equation_t<Fn>>>(eqn);
	calc->template set_selection<Sel>(name);
	calc->set_channel(false);
	return calc;
}

template <typename Sel, typename Fn>
auto ana::selection::cutflow::channel(const std::string& name, Fn fn) const -> std::shared_ptr<evaluator<ana::equation_t<Fn>>>
{
	auto eqn = ana::make_equation(fn);
	auto calc = std::make_shared<evaluator<ana::equation_t<Fn>>>(eqn);
	calc->template set_selection<Sel>(name);
	calc->set_channel(true);
	return calc;
}

template <typename Sel, typename Fn>
auto ana::selection::cutflow::filter(selection const& prev, const std::string& name, Fn fn) const -> std::shared_ptr<evaluator<ana::equation_t<Fn>>>
{
	auto calc = this->filter<Sel>(name,fn);
	calc->set_previous(prev);
	return calc;
}

template <typename Sel, typename Fn>
auto ana::selection::cutflow::channel(selection const& prev,const std::string& name, Fn fn) const -> std::shared_ptr<evaluator<ana::equation_t<Fn>>>
{
	auto calc = this->channel<Sel>(name,fn);
	calc->set_previous(prev);
	return calc;
}

template <typename Sel, typename... Cols>
auto ana::selection::cutflow::evaluate_selection(evaluator<Sel> const& calc, Cols const&... columns) -> std::shared_ptr<selection>
{
	auto sel = calc.evaluate_selection(columns...);
	this->add_selection(*sel);
	return sel;
}