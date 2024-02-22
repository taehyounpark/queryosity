#pragma once

#include <numeric>
#include <unordered_map>
#include <variant>

#include "ana/analogical.h"

namespace ana {

class sumw : public ana::counter::implementation<double> {

public:
  sumw() = default;
  ~sumw() = default;

  virtual void count(double w) override;
  virtual double result() const override;
  virtual double merge(std::vector<double> const &results) const override;

protected:
  double m_result = 0.0;
};

} // namespace ana

void ana::sumw::count(double w) { m_result += w; }

double ana::sumw::result() const { return m_result; }

double ana::sumw::merge(std::vector<double> const &results) const {
  return std::accumulate(results.begin(), results.end(), 0.0);
}