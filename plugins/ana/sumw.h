#pragma once

#include <numeric>
#include <unordered_map>
#include <variant>

#include "ana/analogical.h"

namespace {

class sumw : public ana::aggregation::output<double> {

public:
  sumw() = default;
  ~sumw() = default;

  virtual void count(double w) override;
  virtual double result() const override;
  virtual double merge(std::vector<double> results) const override;

protected:
  double m_result = 0.0;
};

} // namespace

void sumw::count(double w) { m_result += w; }

double sumw::result() const { return m_result; }

double sumw::merge(std::vector<double> results) const {
  return std::accumulate(results.begin(), results.end(), 0.0);
}