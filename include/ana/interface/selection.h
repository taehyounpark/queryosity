#pragma once

#include <memory>
#include <string>
#include <vector>

#include "column.h"
#include "operation.h"

namespace ana {

class selection;

template <typename T>
constexpr bool is_selection_v = std::is_base_of_v<ana::selection, T>;

// class selection : public operation {
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
  virtual void initialize(const dataset::range &part) override;
  virtual void execute(const dataset::range &part,
                       unsigned long long entry) override;
  virtual void finalize(const dataset::range &part) override;

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

inline void ana::selection::initialize(const ana::dataset::range &part) {
  ana::column::calculation<double>::initialize(part);
  m_decision->initialize(part);
}

inline void ana::selection::execute(const ana::dataset::range &part,
                                    unsigned long long entry) {
  ana::column::calculation<double>::execute(part, entry);
  m_decision->execute(part, entry);
}

inline void ana::selection::finalize(const ana::dataset::range &part) {
  ana::column::calculation<double>::finalize(part);
  m_decision->finalize(part);
}

template <typename T>
void ana::selection::set_decision(std::unique_ptr<T> decision) {
  // link value to variable<double>
  m_variable = variable<double>((term<cell_value_t<T>> &)(*decision));
  // keep decision as term
  m_decision = std::move(decision);
}