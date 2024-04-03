#pragma once

#include <functional>
#include <memory>
#include <tuple>

#include "column.h"

namespace queryosity {

/**
 * @brief Representation of multipile columns, out of which derived quantites
 * can be calculated.
 */
template <typename Me, typename... Obs>
class column::composition<Me(Obs...)> : public valued<Me> {

public:
  using vartuple_type = std::tuple<variable<Obs>...>;
  using obstuple_type = std::tuple<observable<Obs>...>;

public:
  composition() = default;
  virtual ~composition() = default;

  template <typename... Vals> void set_arguments(const view<Vals> &...args);

  virtual Me const &value() const override;

  template <unsigned int N>
  constexpr auto value() const
      -> decltype(std::get<N>(std::declval<vartuple_type>()).value());

  template <auto N> constexpr auto value() const;

protected:
  vartuple_type m_arguments;
};

} // namespace queryosity

template <typename Me, typename... Obs>
Me const &queryosity::column::composition<Me(Obs...)>::value() const {
  return static_cast<const Me &>(*this);
}

template <typename Me, typename... Obs>
template <typename... Vals>
void queryosity::column::composition<Me(Obs...)>::set_arguments(
    view<Vals> const &...args) {
  static_assert(sizeof...(Obs) == sizeof...(Vals));
  m_arguments = std::make_tuple(std::invoke(
      [](const view<Vals> &args) -> variable<Obs> {
        return variable<Obs>(args);
      },
      args)...);
}

template <typename Me, typename... Obs>
template <unsigned int N>
constexpr auto queryosity::column::composition<Me(Obs...)>::value() const
    -> decltype(std::get<N>(std::declval<vartuple_type>()).value()) {
  return std::get<N>(m_arguments).value();
}

template <typename Me, typename... Obs>
template <auto N>
constexpr auto queryosity::column::composition<Me(Obs...)>::value() const {
  constexpr auto idx = static_cast<std::underlying_type_t<decltype(N)>>(N);
  return std::get<idx>(this->m_arguments).value();
}