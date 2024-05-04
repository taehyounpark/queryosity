#pragma once

#include <numeric>
#include <unordered_map>
#include <variant>

#include <queryosity.hpp>

namespace queryosity {

class wsum : public queryosity::query::definition<double(double)> {

public:
  wsum() = default;
  ~wsum() = default;

  virtual void fill(queryosity::column::observable<double>, double) override;
  virtual double result() const override;
  virtual double merge(std::vector<double> const &results) const override;

protected:
  double m_result;
};

} // namespace queryosity

void queryosity::wsum::fill(queryosity::column::observable<double> x,
                            double w) {
  m_result += w * x.value();
}

double queryosity::wsum::result() const { return m_result; }

double queryosity::wsum::merge(std::vector<double> const &results) const {
  return std::accumulate(results.begin(), results.end(), 0.0);
}