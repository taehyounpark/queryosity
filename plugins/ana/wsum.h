#pragma once

#include <numeric>
#include <unordered_map>
#include <variant>

#include "ana/analogical.h"

class wsum : public ana::aggregation::logic<double(double)> {

public:
  wsum() = default;
  ~wsum() = default;

  virtual void fill(ana::observable<double>, double) override;
  virtual double result() const override;
  virtual double merge(std::vector<double> results) const override;

protected:
  double m_result;
};

void wsum::fill(ana::observable<double> x, double w) {
  m_result += w * x.value();
}

double wsum::result() const { return m_result; }

double wsum::merge(std::vector<double> results) const {
  return std::accumulate(results.begin(), results.end(), 0.0);
}