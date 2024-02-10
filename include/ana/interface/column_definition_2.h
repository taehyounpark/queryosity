#pragma once

#include <functional>
#include <memory>
#include <tuple>

#include "column_evaluator.h"
#include "delayed.h"

namespace ana {

class dataflow;

template <typename Def> class column::definition {

public:
  template <typename... Args> definition(Args const &...args);

  auto _define(dataflow &df) const;

protected:
  std::function<delayed<evaluator_t<Def>>(dataflow &)> m_define;
};

} // namespace ana

#include "dataflow.h"

template <typename Def>
template <typename... Args>
ana::column::definition<Def>::definition(Args const &...args) {
  m_define = [args...](dataflow &df) { return df._define<Def>(args...); };
}

template <typename Def>
auto ana::column::definition<Def>::_define(dataflow &df) const {
  return this->m_define(df);
}
