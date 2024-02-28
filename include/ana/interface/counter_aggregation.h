#pragma once

#include "counter.h"

namespace ana {

/**
 * @brief Minimal counter with an output result.
 * @details This ABC should be used for actions that do not require any input
 * columns.
 */
template <typename T> class counter::aggregation : public node {

public:
  using result_type = T;

public:
  aggregation();
  virtual ~aggregation() = default;

  /**
   * @brief Create and return the result of the counter.
   * @return The result.
   * @detail The aggregation from each concurrent slot, which is returned by
   * value, are collected into a list to be merged into one.
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
  using node::count;

  virtual void finalize(unsigned int) final override;

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

template <typename T>
ana::counter::aggregation<T>::aggregation() : m_merged(false) {}

template <typename T> bool ana::counter::aggregation<T>::is_merged() const {
  return m_merged;
}

template <typename T>
void ana::counter::aggregation<T>::finalize(unsigned int) {
  m_result = this->result();
}

template <typename T>
T const &ana::counter::aggregation<T>::get_result() const {
  return m_result;
}

template <typename T>
void ana::counter::aggregation<T>::set_merged_result(
    std::vector<T> const &results) {
  if (!results.size()) {
    throw std::logic_error("merging requires at least one result");
  }
  m_result = this->merge(results);
  m_merged = true;
}
