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
	template <typename Sel, typename F>
	auto filter(const std::string& name, F&& lmbd) -> std::shared_ptr<typename Sel::template calculator<equation_t<F>>>;

	template <typename Sel, typename F>
	auto channel(const std::string& name, F&& lmbd) -> std::shared_ptr<typename Sel::template calculator<equation_t<F>>>;

	template <typename Sel, typename F>
	auto filter(selection const& prev, const std::string& name, F&& lmbd) -> std::shared_ptr<typename Sel::template calculator<equation_t<F>>>;

	template <typename Sel, typename F>
	auto channel(selection const& prev, const std::string& name, F&& lmbd) -> std::shared_ptr<typename Sel::template calculator<equation_t<F>>>;

	template <typename Calc, typename... Cols>
	auto apply(Calc& calc, Cols const&... columns) -> std::shared_ptr<typename Calc::selection_type>;

	template <typename Sel, typename F>
	auto repeat_selection(typename Sel::template calculator<equation_t<F>> const& calc) -> std::shared_ptr<typename Sel::template calculator<equation_t<F>>>;

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

template <typename Sel, typename F>
auto ana::selection::cutflow::filter(const std::string& name, F&& lmbd) -> std::shared_ptr<typename Sel::template calculator<equation_t<F>>>
{
	auto eqn = make_equation(std::function(std::forward<F>(lmbd)));
	auto calc = std::make_shared<typename Sel::template calculator<equation_t<F>>>(name,eqn);
	calc->set_channel(false);
	return calc;
}

template <typename Sel, typename F>
auto ana::selection::cutflow::channel(const std::string& name, F&& lmbd) -> std::shared_ptr<typename Sel::template calculator<equation_t<F>>>
{
	auto eqn = make_equation(std::function(std::forward<F>(lmbd)));
	auto calc = std::make_shared<typename Sel::template calculator<equation_t<F>>>(name,eqn);
	calc->set_channel(true);
	return calc;
}

template <typename Sel, typename F>
auto ana::selection::cutflow::filter(selection const& prev, const std::string& name, F&& lmbd) -> std::shared_ptr<typename Sel::template calculator<equation_t<F>>>
{
	auto calc = this->filter<Sel>(name,std::forward<F>(lmbd));
	calc->set_previous(prev);
	return calc;
}

template <typename Sel, typename F>
auto ana::selection::cutflow::channel(selection const& prev,const std::string& name, F&& lmbd) -> std::shared_ptr<typename Sel::template calculator<equation_t<F>>>
{
	auto calc = this->channel<Sel>(name,std::forward<F>(lmbd));
	calc->set_previous(prev);
	return calc;
}

template <typename Calc, typename... Cols>
auto ana::selection::cutflow::apply(Calc& calc, Cols const&... columns) -> std::shared_ptr<typename Calc::selection_type>
{
	auto sel = calc.apply_selection(columns...);
	this->add_selection(*sel);
	return sel;
}

template <typename Sel, typename F>
auto ana::selection::cutflow::repeat_selection(typename Sel::template calculator<equation_t<F>> const& calc) -> std::shared_ptr<typename Sel::template calculator<equation_t<F>>>
{
	return std::make_shared<typename Sel::template calculator<equation_t<F>>>(calc);
}