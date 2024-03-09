#pragma once

#include <functional>
#include <memory>
#include <tuple>

#include "query.h"

namespace queryosity {

class dataflow;

namespace query {

template <typename Cntr> class plan {

public:
  template <typename... Args> plan(Args const &...args);

  auto _make(dataflow &df) const;

protected:
  std::function<todo<query::book<Cntr>>(dataflow &)> m_make;
};

} // namespace query

} // namespace queryosity

#include "dataflow.h"

template <typename Cntr>
template <typename... Args>
queryosity::query::plan<Cntr>::plan(Args const &...args)
    : m_make([args...](dataflow &df) { return df._make<Cntr>(args...); }) {}

template <typename Cntr>
auto queryosity::query::plan<Cntr>::_make(queryosity::dataflow &df) const {
  return this->m_make(df);
}