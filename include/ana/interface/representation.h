#pragma once

#include <functional>
#include <memory>
#include <tuple>

#include "calculation.h"
#include "column.h"

namespace ana {

template <typename Ret, typename... Obs>
class column::representation<Ret(Obs...)> : public term<Ret> {

public:
  using vartuple_type = std::tuple<variable<Obs>...>;
  using obstuple_type = std::tuple<observable<Obs>...>;

public:
  representation() = default;
  virtual ~representation() = default;

  template <typename... Vals> void set_arguments(const cell<Vals> &...args);

  virtual Ret const &value() const override;

  template <unsigned int N>
  constexpr auto value() const
      -> decltype(std::get<N>(std::declval<vartuple_type>()).value());

  template <auto N> constexpr auto value() const;

protected:
  vartuple_type m_arguments;
};

} // namespace ana

template <typename Ret, typename... Obs>
Ret const &ana::column::representation<Ret(Obs...)>::value() const {
  return static_cast<const Ret &>(*this);
}

template <typename Ret, typename... Obs>
template <typename... Vals>
void ana::column::representation<Ret(Obs...)>::set_arguments(
    cell<Vals> const &...args) {
  static_assert(sizeof...(Obs) == sizeof...(Vals));
  m_arguments = std::make_tuple(std::invoke(
      [](const cell<Vals> &args) -> variable<Obs> {
        return variable<Obs>(args);
      },
      args)...);
}

template <typename Ret, typename... Obs>
template <unsigned int N>
constexpr auto ana::column::representation<Ret(Obs...)>::value() const
    -> decltype(std::get<N>(std::declval<vartuple_type>()).value()) {
  return std::get<N>(m_arguments).value();
}

template <typename Ret, typename... Obs>
template <auto N>
constexpr auto ana::column::representation<Ret(Obs...)>::value() const {
  constexpr auto idx = static_cast<std::underlying_type_t<decltype(N)>>(N);
  return std::get<idx>(this->m_arguments).value();
}