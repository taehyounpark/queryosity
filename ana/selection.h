#pragma once

#include <memory>

#include "ana/column.h"
#include "ana/action.h"

namespace ana
{

class selection : public action
{

public:

	class cut;
	class weight;

	class a_or_b;
	class a_and_b;

	template <typename T>
	class evaluator;

	class cutflow;

public:
	selection(const std::string& name, bool channel);
  virtual ~selection() = default;

public:
	std::string get_name() const;
	std::string get_path() const;
	std::string get_full_path() const;

	void set_initial();
	void set_previous(const selection& preselection);

	bool is_initial() const;
	const selection* get_previous() const;

	bool is_channel() const noexcept;

	template <typename Val>
	void set_decision( std::shared_ptr<term<Val>> dec );

	virtual bool   passed_cut() const = 0;
	virtual double get_weight() const = 0;

public:
	virtual void initialize() override;
	virtual void execute() override;
	virtual void finalize() override;

private:
	const std::string m_name;
	const bool m_channel;

	const selection* m_preselection;
	std::shared_ptr<column> m_decision;
	ana::variable<double>   m_variable;

};

template <typename T>
class ana::selection::evaluator
{

public:
	using equation_type = T;

public:
	evaluator(std::shared_ptr<T> eqn);
	~evaluator() = default;

	template <typename Sel>
	void set_selection( const std::string& name, bool channel );
	void set_previous( selection const& prev );

	template <typename... Vals> 
	std::shared_ptr<selection> evaluate_selection( cell<Vals> const&... columns) const;

protected:
	std::function<std::shared_ptr<selection>()> m_make_shared;
	std::shared_ptr<T> m_equation;
	std::function<void(selection&)> m_set_previous;

};

template <typename T> 
constexpr bool is_selection_v = std::is_base_of_v<ana::selection, T>;

}

#include "ana/counter.h"
#include "ana/equation.h"

template <typename T>
void ana::selection::set_decision(std::shared_ptr<term<T>> decision)
{
	// keep decision as term
	m_decision = decision;
	// link value to variable<double>
	m_variable = variable<double>(*decision);
}

template <typename T>
ana::selection::evaluator<T>::evaluator(std::shared_ptr<T> eqn) :
	m_make_shared([]()->std::shared_ptr<selection>{return nullptr;}),
	m_equation(eqn),
	m_set_previous([](selection&){return;})
{}

template <typename T>
template <typename Sel>
void ana::selection::evaluator<T>::set_selection(const std::string& name, bool channel)
{
	m_make_shared = std::bind([](const std::string& name, bool ch)->std::shared_ptr<selection>{return std::make_shared<Sel>(name,ch);}, name,channel);
}

template <typename T>
void ana::selection::evaluator<T>::set_previous(ana::selection const& previous)
{
	m_set_previous = std::bind([](selection& curr, selection const& prev){curr.set_previous(prev);}, std::placeholders::_1, std::cref(previous));
}

template <typename T>
template <typename... Vals> 
std::shared_ptr<ana::selection> ana::selection::evaluator<T>::evaluate_selection( cell<Vals> const&... columns) const
{
	// make this selection
  auto sel = m_make_shared();

	// set the previous selection
	m_set_previous(*sel);

	// set the decision
	m_equation->set_arguments(columns...);
	sel->set_decision(std::static_pointer_cast<term<cell_value_t<T>>>(m_equation));

  return sel;
}