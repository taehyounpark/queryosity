#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "column.hpp"
#include "dataset.hpp"

namespace queryosity {

namespace column {

class computation {

public:
  computation() = default;
  virtual ~computation() = default;

public:
  template <typename DS, typename Val>
  auto read(dataset::reader<DS> &ds, unsigned int slot, const std::string &name)
      -> read_column_t<DS, Val> *;

  template <typename Val> auto assign(Val const &val) -> fixed<Val> *;

  template <typename To, typename Col>
  auto convert(Col const &col) -> conversion<To, value_t<Col>> *;

  template <typename Def, typename... Args>
  auto define(Args const &...vars) const
      -> std::unique_ptr<evaluator<Def>>;

  template <typename Ret, typename... Args>
  auto equate(std::function<Ret(Args...)> fn) const -> std::unique_ptr<
      evaluator<equation<std::decay_t<Ret>(std::decay_t<Args>...)>>>;

  template <typename Def, typename... Cols>
  auto evaluate(evaluator<Def> const&calc, Cols const &...cols) -> Def *;

protected:
  template <typename Col> auto add_column(std::unique_ptr<Col> col) -> Col *;

protected:
  std::vector<std::unique_ptr<column::node>> m_columns;  //!
};

}

} // namespace queryosity

#include "column_conversion.hpp"
#include "column_equation.hpp"
#include "column_evaluator.hpp"
#include "column_fixed.hpp"
#include "dataset_reader.hpp"

template <typename DS, typename Val>
auto queryosity::column::computation::read(dataset::reader<DS> &ds,
                                           unsigned int slot,
                                           const std::string &name)
    -> read_column_t<DS, Val> * {
  auto rdr = ds.template read_column<Val>(slot, name);
  return this->add_column(std::move(rdr));
}

template <typename Val>
auto queryosity::column::computation::assign(Val const &val) -> fixed<Val> * {
  auto cnst = std::make_unique<typename column::fixed<Val>>(val);
  return this->add_column(std::move(cnst));
}

template <typename To, typename Col>
auto queryosity::column::computation::convert(Col const &col)
    -> conversion<To, value_t<Col>> * {
  auto cnv = std::make_unique<conversion<To, value_t<Col>>>(col);
  cnv->set_arguments(col);
  return this->add_column(std::move(cnv));
}

template <typename Def, typename... Args>
auto queryosity::column::computation::define(Args const &...args) const
    -> std::unique_ptr<evaluator<Def>> {
  return std::make_unique<evaluator<Def>>(args...);
}

template <typename Ret, typename... Args>
auto queryosity::column::computation::equate(std::function<Ret(Args...)> fn)
    const -> std::unique_ptr<
      evaluator<equation<std::decay_t<Ret>(std::decay_t<Args>...)>>> {
  return std::make_unique<
      evaluator<equation<std::decay_t<Ret>(std::decay_t<Args>...)>>>(
      fn);
}

template <typename Def, typename... Cols>
auto queryosity::column::computation::evaluate(evaluator<Def> const &calc,
                                               Cols const &...cols) -> Def * {
  auto defn = calc.evaluate(cols...);
  return this->add_column(std::move(defn));
}

template <typename Col>
auto queryosity::column::computation::add_column(std::unique_ptr<Col> col)
    -> Col * {
  auto out = col.get();
  m_columns.push_back(std::move(col));
  return out;
}