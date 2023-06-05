#pragma once

#include "aggregation.h"
#include "aggregation_output.h"

namespace ana {
/**
 * @brief Counter output to be filled with columns using arbitrary logic.
 * @tparam T Output result type.
 * @tparam Obs... Input column data types.
 */
template <typename T, typename... Obs>
class aggregation::logic<T(Obs...)> : public aggregation::output<T> {

public:
  using vartup_type = std::tuple<ana::variable<Obs>...>;

public:
  logic() = default;
  virtual ~logic() = default;

  /**
   * @brief Perform the counting operation for an entry.
   * @param observables The `observable` of each input column.
   * @param weight The weight value of the booked selection for the passed
   * entry.
   * @details This operation is performed N times for a passed entry, where N is
   * the number of `fill` calls made to its `lazy` operation, each with its the
   * set of input columns as provided then.
   */
  virtual void fill(ana::observable<Obs>... observables, double w) = 0;
  virtual void count(double w) final override;

  template <typename... Vals> void enter_columns(term<Vals> const &...cols);

protected:
  std::vector<vartup_type> m_fills;
};

} // namespace ana

#include "column.h"

template <typename T, typename... Obs>
template <typename... Vals>
void ana::aggregation::logic<T(Obs...)>::enter_columns(
    term<Vals> const &...cols) {
  static_assert(sizeof...(Obs) == sizeof...(Vals),
                "dimension mis-match between filled variables & columns.");
  m_fills.emplace_back(cols...);
}

template <typename T, typename... Obs>
void ana::aggregation::logic<T(Obs...)>::count(double w) {
  for (unsigned int ifill = 0; ifill < m_fills.size(); ++ifill) {
    std::apply(
        [this, w](const variable<Obs> &...obs) { this->fill(obs..., w); },
        m_fills[ifill]);
  }
}
