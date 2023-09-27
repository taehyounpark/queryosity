#pragma once

#include <numeric>
#include <unordered_map>
#include <variant>

#include "ana/analogical.h"

namespace {

template <typename T>
class vecx : public ana::aggregation::logic<std::vector<T>(T)> {

public:
  vecx() = default;
  ~vecx() = default;

  virtual void fill(ana::observable<T>, double) override;
  virtual std::vector<T> result() const override;
  virtual std::vector<T>
  merge(std::vector<std::vector<T>> results) const override;

protected:
  std::vector<T> m_result;
};

} // namespace

template <typename T> void vecx<T>::fill(ana::observable<T> x, double) {
  m_result.push_back(x.value());
}

template <typename T> std::vector<T> vecx<T>::result() const {
  return m_result;
}

template <typename T>
std::vector<T> vecx<T>::merge(std::vector<std::vector<T>> results) const {
  std::vector<T> merged;
  for (const auto &result : results) {
    merged.insert(merged.end(), result.begin(), result.end());
  }
  return merged;
}