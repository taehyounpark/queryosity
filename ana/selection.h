#pragma once

#include <memory>

#include "ana/column.h"
#include "ana/term.h"
#include "ana/action.h"

namespace ana
{

class selection : public action
{

public:
	class cut;
	class weight;
	class cutflow;

public:
	selection(const std::string& name);
  virtual ~selection() = default;

public:
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
	const selection* m_preselection;

	std::shared_ptr<column> m_decision;
	ana::variable<double>   m_variable;

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