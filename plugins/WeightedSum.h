#pragma once

#include <numeric>
#include <unordered_map>
#include <variant>

#include "ana/analogical.h"

class WeightedSum : public ana::counter::logic<double(double)> {

public:
  WeightedSum() = default;
  ~WeightedSum() = default;

  virtual void fill(ana::observable<double>, double) override;
  virtual double result() const override;
  virtual double merge(std::vector<double> const &results) const override;

protected:
  double m_result;
};

void WeightedSum::fill(ana::observable<double> x, double w) {
  m_result += w * x.value();
}

double WeightedSum::result() const { return m_result; }

double WeightedSum::merge(std::vector<double> const &results) const {
  return std::accumulate(results.begin(), results.end(), 0.0);
}