#pragma once

#include <memory>
#include <string>
#include <vector>

#include "action.h"
#include "column.h"

namespace ana {

namespace selection {

class cutflow;

class cut;
class weight;

template <typename T> class apply;

class node : public column::calculation<double> {

public:
  node(const selection::node *presel);
  virtual ~node() = default;

public:
  bool is_initial() const noexcept;
  const selection::node *get_previous() const noexcept;

  template <typename T> void set_decision(std::unique_ptr<T> dec);

  virtual bool passed_cut() const = 0;
  virtual double get_weight() const = 0;

public:
  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) override;
  virtual void execute(unsigned int slot, unsigned long long entry) override;
  virtual void finalize(unsigned int slot) override;

protected:
  const selection::node *const m_preselection;
  std::unique_ptr<column::node> m_decision;
  ana::variable<double> m_variable;
};

template <typename T> struct is_applicable : std::false_type {};
template <typename T>
struct is_applicable<selection::apply<T>> : std::true_type {};
template <typename T>
static constexpr bool is_applicable_v = is_applicable<T>::value;

template <typename F>
using custom_apply_t =
    typename selection::template apply<ana::column::template equation_t<F>>;

} // namespace selection

template <typename T>
constexpr bool is_selection_v = std::is_base_of_v<selection::node, T>;
} // namespace ana

#include "column_equation.h"
#include "counter.h"

inline ana::selection::node::node(const selection::node *presel)
    : m_preselection(presel) {}

inline bool ana::selection::node::is_initial() const noexcept {
  return m_preselection ? false : true;
}

inline const ana::selection::node *
ana::selection::node::get_previous() const noexcept {
  return m_preselection;
}

inline void ana::selection::node::initialize(unsigned int slot,
                                             unsigned long long begin,
                                             unsigned long long end) {
  ana::column::calculation<double>::initialize(slot, begin, end);
  m_decision->initialize(slot, begin, end);
}

inline void ana::selection::node::execute(unsigned int slot,
                                          unsigned long long entry) {
  ana::column::calculation<double>::execute(slot, entry);
  m_decision->execute(slot, entry);
}

inline void ana::selection::node::finalize(unsigned int slot) {
  ana::column::calculation<double>::finalize(slot);
  m_decision->finalize(slot);
}

template <typename T>
void ana::selection::node::set_decision(std::unique_ptr<T> decision) {
  // link value to variable<double>
  m_variable = variable<double>(/*(term<cell_value_t<T>> &)*/ *decision);
  // keep decision as term
  m_decision = std::move(decision);
}