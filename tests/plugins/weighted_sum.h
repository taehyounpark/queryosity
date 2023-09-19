#pragma once

#include <unordered_map>
#include <variant>
#include <numeric>

#include "ana/interface.h"

class weighted_sum : public ana::aggregation::logic<int(int)> {

public:
  weighted_sum() = default;
  ~weighted_sum() = default;

  virtual void fill(ana::observable<int>, double) override;
	virtual int result() const override;
	virtual int merge(std::vector<int> results) const override;

protected:
  int m_result;

};

void weighted_sum::fill(ana::observable<int> x, double w) {
  m_result += w*x.value();
}

int weighted_sum::result() const {
  return m_result;
}

int weighted_sum::merge(std::vector<int> results) const {
  return std::accumulate(results.begin(), results.end(),0LL);
}