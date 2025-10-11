#pragma once

#include "query.hpp"
#include "query_fillable.hpp"

namespace queryosity {

/**
 * @brief Query filled with column value(s) per-entry.
 * @tparam Out Output result type.
 * @tparam Cols Input column data types.
 */
template <typename... Cols>
class query::callback : public node,
                                 public query::fillable<Cols...> {

public:
  using vartup_type = std::tuple<column::variable<Cols>...>;

public:
  callback() = default;
  virtual ~callback() = default;

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

template <typename... Cols>
void queryosity::query::callback<Cols...>::count(double w) {
  for (unsigned int ifill = 0; ifill < this->m_fills.size(); ++ifill) {
    std::apply(
        [this, w](const column::variable<Cols> &...obs) {
          this->fill(obs..., w);
        },
        this->m_fills[ifill]);
  }
}