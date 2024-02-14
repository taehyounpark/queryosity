#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "column.h"
#include "dataset_source.h"

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
  auto read(dataset::source<DS> &ds, const dataset::range &part,
            const std::string &name) -> std::unique_ptr<read_column_t<DS, Val>>;

  template <typename Val>
  auto assign(Val const &val) -> std::unique_ptr<column::fixed<Val>>;

  template <typename Def, typename... Args>
  auto define(Args const &...vars) const
      -> std::unique_ptr<ana::column::template evaluator_t<Def>>;

  template <typename Ret, typename... Args>
  auto equate(std::function<Ret(Args...)> fn) const -> std::unique_ptr<
      ana::column::template evaluator_t<std::function<Ret(Args...)>>>;

  template <typename Def, typename... Cols>
  auto evaluate_column(column::evaluator<Def> &calc, Cols const &...columns)
      -> std::unique_ptr<Def>;

protected:
  void add_column(column::column_base &column);

protected:
  std::vector<column::column_base *> m_columns;
};

} // namespace ana

#include "column_equation.h"
#include "column_evaluator.h"
#include "column_fixed.h"

template <typename DS, typename Val>
auto ana::column::computation::read(dataset::source<DS> &ds,
                                    const ana::dataset::range &part,
                                    const std::string &name)
    -> std::unique_ptr<read_column_t<DS, Val>> {
  auto rdr = ds.template read_column<Val>(part, name);
  this->add_column(*rdr);
  return rdr;
}

template <typename Val>
auto ana::column::computation::assign(Val const &val)
    -> std::unique_ptr<ana::column::fixed<Val>> {
  return std::make_unique<typename column::fixed<Val>>(val);
}

template <typename Def, typename... Args>
auto ana::column::computation::define(Args const &...args) const
    -> std::unique_ptr<ana::column::template evaluator_t<Def>> {
  return std::make_unique<evaluator<Def>>(args...);
}

template <typename Ret, typename... Args>
auto ana::column::computation::equate(std::function<Ret(Args...)> fn) const
    -> std::unique_ptr<
        ana::column::template evaluator_t<std::function<Ret(Args...)>>> {
  return std::make_unique<
      ana::column::template evaluator_t<std::function<Ret(Args...)>>>(fn);
}

template <typename Def, typename... Cols>
auto ana::column::computation::evaluate_column(column::evaluator<Def> &calc,
                                               Cols const &...columns)
    -> std::unique_ptr<Def> {
  auto defn = calc.evaluate_column(columns...);
  this->add_column(*defn);
  return defn;
}

inline void ana::column::computation::add_column(column::column_base &column) {
  m_columns.push_back(&column);
}