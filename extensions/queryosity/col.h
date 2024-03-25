#pragma once

#include <unordered_map>
#include <variant>

#include "queryosity/queryosity.h"

namespace queryosity {

template <typename T>
class col : public queryosity::query::definition<std::vector<T>(T)> {

public:
  col() = default;
  ~col() = default;

  virtual void fill(queryosity::column::observable<T>, double) override;
  virtual std::vector<T> result() const override;
  virtual std::vector<T>
  merge(std::vector<std::vector<T>> const &results) const override;

protected:
  std::vector<T> m_result;
};

} // namespace queryosity

template <typename T>
void queryosity::col<T>::fill(queryosity::column::observable<T> x, double) {
  m_result.push_back(x.value());
}

template <typename T> std::vector<T> queryosity::col<T>::result() const {
  return m_result;
}

template <typename T>
std::vector<T>
queryosity::col<T>::merge(std::vector<std::vector<T>> const &results) const {
  std::vector<T> merged;
  for (const auto &result : results) {
    merged.insert(merged.end(), result.begin(), result.end());
  }
  return merged;
}