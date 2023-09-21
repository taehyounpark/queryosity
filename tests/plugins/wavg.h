#pragma once

#include <numeric>
#include <unordered_map>
#include <variant>

#include "ana/analogical.h"

namespace {

struct wavg_val_t {
  double wsum = 0.0;
  double sumw = 0.0;
  operator double() const { return wsum / sumw; }
};

class wavg : public ana::aggregation::logic<wavg_val_t(double)> {

public:
  wavg() = default;
  ~wavg() = default;

  virtual void fill(ana::observable<double>, double) override;
  virtual wavg_val_t result() const override;
  virtual wavg_val_t merge(std::vector<wavg_val_t> results) const override;

protected:
  wavg_val_t m_result;
};

} // namespace

void wavg::fill(ana::observable<double> x, double w) {
  m_result.wsum += x.value() * w;
  m_result.sumw += w;
}

wavg_val_t wavg::result() const { return m_result; }

wavg_val_t wavg::merge(std::vector<wavg_val_t> results) const {
  wavg_val_t merged;
  for (const auto &result : results) {
    merged.wsum += result.wsum;
    merged.sumw += result.sumw;
  }
  return merged;
}