#pragma once

#include <functional>
#include <memory>
#include <tuple>

#include "query.hpp"

namespace queryosity {

class dataflow;

namespace query {

/**
 * @ingroup api
 * @brief Argument to specify a query in the dataflow.
 * @tparam Qry Concrete implementation of
 * `queryosity::query::definition<T(Obs...)>`.
 */
template <typename Qry> struct result {

public:
  /**
   * @brief Argument constructor.
   * @tparam Args Constructor argument types for @p Qry.
   * @param args Constructor arguments for @p Qry.
   */
  template <typename... Args> result(Args... args);
  ~result() = default;

  auto make(dataflow &df) const;

protected:
  std::function<todo<query::booker<Qry>>(dataflow &)> m_make;
};

} // namespace query

} // namespace queryosity

#include "dataflow.hpp"

template <typename Qry>
template <typename... Args>
queryosity::query::result<Qry>::result(Args... args)
    : m_make([args...](dataflow &df) { return df._make<Qry>(args...); }) {}

template <typename Qry>
auto queryosity::query::result<Qry>::make(queryosity::dataflow &df) const {
  return this->m_make(df);
}