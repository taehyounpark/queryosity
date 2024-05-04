#pragma once

#include "query.hpp"
#include "query_aggregation.hpp"
#include "query_fillable.hpp"

namespace queryosity {

/**
 * @brief Query filled with column value(s) per-entry.
 * @tparam Out Output result type.
 * @tparam Ins Input column data types.
 */
template <typename Out, typename... Ins>
class query::definition<Out(Ins...)> : public query::aggregation<Out>,
                                       public query::fillable<Ins...> {

public:
  using vartup_type = std::tuple<column::variable<Ins>...>;

public:
  definition() = default;
  virtual ~definition() = default;

  /**
   * @brief Perform the counting action for an entry.
   * @param[in] observables Input column observables.
   * @param[in] weight The weight value of the booked selection for the passed
   * entry.
   * @details This action is performed N times for a passed entry, where N is
   * the number of `fill()` calls made to its lazy node.
   */
  virtual void count(double w) final override;
};

} // namespace queryosity

template <typename Out, typename... Ins>
void queryosity::query::definition<Out(Ins...)>::count(double w) {
  for (unsigned int ifill = 0; ifill < this->m_fills.size(); ++ifill) {
    std::apply(
        [this, w](const column::variable<Ins> &...obs) {
          this->fill(obs..., w);
        },
        this->m_fills[ifill]);
  }
}