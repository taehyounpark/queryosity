#pragma once

#include "query.h"
#include "query_aggregation.h"

namespace queryosity {

/**
 * @brief Query filled with column value(s) per-entry.
 * @tparam T Output result type.
 * @tparam Obs... Input column data types.
 */
template <typename T, typename... Obs>
class query::definition<T(Obs...)> : public query::aggregation<T> {

public:
  using vartup_type = std::tuple<column::variable<Obs>...>;

public:
  definition() = default;
  virtual ~definition() = default;

  /**
   * @brief Perform the counting action for an entry.
   * @param observables The `observable` of each input column.
   * @param weight The weight value of the booked selection for the passed
   * entry.
   * @details This action is performed N times for a passed entry, where N is
   * the number of `fill` calls made during its `todo` configuration.
   */
  virtual void fill(column::observable<Obs>... observables, double w) = 0;
  virtual void count(double w) final override;

  template <typename... Vals>
  void enter_columns(column::view<Vals> const &...cols);

protected:
  std::vector<vartup_type> m_fills;
};

} // namespace queryosity

#include "column.h"

template <typename T, typename... Obs>
template <typename... Vals>
void queryosity::query::definition<T(Obs...)>::enter_columns(
    column::view<Vals> const &...cols) {
  static_assert(sizeof...(Obs) == sizeof...(Vals),
                "dimension mis-match between filled variables & columns.");
  m_fills.emplace_back(cols...);
}

template <typename T, typename... Obs>
void queryosity::query::definition<T(Obs...)>::count(double w) {
  for (unsigned int ifill = 0; ifill < m_fills.size(); ++ifill) {
    std::apply(
        [this, w](const column::variable<Obs> &...obs) {
          this->fill(obs..., w);
        },
        m_fills[ifill]);
  }
}
