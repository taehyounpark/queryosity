#pragma once

#include <unordered_map>
#include <variant>
#include <numeric>

#include "ana/interface.h"

class sum_of_weights : public ana::aggregation::output<double> {

public:
  sum_of_weights() = default;
  ~sum_of_weights() = default;

  virtual void count(double w) override;
	virtual double result() const override;
	virtual double merge(std::vector<double> results) const override;

protected:
  double m_result = 0.0;

};

void sum_of_weights::count(double w) {
  m_result += w;
}

double sum_of_weights::result() const {
  return m_result;
}

double sum_of_weights::merge(std::vector<double> results) const {
  return std::accumulate(results.begin(), results.end(),0.0);
}