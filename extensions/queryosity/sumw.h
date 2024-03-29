#pragma once

#include <numeric>
#include <unordered_map>
#include <variant>

#include "queryosity.h"

namespace queryosity {

class sumw : public queryosity::query::aggregation<double> {

public:
  sumw() = default;
  ~sumw() = default;

  virtual void count(double w) override;
  virtual double result() const override;
  virtual double merge(std::vector<double> const &results) const override;

protected:
  double m_result = 0.0;
};

} // namespace queryosity

void queryosity::sumw::count(double w) { m_result += w; }

double queryosity::sumw::result() const { return m_result; }

double queryosity::sumw::merge(std::vector<double> const &results) const {
  return std::accumulate(results.begin(), results.end(), 0.0);
}