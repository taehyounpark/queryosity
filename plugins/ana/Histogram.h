#include <boost/histogram.hpp> // make_histogram, regular, weight, indexed
#include <functional>          // std::ref
#include <utility>

#include "ana/analogical.h"

using BooleanAxis = boost::histogram::axis::boolean<>;
using IntegerAxis = boost::histogram::axis::integer<>;
using LinearAxis = boost::histogram::axis::regular<>;
using VariableAxis = boost::histogram::axis::variable<>;

using Axis_t = boost::histogram::axis::variant<BooleanAxis, IntegerAxis,
                                               LinearAxis, VariableAxis>;
using Axes_t = std::vector<Axis_t>;
using Histogram_t = boost::histogram::histogram<Axes_t>;

template <typename... Cols>
class Histogram
    : public ana::aggregation::logic<std::shared_ptr<Histogram_t>(Cols...)> {

public:
public:
  template <typename... Axes> Histogram(Axes &&...axes);
  ~Histogram() = default;

  virtual void fill(ana::observable<Cols>... columns, double w) override;
  virtual std::shared_ptr<Histogram_t> result() const override;
  virtual std::shared_ptr<Histogram_t>
  merge(std::vector<std::shared_ptr<Histogram_t>> results) const override;

protected:
  std::shared_ptr<Histogram_t> m_hist;
};

template <typename... Cols>
template <typename... Axes>
Histogram<Cols...>::Histogram(Axes &&...axes) {
  m_hist = std::make_shared<Histogram_t>(std::move(
      boost::histogram::make_weighted_histogram(std::forward<Axes>(axes)...)));
}

template <typename... Cols>
void Histogram<Cols...>::fill(ana::observable<Cols>... columns, double w) {
  (*m_hist)(columns.value()..., boost::histogram::weight(w));
}

template <typename... Cols>
std::shared_ptr<Histogram_t> Histogram<Cols...>::result() const {
  return m_hist;
}

template <typename... Cols>
std::shared_ptr<Histogram_t> Histogram<Cols...>::merge(
    std::vector<std::shared_ptr<Histogram_t>> results) const {
  auto sum = std::make_shared<Histogram_t>(*results[0]);
  sum->reset();
  for (const auto &result : results) {
    *sum += *result;
  }
  return sum;
}