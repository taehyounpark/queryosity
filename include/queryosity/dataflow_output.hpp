#pragma once

#include <map>

#include "dataflow.hpp"
#include "systematic.hpp"

namespace queryosity {

template <typename Dmpr> class dataflow::output {

public:
  output(Dmpr& dmpr);
  ~output() = default;

  template <typename Qry>
  void save(Qry const& qry);

protected:
  output::dumper<Dmpr> *m_dmpr;
  
};

} // namespace queryosity

#include "lazy.hpp"
#include "lazy_varied.hpp"

