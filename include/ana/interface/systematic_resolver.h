#pragma once

/** @file */

#include <set>
#include <string>
#include <type_traits>

#include "systematic.h"

namespace ana {

class dataflow;

template <typename U> class systematic::resolver {

public:
  using nominal_type = U;

public:
  template <typename> friend class lazy;
  template <typename> friend class delayed;
  template <typename> friend class resolver;

public:
  resolver(dataflow &df);
  virtual ~resolver() = default;

public:
  virtual void set_variation(const std::string &var_name, U &&var) = 0;

  virtual U const &nominal() const = 0;
  virtual U const &variation(const std::string &var_name) const = 0;

  virtual bool has_variation(const std::string &var_name) const = 0;
  virtual std::set<std::string> list_variation_names() const = 0;

protected:
  dataflow *m_df;
};

} // namespace ana

template <typename U>
ana::systematic::resolver<U>::resolver(dataflow &df) : m_df(&df) {}