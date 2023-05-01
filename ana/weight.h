#pragma once


#include "ana/selection.h"

namespace ana
{

class selection::weight : public selection
{

public:
	template <typename T>
	class calculator;

public:
	weight(const std::string& name);
  virtual ~weight() = default;

public:
	virtual bool   passed_cut() const override;
	virtual double get_weight() const override;

};

template <typename T>
class ana::selection::weight::calculator
{

public:
	using selection_type = weight;
	using equation_type = T;

public:
	calculator(const std::string& name, std::shared_ptr<T> eqn);
	~calculator() = default;

	void set_previous( selection const& prev );
	void set_channel(bool ch);

	template <typename... Vals> 
	std::shared_ptr<weight> apply_selection( cell<Vals> const&... columns) const;

protected:
	std::shared_ptr<T> m_equation;
	std::function<std::shared_ptr<weight>()> m_make_shared_weight;
	std::function<void(weight&)> m_set_previous;
	bool m_channel;
	std::string m_name;

};

}

template <typename T>
ana::selection::weight::calculator<T>::calculator(const std::string& name, std::shared_ptr<T> eqn) :
	m_name(name),
	m_make_shared_weight(std::bind([](const std::string& name){return std::make_shared<weight>(name);}, name)),
	m_equation(eqn),
	m_set_previous([](weight&){return;}),
	m_channel(false)
{}

template <typename T>
void ana::selection::weight::calculator<T>::set_channel(bool ch)
{
	m_channel = ch;
}

template <typename T>
void ana::selection::weight::calculator<T>::set_previous(selection const& previous)
{
	m_set_previous = std::bind([](weight& curr, selection const& prev){curr.set_previous(prev);}, std::placeholders::_1, std::cref(previous));
}

template <typename T>
template <typename... Vals>
std::shared_ptr<ana::selection::weight> ana::selection::weight::calculator<T>::apply_selection(cell<Vals> const&... columns) const
{
	// make this selection
  auto sel = m_make_shared_weight();

	// mark it as channel if so
	sel->set_channel(m_channel);

	// set the previous selection
	m_set_previous(*sel);

	// set the decision
	m_equation->set_arguments(columns...);
	sel->set_decision(std::static_pointer_cast<term<cell_value_t<T>>>(m_equation));

  return sel;
}