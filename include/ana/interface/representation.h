#pragma once

#include <functional>
#include <memory>
#include <tuple>

#include "calculation.h"
#include "column.h"

namespace ana {

template <typename Ret>
template <typename... Obs>
class term<Ret>::representation_of : public term<Ret> {

public:
  using vartuple_type = std::tuple<variable<Obs>...>;
  using obstuple_type = std::tuple<observable<Obs>...>;

public:
  representation_of() = default;
  virtual ~representation_of() = default;

  template <typename... Vals> void set_arguments(const cell<Vals> &...args);

  virtual Ret const &value() const override;

  template <unsigned int N>
  constexpr auto value() const
      -> decltype(std::get<N>(std::declval<vartuple_type>()).value());

  template <auto en> constexpr auto value() const;

protected:
  vartuple_type m_arguments;
};

template <typename Ret, typename... Obs>
class column::representation<Ret(Obs...)>
    : public term<Ret>::template representation_of<Obs...> {

public:
  using vartuple_type = typename term<Ret>::template representation_of<Ret(
      Obs...)>::vartuple_type;
  using obstuple_type = typename term<Ret>::template representation_of<Ret(
      Obs...)>::obstuple_type;

public:
  representation() = default;
  virtual ~representation() = default;
};

} // namespace ana

template <typename Ret>
template <typename... Obs>
Ret const &ana::term<Ret>::representation_of<Obs...>::value() const {
  return static_cast<const Ret &>(*this);
}

template <typename Ret>
template <typename... Obs>
template <typename... Vals>
void ana::term<Ret>::representation_of<Obs...>::set_arguments(
    cell<Vals> const &...args) {
  static_assert(sizeof...(Obs) == sizeof...(Vals));
  m_arguments = std::make_tuple(std::invoke(
      [](const cell<Vals> &args) -> variable<Obs> {
        return variable<Obs>(args);
      },
      args)...);
}

template <typename Ret>
template <typename... Obs>
template <unsigned int N>
constexpr auto ana::term<Ret>::representation_of<Obs...>::value() const
    -> decltype(std::get<N>(std::declval<vartuple_type>()).value()) {
  return std::get<N>(m_arguments).value();
}

template <typename Ret>
template <typename... Obs>
template <auto en>
constexpr auto ana::term<Ret>::representation_of<Obs...>::value() const {
  constexpr auto idx = static_cast<std::underlying_type_t<decltype(en)>>(en);
  return std::get<idx>(this->m_arguments).value();
}