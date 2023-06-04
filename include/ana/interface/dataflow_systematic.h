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
 * @brief Systematic of a dataflow action.
 */
template <typename T> template <typename U> class dataflow<T>::systematic {

public:
  using dataflow_type = dataflow<T>;
  using dataset_type = T;
  using nominal_type = U;

public:
  friend class dataflow<T>;
  template <typename> friend class systematic;

public:
  systematic(dataflow<T> &dataflow);

  virtual ~systematic() = default;

public:
  virtual void set_variation(const std::string &var_name, U &&nom) = 0;

  virtual U const &get_nominal() const = 0;
  virtual U const &get_variation(const std::string &var_name) const = 0;

  virtual bool has_variation(const std::string &var_name) const = 0;
  virtual std::set<std::string> list_variation_names() const = 0;

protected:
  dataflow<T> *m_df;
};

template <typename... Nodes>
auto list_all_variation_names(Nodes const &...nodes) -> std::set<std::string>;

} // namespace ana

template <typename... Nodes>
auto ana::list_all_variation_names(Nodes const &...nodes)
    -> std::set<std::string> {
  std::set<std::string> variation_names;
  (variation_names.merge(nodes.list_variation_names()), ...);
  return variation_names;
}