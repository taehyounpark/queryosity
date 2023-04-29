#pragma once

#include "ana/selection.h"

namespace ana
{

class selection::cut : public selection
{

public:
	template <typename T>
	class calculator;

public:
	cut(const std::string& name);
  virtual ~cut() = default;

public:
	virtual bool   passed_cut() const override;
	virtual double get_weight() const override;

};

template <typename T>
class ana::selection::cut::calculator
{

public:
	using selection_type = cut;

public:
	calculator(const std::string& name, std::shared_ptr<T> eqn);
	~calculator() = default;

	void set_channel(bool ch);

	template <typename Sel>
	void set_previous( Sel const& prev );

	template <typename... Vals> 
	std::shared_ptr<cut> apply_selection( cell<Vals> const&... columns) const;

protected:
	std::shared_ptr<T> m_equation;
	std::function<std::shared_ptr<cut>()> m_make_shared_cut;
	std::function<void(cut&)> m_set_previous;
	bool m_channel;

};

}

template <typename T>
ana::selection::cut::calculator<T>::calculator(const std::string& name, std::shared_ptr<T> eqn) :
	m_make_shared_cut(std::bind([](const std::string& name){return std::make_shared<cut>(name);}, name)),
	m_equation(eqn),
	m_set_previous([](cut&){return;}),
	m_channel(false)
{}

template <typename T>
void ana::selection::cut::calculator<T>::set_channel(bool ch)
{
	m_channel = ch;
}

template <typename T>
template <typename Sel>
void ana::selection::cut::calculator<T>::set_previous(Sel const& previous)
{
	m_set_previous = std::bind([](cut& curr, Sel const& prev){curr.set_previous(prev);}, std::placeholders::_1, std::cref(previous));
}

template <typename T>
template <typename... Vals> 
std::shared_ptr<ana::selection::cut> ana::selection::cut::calculator<T>::apply_selection( cell<Vals> const&... columns) const
{
	// make this selection
  auto sel = m_make_shared_cut();

	// mark it as channel if so
	sel->set_channel(m_channel);

	// set the previous selection
	m_set_previous(*sel);

	// set the decision
	m_equation->set_arguments(columns...);
	sel->set_decision(std::static_pointer_cast<term<term_value_t<T>>>(m_equation));

  return sel;
}