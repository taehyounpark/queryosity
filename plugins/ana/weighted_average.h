#pragma once

#include <numeric>
#include <unordered_map>
#include <variant>

#include "ana/analogical.h"

struct wavg_t {
  double wsum = 0.0;
  double sumw = 0.0;
  operator double() const { return wsum / sumw; }
};

class weighted_average : public ana::aggregation::logic<wavg_t(double)> {

public:
  weighted_average() = default;
  ~weighted_average() = default;

  virtual void fill(ana::observable<double>, double) override;
  virtual wavg_t result() const override;
  virtual wavg_t merge(std::vector<wavg_t> results) const override;

protected:
  wavg_t m_result;
};

void weighted_average::fill(ana::observable<double> x, double w) {
  m_result.wsum += x.value() * w;
  m_result.sumw += w;
}

wavg_t weighted_average::result() const { return m_result; }

wavg_t weighted_average::merge(std::vector<wavg_t> results) const {
  wavg_t merged;
  for (const auto &result : results) {
    merged.wsum += result.wsum;
    merged.sumw += result.sumw;
  }
  return merged;
}