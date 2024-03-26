#pragma once

#include "systematic.h"

#include <string>
#include <type_traits>

namespace queryosity {

template <typename T> class lazy;

namespace systematic {

template <typename Lzy, typename... Vars>
auto vary(systematic::nominal<Lzy> const &nom, Vars const &...vars);

} // namespace systematic

} // namespace queryosity

#include "column.h"

template <typename Lzy, typename... Vars>
auto queryosity::systematic::vary(systematic::nominal<Lzy> const &nom,
                                  Vars const &...vars) {
  using action_type = typename Lzy::action_type;
  using value_type = column::template value_t<action_type>;
  using nominal_type = lazy<column::valued<value_type>>;
  using varied_type = typename nominal_type::varied;
  varied_type syst(nom.get().template to<value_type>());
  (syst.set_variation(vars.name(), vars.get().template to<value_type>()), ...);
  return syst;
}