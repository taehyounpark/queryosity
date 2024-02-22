#include <boost/histogram.hpp> // make_histogram, regular, weight, indexed
#include <functional>          // std::ref
#include <utility>

#include "ana/analogical.h"

namespace ana {

namespace hist {

namespace axis {

using boolean = boost::histogram::axis::boolean<>;
using regular = boost::histogram::axis::regular<>;
using integer = boost::histogram::axis::integer<>;
using variable = boost::histogram::axis::variable<>;

using axis_t =
    boost::histogram::axis::variant<boolean, regular, integer, variable>;

} // namespace axis

using axes_t = std::vector<axis::axis_t>;
using hist_t = boost::histogram::histogram<axes_t>;

template <typename... Cols>
class hist : public ana::counter::definition<std::shared_ptr<hist_t>(Cols...)> {

public:
public:
  template <typename... Axes> hist(Axes &&...axes);
  ~hist() = default;

  virtual void fill(ana::observable<Cols>... columns, double w) override;
  virtual std::shared_ptr<hist_t> result() const override;
  virtual std::shared_ptr<hist_t>
  merge(std::vector<std::shared_ptr<hist_t>> const &results) const override;

protected:
  std::shared_ptr<hist_t> m_hist;
};

} // namespace hist

} // namespace ana

template <typename... Cols>
template <typename... Axes>
ana::hist::hist<Cols...>::hist(Axes &&...axes) {
  m_hist = std::make_shared<hist_t>(std::move(
      boost::histogram::make_weighted_histogram(std::forward<Axes>(axes)...)));
}

template <typename... Cols>
void ana::hist::hist<Cols...>::fill(ana::observable<Cols>... columns,
                                    double w) {
  (*m_hist)(columns.value()..., boost::histogram::weight(w));
}

template <typename... Cols>
std::shared_ptr<ana::hist::hist_t> ana::hist::hist<Cols...>::result() const {
  return m_hist;
}

template <typename... Cols>
std::shared_ptr<ana::hist::hist_t> ana::hist::hist<Cols...>::merge(
    std::vector<std::shared_ptr<hist_t>> const &results) const {
  auto sum = std::make_shared<hist_t>(*results[0]);
  sum->reset();
  for (const auto &result : results) {
    *sum += *result;
  }
  return sum;
}