#pragma once

#include <functional>
#include <memory>
#include <tuple>

#include "query.h"

namespace queryosity {

class dataflow;

namespace query {

/**
 * @brief Plan a query
 * @tparam Qry Concrete implementation of
 * queryosity::query::definition<T(Obs...)>.
 */
template <typename Qry> class plan {

public:
  /**
   * @brief Constructor.
   * @tparam Args `Qry` constructor argument types.
   * @param args Constructor arguments.
   */
  template <typename... Args> plan(Args const &...args);

  auto _make(dataflow &df) const;

protected:
  std::function<todo<query::booker<Qry>>(dataflow &)> m_make;
};

} // namespace query

} // namespace queryosity

#include "dataflow.h"

template <typename Qry>
template <typename... Args>
queryosity::query::plan<Qry>::plan(Args const &...args)
    : m_make([args...](dataflow &df) { return df._make<Qry>(args...); }) {}

template <typename Qry>
auto queryosity::query::plan<Qry>::_make(queryosity::dataflow &df) const {
  return this->m_make(df);
}