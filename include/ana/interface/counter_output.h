#pragma once

#include "counter.h"

namespace ana {

/**
 * @brief Minimal counter with an output result.
 * @details This ABC should be used for operations that do not require any input
 * columns.
 */
template <typename T> class counter::output : public counter {

public:
  using result_type = T;

public:
  output();
  virtual ~output() = default;

  /**
   * @brief Create and return the result of the counter.
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
  using counter::count;

  virtual void finalize(const dataset::range &) final override;

  T const &get_result() const;

  T const &operator->() const { return this->get_result(); }

  bool is_merged() const;

  void set_result(std::vector<T> const &results);

protected:
  T m_result;
  bool m_merged;
};

} // namespace ana

#include "selection.h"

template <typename T> ana::counter::output<T>::output() : m_merged(false) {}

template <typename T> bool ana::counter::output<T>::is_merged() const {
  return m_merged;
}

template <typename T>
void ana::counter::output<T>::finalize(const ana::dataset::range &) {
  m_result = this->result();
}

template <typename T> T const &ana::counter::output<T>::get_result() const {
  return m_result;
}

template <typename T>
void ana::counter::output<T>::set_result(std::vector<T> const &results) {
  if (!results.size()) {
    throw std::logic_error("merging requires at least one result");
  }
  m_result = this->merge(results);
  m_merged = true;
}
