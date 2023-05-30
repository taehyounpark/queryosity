#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "column.h"
#include "constant.h"
#include "dataset.h"
#include "definition.h"
#include "equation.h"
#include "representation.h"

namespace ana {

/**
 * @brief Computation graph of columns.
 * @details `column::computation<Dataset_t>` issues `shared::ptr<Column_t>`
 * for all columns, or their evaluators, used in the analysis.
 * It keeps a raw pointer of each, only if their action needs to be called
 * for each dataset entry (e.g. constant values are not stored).
 */
template <typename T>
class column::computation

{

public:
  computation(const dataset::range &part, dataset::reader<T> &reader);
  virtual ~computation() = default;

public:
  template <typename Val>
  auto read(const std::string &name) -> std::shared_ptr<read_column_t<T, Val>>;

  template <typename Val>
  auto constant(Val const &val) -> std::shared_ptr<column::constant<Val>>;

  template <typename Def, typename... Args>
  auto define(Args const &...vars) const
      -> std::shared_ptr<ana::column::template evaluator_t<Def>>;

  template <typename F>
  auto define(F expression) const
      -> std::shared_ptr<ana::column::template evaluator_t<F>>;

  template <typename Def, typename... Cols>
  auto evaluate_column(column::evaluator<Def> &calc, Cols const &...columns)
      -> std::shared_ptr<Def>;

protected:
  void add_column(column &column);

protected:
  const dataset::range m_part;
  dataset::reader<T> *m_reader;
  std::vector<column *> m_columns;
};

} // namespace ana

template <typename T>
ana::column::computation<T>::computation(const dataset::range &part,
                                         dataset::reader<T> &reader)
    : m_reader(&reader), m_part(part) {}

template <typename T>
template <typename Val>
auto ana::column::computation<T>::read(const std::string &name)
    -> std::shared_ptr<read_column_t<T, Val>> {
  using read_column_type = decltype(m_reader->template read_column<Val>(
      std::declval<const dataset::range &>(),
      std::declval<const std::string &>()));
  static_assert(is_shared_ptr_v<read_column_type>,
                "dataset must open a std::shared_ptr of its column reader");
  auto rdr = m_reader->template read_column<Val>(m_part, name);
  this->add_column(*rdr);
  return rdr;
}

template <typename T>
template <typename Val>
auto ana::column::computation<T>::constant(Val const &val)
    -> std::shared_ptr<ana::column::constant<Val>> {
  return std::make_shared<typename column::constant<Val>>(val);
}

template <typename T>
template <typename Def, typename... Args>
auto ana::column::computation<T>::define(Args const &...args) const
    -> std::shared_ptr<ana::column::template evaluator_t<Def>> {
  return std::make_shared<evaluator<Def>>(args...);
}

template <typename T>
template <typename F>
auto ana::column::computation<T>::define(F expression) const
    -> std::shared_ptr<ana::column::template evaluator_t<F>> {
  return std::make_shared<evaluator<ana::column::template equation_t<F>>>(
      expression);
}

template <typename T>
template <typename Def, typename... Cols>
auto ana::column::computation<T>::evaluate_column(column::evaluator<Def> &calc,
                                                  Cols const &...columns)
    -> std::shared_ptr<Def> {
  auto defn = calc.evaluate_column(columns...);
  // only if the evaluated column is a definition
  if constexpr (column::template is_definition_v<Def>) {
    this->add_column(*defn);
  }
  return defn;
}

template <typename T>
void ana::column::computation<T>::add_column(column &column) {
  m_columns.push_back(&column);
}