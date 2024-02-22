#pragma once

#include <numeric>
#include <unordered_map>
#include <variant>

#include "ana/analogical.h"

namespace ana {

class wsum : public ana::counter::definition<double(double)> {

public:
  wsum() = default;
  ~wsum() = default;

  virtual void fill(ana::observable<double>, double) override;
  virtual double result() const override;
  virtual double merge(std::vector<double> const &results) const override;

protected:
  double m_result;
};

} // namespace ana

void ana::wsum::fill(ana::observable<double> x, double w) {
  m_result += w * x.value();
}

double ana::wsum::result() const { return m_result; }

double ana::wsum::merge(std::vector<double> const &results) const {
  return std::accumulate(results.begin(), results.end(), 0.0);
}