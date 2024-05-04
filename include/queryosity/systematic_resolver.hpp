#pragma once

#include <set>
#include <string>
#include <type_traits>

#include "systematic.hpp"

namespace queryosity {

class dataflow;

template <typename U> class systematic::resolver {

public:
  using nominal_type = U;

public:
  template <typename> friend class lazy;
  template <typename> friend class todo;
  template <typename> friend class resolver;

public:
  resolver() = default;
  virtual ~resolver() = default;

public:
  virtual void set_variation(const std::string &var_name, U var) = 0;

  virtual U &nominal() = 0;
  virtual U &variation(const std::string &var_name) = 0;

  virtual U const &nominal() const = 0;
  virtual U const &variation(const std::string &var_name) const = 0;

  virtual bool has_variation(const std::string &var_name) const = 0;
  virtual std::set<std::string> get_variation_names() const = 0;
};

} // namespace queryosity