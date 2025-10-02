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
template <typename Qry> struct output {

public:
  /**
   * @brief Argument constructor.
   * @tparam Args Constructor argument types for @p Qry.
   * @param args Constructor arguments for @p Qry.
   */
  template <typename... Args> output(Args &&...args);
  ~output() = default;

  auto make(dataflow &df) const;

protected:
  std::function<todo<query::booker<Qry>>(dataflow &)> m_make;
};

} // namespace query

} // namespace queryosity

#include "dataflow.hpp"

template <typename Qry>
template <typename... Args>
queryosity::query::output<Qry>::output(Args &&...args)
    : m_make([args_tuple = std::make_tuple(std::forward<Args>(args)...)](
                 dataflow &df) mutable {
        return std::apply(
            [&df](auto &&...args) {
              return df._make<Qry>(std::forward<decltype(args)>(args)...);
            },
            args_tuple);
      }) {}

template <typename Qry>
auto queryosity::query::output<Qry>::make(queryosity::dataflow &df) const {
  return this->m_make(df);
}