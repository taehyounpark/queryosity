#pragma once

#include <memory>
#include <string>
#include <vector>

#include "action.h"
#include "column.h"

namespace ana {

class selection;

template <typename T>
constexpr bool is_selection_v = std::is_base_of_v<ana::selection, T>;

// class selection : public action {
class selection : public column::calculation<double> {

public:
  class cutflow;

  class cut;
  class weight;

  template <typename T> class applicator;

public:
  selection(const selection *presel);
  virtual ~selection() = default;

public:
  bool is_initial() const noexcept;
  const selection *get_previous() const noexcept;

  template <typename T> void set_decision(std::unique_ptr<T> dec);

  virtual bool passed_cut() const = 0;
  virtual double get_weight() const = 0;

public:
  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) override;
  virtual void execute(unsigned int slot, unsigned long long entry) override;
  virtual void finalize(unsigned int slot) override;

private:
  const selection *const m_preselection;

  std::unique_ptr<column::column_base> m_decision;
  ana::variable<double> m_variable;

public:
  template <typename T> struct is_applicator : std::false_type {};
  template <typename T>
  struct is_applicator<selection::applicator<T>> : std::true_type {};
  template <typename T>
  static constexpr bool is_applicator_v = is_applicator<T>::value;

  template <typename F>
  using custom_applicator_t = typename selection::template applicator<
      ana::column::template equation_t<F>>;
  using trivial_applicator_type = typename selection::template applicator<
      ana::column::equation<double(double)>>;
};

} // namespace ana

#include "column_equation.h"
#include "counter.h"

inline ana::selection::selection(const selection *presel)
    : m_preselection(presel) {}

inline bool ana::selection::is_initial() const noexcept {
  return m_preselection ? false : true;
}

inline const ana::selection *ana::selection::get_previous() const noexcept {
  return m_preselection;
}

inline void ana::selection::initialize(unsigned int slot,
                                       unsigned long long begin,
                                       unsigned long long end) {
  ana::column::calculation<double>::initialize(slot, begin, end);
  m_decision->initialize(slot, begin, end);
}

inline void ana::selection::execute(unsigned int slot,
                                    unsigned long long entry) {
  ana::column::calculation<double>::execute(slot, entry);
  m_decision->execute(slot, entry);
}

inline void ana::selection::finalize(unsigned int slot) {
  ana::column::calculation<double>::finalize(slot);
  m_decision->finalize(slot);
}

template <typename T>
void ana::selection::set_decision(std::unique_ptr<T> decision) {
  // link value to variable<double>
  m_variable = variable<double>((term<cell_value_t<T>> &)(*decision));
  // keep decision as term
  m_decision = std::move(decision);
}