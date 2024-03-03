#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "column.h"
#include "dataset.h"

namespace queryosity {

/**
 * @brief Computation graph of columns.
 * @details `column::computation<Dataset_t>` issues `unique::ptr<Column_t>`
 * for all columns, or their evaluates, used in the analysis.
 * It keeps a raw pointer of each, only if their action needs to be called
 * for each dataset entry (e.g. constant values are not stored).
 */
class column::computation {

public:
  computation() = default;
  virtual ~computation() = default;

public:
  template <typename DS, typename Val>
  auto read(dataset::reader<DS> &ds, unsigned int slot, const std::string &name)
      -> std::unique_ptr<read_column_t<DS, Val>>;

  template <typename Val>
  auto assign(Val const &val) -> std::unique_ptr<column::fixed<Val>>;

  template <typename Def, typename... Args>
  auto define(Args const &...vars) const
      -> std::unique_ptr<queryosity::column::template evaluate_t<Def>>;

  template <typename Ret, typename... Args>
  auto equate(std::function<Ret(Args...)> fn) const -> std::unique_ptr<
      queryosity::column::template evaluate_t<std::function<Ret(Args...)>>>;

  template <typename Def, typename... Cols>
  auto evaluate(column::evaluate<Def> &calc, Cols const &...columns)
      -> std::unique_ptr<Def>;

protected:
  void add_column(column::node &column);

protected:
  std::vector<column::node *> m_columns;
};

} // namespace queryosity

#include "column_equation.h"
#include "column_evaluate.h"
#include "column_fixed.h"
#include "dataset_source.h"

template <typename DS, typename Val>
auto queryosity::column::computation::read(dataset::reader<DS> &ds,
                                           unsigned int slot,
                                           const std::string &name)
    -> std::unique_ptr<read_column_t<DS, Val>> {
  auto rdr = ds.template read_column<Val>(slot, name);
  this->add_column(*rdr);
  return rdr;
}

template <typename Val>
auto queryosity::column::computation::assign(Val const &val)
    -> std::unique_ptr<queryosity::column::fixed<Val>> {
  return std::make_unique<typename column::fixed<Val>>(val);
}

template <typename Def, typename... Args>
auto queryosity::column::computation::define(Args const &...args) const
    -> std::unique_ptr<queryosity::column::template evaluate_t<Def>> {
  return std::make_unique<column::evaluate<Def>>(args...);
}

template <typename Ret, typename... Args>
auto queryosity::column::computation::equate(std::function<Ret(Args...)> fn)
    const -> std::unique_ptr<
        queryosity::column::template evaluate_t<std::function<Ret(Args...)>>> {
  return std::make_unique<
      queryosity::column::template evaluate_t<std::function<Ret(Args...)>>>(fn);
}

template <typename Def, typename... Cols>
auto queryosity::column::computation::evaluate(column::evaluate<Def> &calc,
                                               Cols const &...columns)
    -> std::unique_ptr<Def> {
  auto defn = calc._evaluate(columns...);
  this->add_column(*defn);
  return defn;
}

inline void queryosity::column::computation::add_column(column::node &column) {
  m_columns.push_back(&column);
}