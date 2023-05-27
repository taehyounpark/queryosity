#pragma once

#include <string>
#include <vector>
#include <memory>

#include "selection.h"
#include "cut.h"
#include "weight.h"

namespace ana
{

class selection::cutflow
{

public:
	cutflow() = default;
	~cutflow() = default;

public:
	template <typename Sel, typename F>
	auto filter(const std::string& name, F expression) const -> std::shared_ptr<evaluator<ana::equation_t<F>>>;

	template <typename Sel, typename F>
	auto channel(const std::string& name, F expression) const -> std::shared_ptr<evaluator<ana::equation_t<F>>>;

	template <typename Sel, typename F>
	auto filter(selection const& prev, const std::string& name, F expression) const -> std::shared_ptr<evaluator<ana::equation_t<F>>>;

	template <typename Sel, typename F>
	auto channel(selection const& prev, const std::string& name, F expression) const -> std::shared_ptr<evaluator<ana::equation_t<F>>>;

	template <typename Sel, typename... Cols>
	auto evaluate_selection(evaluator<Sel> const& calc, Cols const&... columns) -> std::shared_ptr<selection>;

	template <typename Sel>
	auto join(selection const& a, selection const& b) const -> std::shared_ptr<selection>;

protected:
	void add_selection(selection& selection);

protected:
	std::vector<selection*> m_selections;

};

}

#include "column.h"
#include "counter.h"
#include "equation.h"

template <typename Sel, typename F>
auto ana::selection::cutflow::filter(const std::string& name, F expression) const -> std::shared_ptr<evaluator<ana::equation_t<F>>>
{
	auto eqn = ana::make_equation(expression);
	auto calc = std::make_shared<evaluator<ana::equation_t<F>>>(eqn);
	calc->template set_selection<Sel>(name,false);
	return calc;
}

template <typename Sel, typename F>
auto ana::selection::cutflow::channel(const std::string& name, F expression) const -> std::shared_ptr<evaluator<ana::equation_t<F>>>
{
	auto eqn = ana::make_equation(expression);
	auto calc = std::make_shared<evaluator<ana::equation_t<F>>>(eqn);
	calc->template set_selection<Sel>(name,true);
	return calc;
}

template <typename Sel, typename F>
auto ana::selection::cutflow::filter(selection const& prev, const std::string& name, F expression) const -> std::shared_ptr<evaluator<ana::equation_t<F>>>
{
	auto calc = this->filter<Sel>(name,expression);
	calc->set_previous(prev);
	return calc;
}

template <typename Sel, typename F>
auto ana::selection::cutflow::channel(selection const& prev,const std::string& name, F expression) const -> std::shared_ptr<evaluator<ana::equation_t<F>>>
{
	auto calc = this->channel<Sel>(name,expression);
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

template <typename Sel>
auto ana::selection::cutflow::join(ana::selection const& a, ana::selection const& b) const -> std::shared_ptr<ana::selection>
{
	return std::make_shared<Sel>(a,b);
}

inline void ana::selection::cutflow::add_selection(ana::selection& sel)
{
	m_selections.push_back(&sel);
}