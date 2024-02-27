#pragma once

#include <functional>
#include <memory>
#include <tuple>

#include "counter.h"

namespace ana {

class dataflow;

namespace counter {

template <typename Cntr> class output {

public:
  template <typename... Args> output(Args const &...args);

  auto _aggregate(dataflow &df) const;

protected:
  std::function<todo<counter::booker<Cntr>>(dataflow &)> m_aggregate;
};

} // namespace counter

} // namespace ana

#include "dataflow.h"

template <typename Cntr>
template <typename... Args>
ana::counter::output<Cntr>::output(Args const &...args)
    : m_aggregate(
          [args...](dataflow &df) { return df._aggregate<Cntr>(args...); }) {}

template <typename Cntr>
auto ana::counter::output<Cntr>::_aggregate(ana::dataflow &df) const {
  return this->m_aggregate(df);
}