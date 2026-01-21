#include <queryosity.hpp>

#include <boost/histogram.hpp> // make_histogram, regular, weight, indexed
#include <boost/histogram/ostream.hpp>

#include <functional> // std::ref
#include <utility>

namespace queryosity {

namespace boost {

/**
 * @brief `boost::histogram` extension for queryosity
 */
namespace histogram {

/**
 * @ingroup ext
 * @brief Histogram axes for queryosity
 */
namespace axis {

/// Boolean bin edges.
using boolean = ::boost::histogram::axis::boolean<>;
/// Integer bin edges.
using integer = ::boost::histogram::axis::integer<>;
/// Numerical bin edges (regular width).
using regular = ::boost::histogram::axis::regular<>;
/// Numerical bin edges (variable width).
using variable = ::boost::histogram::axis::variable<>;

using axis_t =
    ::boost::histogram::axis::variant<boolean, regular, integer, variable>;

} // namespace axis

using axes_t = std::vector<axis::axis_t>;
using histogram_t = ::boost::histogram::histogram<axes_t>;

/**
 * @ingroup ext
 * @brief N-dimensional histogram query for queryosity.
 * @tparam Vals Input column data types.
 * @details The number of columns data types provided determines the
 * dimensionality of the histogram.
 */
template <typename... Vals>
class histogram
    : public queryosity::query::definition<std::shared_ptr<histogram_t>(
          Vals...)> {

public:
public:
  /**
   * @brief Constructor with axis configurations.
   * @tparam Axes Axis types.
   * @details The number of axis arguments provided must match the
   * dimensionality of the histogram.
   */
  template <typename... Axes> histogram(Axes &&...axes);
  ~histogram() = default;

  /**
   * @brief Fill histogram with input columns.
   * @param columns Input column observables.
   * @param weight Selection weight value.
   */
  virtual void fill(queryosity::column::observable<Vals>... columns,
                    double weight) final override;

  /**
   * @brief Retrieve the result.
   * @return The (smart pointer to) histogram.
   */
  virtual std::shared_ptr<histogram_t> result() const final override;

  /**
   * @brief Merge results from multithreaded runs.
   * @param[in] results Result from each multithreaded slot.
   * @return Merged histogram.
   */
  virtual std::shared_ptr<histogram_t>
  merge(std::vector<std::shared_ptr<histogram_t>> const &results)
      const final override;

protected:
  std::shared_ptr<histogram_t> m_histogram;
};

} // namespace histogram

} // namespace boost

} // namespace queryosity

template <typename... Vals>
template <typename... Axes>
queryosity::boost::histogram::histogram<Vals...>::histogram(Axes &&...axes) {
  m_histogram = std::make_shared<histogram_t>(
      std::move(::boost::histogram::make_weighted_histogram(
          std::forward<Axes>(axes)...)));
}

template <typename... Vals>
void queryosity::boost::histogram::histogram<Vals...>::fill(
    queryosity::column::observable<Vals>... columns, double w) {
  (*m_histogram)(columns.value()..., ::boost::histogram::weight(w));
}

template <typename... Vals>
std::shared_ptr<queryosity::boost::histogram::histogram_t>
queryosity::boost::histogram::histogram<Vals...>::result() const {
  return m_histogram;
}

template <typename... Vals>
std::shared_ptr<queryosity::boost::histogram::histogram_t>
queryosity::boost::histogram::histogram<Vals...>::merge(
    std::vector<std::shared_ptr<histogram_t>> const &results) const {
  auto sum = std::make_shared<histogram_t>(*results[0]);
  sum->reset();
  for (auto const &result : results) {
    *sum += *result;
  }
  return sum;
}