#pragma once

/** @file */

#include <set>
#include <string>
#include <type_traits>

#include "dataflow.h"

namespace ana {

/**
 * @class dataflow::systematic dataflow_systematic.h
 * interface/dataflow_systematic.h
 * @brief Systematic of a dataflow operation.
 */
template <typename U> class dataflow::systematic {

public:
  using nominal_type = U;

public:
  friend class low;
  template <typename> friend class systematic;

public:
  systematic(dataflow &df);

  virtual ~systematic() = default;

public:
  virtual void set_variation(const std::string &var_name, U &&nom) = 0;

  virtual U const &nominal() const = 0;
  virtual U const &variation(const std::string &var_name) const = 0;

  virtual bool has_variation(const std::string &var_name) const = 0;
  virtual std::set<std::string> list_variation_names() const = 0;

protected:
  dataflow *m_df;
};

template <typename... Nodes>
auto list_all_variation_names(Nodes const &...nodes) -> std::set<std::string>;

} // namespace ana

template <typename U>
ana::dataflow::systematic<U>::systematic(dataflow &df) : m_df(&df) {}

template <typename... Nodes>
auto ana::list_all_variation_names(Nodes const &...nodes)
    -> std::set<std::string> {
  std::set<std::string> variation_names;
  (variation_names.merge(nodes.list_variation_names()), ...);
  return variation_names;
}