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
	template <typename Sel, typename Lmbd>
	auto filter(const std::string& name, Lmbd&& lmbd) -> std::shared_ptr<calculator<column::equation_t<Lmbd>>>;

	template <typename Sel, typename Lmbd>
	auto channel(const std::string& name, Lmbd&& lmbd) -> std::shared_ptr<calculator<column::equation_t<Lmbd>>>;

	template <typename Sel, typename Lmbd>
	auto filter(selection const& prev, const std::string& name, Lmbd&& lmbd) -> std::shared_ptr<calculator<column::equation_t<Lmbd>>>;

	template <typename Sel, typename Lmbd>
	auto channel(selection const& prev, const std::string& name, Lmbd&& lmbd) -> std::shared_ptr<calculator<column::equation_t<Lmbd>>>;

	template <typename Sel, typename... Cols>
	auto evaluate_selection(calculator<Sel>& calc, Cols const&... columns) -> std::shared_ptr<selection>;

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

template <typename Sel, typename Lmbd>
auto ana::selection::cutflow::filter(const std::string& name, Lmbd&& lmbd) -> std::shared_ptr<calculator<column::equation_t<Lmbd>>>
{
	auto eqn = column::make_equation(std::function(std::forward<Lmbd>(lmbd)));
	auto calc = std::make_shared<calculator<column::equation_t<Lmbd>>>(eqn);
	calc->template set_selection<Sel>(name);
	calc->set_channel(false);
	return calc;
}

template <typename Sel, typename Lmbd>
auto ana::selection::cutflow::channel(const std::string& name, Lmbd&& lmbd) -> std::shared_ptr<calculator<column::equation_t<Lmbd>>>
{
	auto eqn = column::make_equation(std::function(std::forward<Lmbd>(lmbd)));
	auto calc = std::make_shared<calculator<column::equation_t<Lmbd>>>(eqn);
	calc->template set_selection<Sel>(name);
	calc->set_channel(true);
	return calc;
}

template <typename Sel, typename Lmbd>
auto ana::selection::cutflow::filter(selection const& prev, const std::string& name, Lmbd&& lmbd) -> std::shared_ptr<calculator<column::equation_t<Lmbd>>>
{
	auto calc = this->filter<Sel>(name,std::forward<Lmbd>(lmbd));
	calc->set_previous(prev);
	return calc;
}

template <typename Sel, typename Lmbd>
auto ana::selection::cutflow::channel(selection const& prev,const std::string& name, Lmbd&& lmbd) -> std::shared_ptr<calculator<column::equation_t<Lmbd>>>
{
	auto calc = this->channel<Sel>(name,std::forward<Lmbd>(lmbd));
	calc->set_previous(prev);
	return calc;
}

template <typename Sel, typename... Cols>
auto ana::selection::cutflow::evaluate_selection(calculator<Sel>& calc, Cols const&... columns) -> std::shared_ptr<selection>
{
	auto sel = calc.evaluate_selection(columns...);
	this->add_selection(*sel);
	return sel;
}