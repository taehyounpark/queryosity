#pragma once

#include "aggregation.h"

namespace ana {

/**
 * @brief Minimal aggregation with an output result.
 * @details This ABC should be used for operations that do not require any input
 * columns.
 */
template <typename T> class aggregation::output : public aggregation {

public:
  using result_type = T;

public:
  output();
  virtual ~output() = default;

  /**
   * @brief Create and return the result of the aggregation.
   * @return The result.
   * @detail The output from each concurrent slot, which is returned by value,
   * are collected into a list to be merged into one.
   */
  virtual T result() const = 0;

  /**
   * @brief Merge the results from concurrent slots into one.
   * @param results Incoming results.
   * @return The merged result.
   */
  virtual T merge(std::vector<T> const &results) const = 0;

  /**
   * @brief Count an entry for which the booked selection has passed with its
   * weight.
   * @param weight The value of the weight at booked selection for the passed
   * entry.
   */
  using aggregation::aggregate;

  virtual void finalize(const dataset::range &) final override;

  T const &get_result() const;

  T const &operator->() const { return this->get_result(); }

  bool is_merged() const;

  void set_merged_result(std::vector<T> const &results);

protected:
  T m_result;
  bool m_merged;
};

} // namespace ana

#include "selection.h"

template <typename T> ana::aggregation::output<T>::output() : m_merged(false) {}

template <typename T> bool ana::aggregation::output<T>::is_merged() const {
  return m_merged;
}

template <typename T>
void ana::aggregation::output<T>::finalize(const ana::dataset::range &) {
  m_result = this->result();
}

template <typename T> T const &ana::aggregation::output<T>::get_result() const {
  return m_result;
}

template <typename T>
void ana::aggregation::output<T>::set_merged_result(
    std::vector<T> const &results) {
  if (!results.size()) {
    throw std::logic_error("merging requires at least one result");
  }
  m_result = this->merge(results);
  m_merged = true;
}
