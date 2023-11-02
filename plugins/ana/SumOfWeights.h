#pragma once

#include <numeric>
#include <unordered_map>
#include <variant>

#include "ana/analogical.h"

namespace {

class SumOfWeights : public ana::aggregation::output<double> {

public:
  SumOfWeights() = default;
  ~SumOfWeights() = default;

  virtual void count(double w) override;
  virtual double result() const override;
  virtual double merge(std::vector<double> results) const override;

protected:
  double m_result = 0.0;
};

} // namespace

void SumOfWeights::count(double w) { m_result += w; }

double SumOfWeights::result() const { return m_result; }

double SumOfWeights::merge(std::vector<double> results) const {
  return std::accumulate(results.begin(), results.end(), 0.0);
}