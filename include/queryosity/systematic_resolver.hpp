#pragma once

#include <map>
#include <set>
#include <string>
#include <functional>

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

  std::map<std::string, std::reference_wrapper<U>> variations();
  std::map<std::string, std::reference_wrapper<U const>> variations() const;

};

} // namespace queryosity

template <typename U>
std::map<std::string, std::reference_wrapper<U>>
queryosity::systematic::resolver<U>::variations() {
    std::map<std::string, std::reference_wrapper<U>> out;
    for (auto const& name : this->get_variation_names()) {
        out.emplace(name, this->variation(name));
    }
    return out;
}

template <typename U>
std::map<std::string, std::reference_wrapper<U const>>
queryosity::systematic::resolver<U>::variations() const {
    std::map<std::string, std::reference_wrapper<U const>> out;
    for (auto const& name : this->get_variation_names()) {
        out.emplace(name, this->variation(name));  // calls const variation(name) const
    }
    return out;
}
