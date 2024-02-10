#pragma once

#include <unordered_map>
#include <variant>

#include "ana/analogical.h"

template <typename T>
class Column : public ana::aggregation::logic<std::vector<T>(T)> {

public:
  Column() = default;
  ~Column() = default;

  virtual void fill(ana::observable<T>, double) override;
  virtual std::vector<T> result() const override;
  virtual std::vector<T>
  merge(std::vector<std::vector<T>> const &results) const override;

protected:
  std::vector<T> m_result;
};

template <typename T> void Column<T>::fill(ana::observable<T> x, double) {
  m_result.push_back(x.value());
}

template <typename T> std::vector<T> Column<T>::result() const {
  return m_result;
}

template <typename T>
std::vector<T>
Column<T>::merge(std::vector<std::vector<T>> const &results) const {
  std::vector<T> merged;
  for (const auto &result : results) {
    merged.insert(merged.end(), result.begin(), result.end());
  }
  return merged;
}