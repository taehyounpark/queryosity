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

	template <typename T>
	class calculator;

	class cutflow;

public:
	selection(const std::string& name);
  virtual ~selection() = default;

public:
	std::string get_name() const;

	void set_initial();
	void set_previous(const selection& preselection);

	bool is_initial() const;
	const selection* get_previous() const;

	void set_channel(bool channel=true);
	bool is_channel() const noexcept;

	std::string get_path() const;
	std::string get_full_path() const;

	// template <typename T>
	// auto book_counter(T& booker) const -> decltype(booker.book_counter(std::declval<selection>()));

	template <typename Val>
	void set_decision( std::shared_ptr<term<Val>> dec );

	virtual bool   passed_cut() const = 0;
	virtual double get_weight() const = 0;

public:
	virtual void initialize() override;
	virtual void execute() override;
	virtual void finalize() override;

private:
	std::string m_name;

	const selection* m_preselection;

	std::shared_ptr<column> m_decision;
	ana::variable<double>   m_variable;

	bool m_channel;
};

template <typename T>
class ana::selection::calculator
{

public:
	using equation_type = T;

public:
	calculator(std::shared_ptr<T> eqn);
	~calculator() = default;

	template <typename Sel>
	void set_selection( const std::string& name );
	void set_previous( selection const& prev );
	void set_channel(bool ch);

	template <typename... Vals> 
	std::shared_ptr<selection> evaluate_selection( cell<Vals> const&... columns) const;

protected:
	std::shared_ptr<T> m_equation;
	std::function<std::shared_ptr<selection>()> m_make_shared_selection;
	std::function<void(selection&)> m_set_previous;
	bool m_channel;

};


// template <typename Out, typename... Vals> 
// constexpr std::true_type check_selection(selection const&);
// constexpr std::false_type check_selection(...);
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
ana::selection::calculator<T>::calculator(std::shared_ptr<T> eqn) :
	m_make_shared_selection([]()->std::shared_ptr<selection>{return nullptr;}),
	m_equation(eqn),
	m_set_previous([](selection&){return;}),
	m_channel(false)
{}

template <typename T>
template <typename Sel>
void ana::selection::calculator<T>::set_selection(const std::string& name)
{
	m_make_shared_selection = std::bind([](const std::string& name)->std::shared_ptr<selection>{return std::make_shared<Sel>(name);}, name);
}


template <typename T>
void ana::selection::calculator<T>::set_channel(bool ch)
{
	m_channel = ch;
}

template <typename T>
void ana::selection::calculator<T>::set_previous(ana::selection const& previous)
{
	m_set_previous = std::bind([](selection& curr, selection const& prev){curr.set_previous(prev);}, std::placeholders::_1, std::cref(previous));
}

template <typename T>
template <typename... Vals> 
std::shared_ptr<ana::selection> ana::selection::calculator<T>::evaluate_selection( cell<Vals> const&... columns) const
{
	// make this selection
  auto sel = m_make_shared_selection();

	// mark it as channel if so
	sel->set_channel(m_channel);

	// set the previous selection
	m_set_previous(*sel);

	// set the decision
	m_equation->set_arguments(columns...);
	sel->set_decision(std::static_pointer_cast<term<cell_value_t<T>>>(m_equation));

  return sel;
}