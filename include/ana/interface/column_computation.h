#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "column.h"

namespace ana {

/**
 * @brief Computation graph of columns.
 * @details `column::computation<Dataset_t>` issues `unique::ptr<Column_t>`
 * for all columns, or their evaluators, used in the analysis.
 * It keeps a raw pointer of each, only if their operation needs to be called
 * for each dataset entry (e.g. constant values are not stored).
 */
class column::computation {

public:
  computation() = default;
  virtual ~computation() = default;

public:
  template <typename DS, typename Val>
  auto read(dataset::input<DS> &ds, const dataset::range &part,
            const std::string &name) -> std::unique_ptr<read_column_t<DS, Val>>;

  template <typename Val>
  auto constant(Val const &val) -> std::unique_ptr<column::constant<Val>>;

  template <typename Def, typename... Args>
  auto define(Args const &...vars) const
      -> std::unique_ptr<ana::column::template evaluator_t<Def>>;

  template <typename F>
  auto define(F expression) const
      -> std::unique_ptr<ana::column::template evaluator_t<F>>;

  template <typename Def, typename... Cols>
  auto evaluate_column(column::evaluator<Def> &calc, Cols const &...columns)
      -> std::unique_ptr<Def>;

protected:
  void add_column(column &column);

protected:
  std::vector<column *> m_columns;
};

} // namespace ana

#include "column_constant.h"
#include "column_equation.h"
#include "column_evaluator.h"

template <typename DS, typename Val>
auto ana::column::computation::read(dataset::input<DS> &ds,
                                    const ana::dataset::range &part,
                                    const std::string &name)
    -> std::unique_ptr<read_column_t<DS, Val>> {
  auto rdr = ds.template read_column<Val>(part, name);
  this->add_column(*rdr);
  return rdr;
}

template <typename Val>
auto ana::column::computation::constant(Val const &val)
    -> std::unique_ptr<ana::column::constant<Val>> {
  return std::make_unique<typename column::constant<Val>>(val);
}

template <typename Def, typename... Args>
auto ana::column::computation::define(Args const &...args) const
    -> std::unique_ptr<ana::column::template evaluator_t<Def>> {
  return std::make_unique<evaluator<Def>>(args...);
}

template <typename F>
auto ana::column::computation::define(F expression) const
    -> std::unique_ptr<ana::column::template evaluator_t<F>> {
  return std::make_unique<evaluator<ana::column::template equation_t<F>>>(
      expression);
}

template <typename Def, typename... Cols>
auto ana::column::computation::evaluate_column(column::evaluator<Def> &calc,
                                               Cols const &...columns)
    -> std::unique_ptr<Def> {
  auto defn = calc.evaluate_column(columns...);
  // only if the evaluated column is not a representation, which does not need
  // to executed
  // ... but it still needs to be initialized and finalized!
  // if constexpr (!column::template is_representation_v<Def>) {
  this->add_column(*defn);
  // }
  return defn;
}

inline void ana::column::computation::add_column(column &column) {
  m_columns.push_back(&column);
}