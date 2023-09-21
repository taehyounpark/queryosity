#pragma once

#include <numeric>
#include <unordered_map>
#include <variant>

#include "ana/analogical.h"

class entry_count : public ana::aggregation::output<long long> {

public:
  entry_count() = default;
  ~entry_count() = default;

  virtual void count(double) override;
  virtual long long result() const override;
  virtual long long merge(std::vector<long long> results) const override;

protected:
  long long m_count = 0;
};

void entry_count::count(double) { ++m_count; }

long long entry_count::result() const { return m_count; }

long long entry_count::merge(std::vector<long long> results) const {
  return std::accumulate(results.begin(), results.end(), 0);
}