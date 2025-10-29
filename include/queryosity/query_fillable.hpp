#pragma once

#include "query.hpp"

namespace queryosity {

namespace column {

template <typename T> class observable;

template <typename T> class variable;

template <typename T> class view;

} // namespace column

/**
 * @brief Query filled with column value(s) per-entry.
 * @tparam Out Output result type.
 * @tparam Ins Input column data types.
 */
template <typename... Ins> class query::fillable {

public:
  using vartup_type = std::tuple<column::variable<Ins>...>;

public:
  fillable() = default;
  virtual ~fillable() = default;

  /**
   * @brief Perform the counting action for an entry.
   * @param[in] observables Input column observables.
   * @param[in] weight The weight value of the booked selection for the passed
   * entry.
   * @details This action is performed N times for a passed entry, where N is
   * the number of `fill()` calls made to its lazy node.
   */
  virtual void fill(column::observable<Ins>... observables, double weight) = 0;

  template <typename... Vals>
  void enter_columns(column::view<Vals> const &...cols);

protected:
  std::vector<vartup_type> m_fills;
};

} // namespace queryosity

#include "column.hpp"

template <typename... Ins>
template <typename... Vals>
void queryosity::query::fillable<Ins...>::enter_columns(
    column::view<Vals> const &...cols) {
  static_assert(sizeof...(Ins) == sizeof...(Vals),
                "dimension mis-match between filled variables & columns.");
  m_fills.emplace_back(cols...);
}
