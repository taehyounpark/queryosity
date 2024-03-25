#include <boost/histogram.hpp> // make_histogram, regular, weight, indexed
#include <functional>          // std::ref
#include <utility>

#include "queryosity/queryosity.h"

namespace queryosity {

/**
 * @brief `boost::histogram` extension for queryosity
 */
namespace hist {

/**
 * @ingroup ext
 * @brief Histogram axes for queryosity
 */
namespace axis {

/// Boolean bin edges.
using boolean = boost::histogram::axis::boolean<>;
/// Integer bin edges.
using integer = boost::histogram::axis::integer<>;
/// Numerical bin edges (regular width).
using regular = boost::histogram::axis::regular<>;
/// Numerical bin edges (variable width).
using variable = boost::histogram::axis::variable<>;

using axis_t =
    boost::histogram::axis::variant<boolean, regular, integer, variable>;

} // namespace axis

using axes_t = std::vector<axis::axis_t>;
using hist_t = boost::histogram::histogram<axes_t>;

/**
 * @ingroup ext
 * @brief N-dimensional histogram query for queryosity.
 * @tparam Cols Input column data types.
 * @details The number of columns data types provided determines the
 * dimensionality of the histogram.
 */
template <typename... Cols>
class hist
    : public queryosity::query::definition<std::shared_ptr<hist_t>(Cols...)> {

public:
public:
  /**
   * @brief Constructor with axis configurations.
   * @tparam Axes Axis types.
   * @details The number of axis arguments provided must match the
   * dimensionality of the histogram.
   */
  template <typename... Axes> hist(Axes &&...axes);
  ~hist() = default;

  /**
   * @brief Fill histogram with input columns.
   * @param columns Input column observables.
   * @param weight Selection weight value.
   */
  virtual void fill(queryosity::column::observable<Cols>... columns,
                    double weight) override;

  /**
   * @brief Retrieve the result.
   * @return The (smart pointer to) histogram.
   */
  virtual std::shared_ptr<hist_t> result() const override;

  /**
   * @brief Merge results from multithreaded runs.
   * @param[in] results Result from each multithreaded slot.
   * @return Merged histogram.
   */
  virtual std::shared_ptr<hist_t>
  merge(std::vector<std::shared_ptr<hist_t>> const &results) const override;

protected:
  std::shared_ptr<hist_t> m_hist;
};

} // namespace hist

} // namespace queryosity

template <typename... Cols>
template <typename... Axes>
queryosity::hist::hist<Cols...>::hist(Axes &&...axes) {
  m_hist = std::make_shared<hist_t>(std::move(
      boost::histogram::make_weighted_histogram(std::forward<Axes>(axes)...)));
}

template <typename... Cols>
void queryosity::hist::hist<Cols...>::fill(
    queryosity::column::observable<Cols>... columns, double w) {
  (*m_hist)(columns.value()..., boost::histogram::weight(w));
}

template <typename... Cols>
std::shared_ptr<queryosity::hist::hist_t>
queryosity::hist::hist<Cols...>::result() const {
  return m_hist;
}

template <typename... Cols>
std::shared_ptr<queryosity::hist::hist_t>
queryosity::hist::hist<Cols...>::merge(
    std::vector<std::shared_ptr<hist_t>> const &results) const {
  auto sum = std::make_shared<hist_t>(*results[0]);
  sum->reset();
  for (const auto &result : results) {
    *sum += *result;
  }
  return sum;
}