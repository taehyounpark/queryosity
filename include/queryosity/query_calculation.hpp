#pragma once

#include "query.hpp"
#include "query_fillable.hpp"

#include <vector>

namespace queryosity
{

namespace query
{

template <typename T> class calculation : public node, public fillable<T>
{

public:
  calculation() = default;
  ~calculation() = default;

  virtual void initialize(unsigned int, unsigned long long, unsigned long long) final override;
  virtual void fill(column::observable<T>, double) final override;
  virtual void finalize(unsigned int) final override;
};

} // namespace query

} // namespace queryosity

template <typename T>
void queryosity::query::calculation<T>::initialize(unsigned int, unsigned long long begin, unsigned long long end)
{}

template <typename T> void queryosity::query::calculation<T>::fill(column::observable<T> x, double)
{
  (void)x.value();
}

template <typename T> void queryosity::query::calculation<T>::finalize(unsigned int)
{}