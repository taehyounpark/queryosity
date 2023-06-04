#pragma once

#include "dataflow_delayed.h"
#include "dataflow_systematic.h"

namespace ana {

template <typename T>
template <typename Bld>
class dataflow<T>::delayed<Bld>::varied : public systematic<delayed<Bld>> {

public:
  varied(delayed<Bld> &&nom);
  ~varied() = default;

  varied(varied &&) = default;
  varied &operator=(varied &&) = default;

  virtual void set_variation(const std::string &var_name,
                             delayed &&var) override;

  virtual delayed const &get_nominal() const override;
  virtual delayed const &
  get_variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> list_variation_names() const override;

public:
  template <typename... Args, typename V = Bld,
            std::enable_if_t<ana::is_column_v<V>, bool> = false>
  auto vary(const std::string &var_name, Args &&...args) -> varied;

  template <
      typename... Args, typename V = Bld,
      std::enable_if_t<ana::column::template is_evaluator_v<V>, bool> = false>
  auto vary(const std::string &var_name, Args &&...args) -> varied;

  template <
      typename... Args, typename V = Bld,
      std::enable_if_t<ana::column::template is_evaluator_v<V>, bool> = false>
  auto evaluate(Args &&...args) -> typename ana::dataflow<T>::template lazy<
      column::template evaluated_t<V>>::varied;

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<ana::selection::template is_applicator_v<V>,
                             bool> = false>
  auto apply(Nodes const &...columns) -> typename lazy<selection>::varied;

  template <
      typename... Nodes, typename V = Bld,
      std::enable_if_t<ana::counter::template is_booker_v<V>, bool> = false>
  auto fill(Nodes const &...columns) -> varied;

  template <
      typename Node, typename V = Bld,
      std::enable_if_t<ana::counter::template is_booker_v<V>, bool> = false>
  auto at(Node const &selection) -> typename lazy<counter::booked_t<V>>::varied;

  template <
      typename... Nodes, typename V = Bld,
      std::enable_if_t<ana::counter::template is_booker_v<V>, bool> = false>
  auto at(Nodes const &...selections) ->
      typename delayed<counter::bookkeeper<counter::booked_t<V>>>::varied;

  template <typename... Args>
  auto operator()(Args &&...args) ->
      typename lazy<typename decltype(std::declval<delayed<Bld>>().operator()(
          std::forward<Args>(args).get_nominal()...))::action_type>::varied;

  template <
      typename V = Bld,
      std::enable_if_t<ana::counter::template is_bookkeeper_v<V>, bool> = false>
  auto operator[](const std::string &var_name) const -> delayed<V> const &;

protected:
  delayed<Bld> m_nom;
  std::unordered_map<std::string, delayed<Bld>> m_var_map;
  std::set<std::string> m_var_names;
};

} // namespace ana

#include "dataflow.h"
#include "dataflow_lazy.h"
#include "selection.h"

template <typename T>
template <typename Bld>
ana::dataflow<T>::delayed<Bld>::varied::varied(delayed<Bld> &&nom)
    : systematic<delayed<Bld>>::systematic(*nom.m_df), m_nom(std::move(nom)) {}

template <typename T>
template <typename Bld>
void ana::dataflow<T>::delayed<Bld>::varied::set_variation(
    const std::string &var_name, delayed &&var) {
  m_var_map.insert(std::move(std::make_pair(var_name, std::move(var))));
  m_var_names.insert(var_name);
}

template <typename T>
template <typename Bld>
auto ana::dataflow<T>::delayed<Bld>::varied::get_nominal() const
    -> delayed const & {
  return m_nom;
}

template <typename T>
template <typename Bld>
auto ana::dataflow<T>::delayed<Bld>::varied::get_variation(
    const std::string &var_name) const -> delayed const & {
  return (this->has_variation(var_name) ? m_var_map.at(var_name) : m_nom);
}

template <typename T>
template <typename Bld>
bool ana::dataflow<T>::delayed<Bld>::varied::has_variation(
    const std::string &var_name) const {
  return m_var_map.find(var_name) != m_var_map.end();
}

template <typename T>
template <typename Bld>
std::set<std::string>
ana::dataflow<T>::delayed<Bld>::varied::list_variation_names() const {
  return m_var_names;
}

template <typename T>
template <typename Bld>
template <typename V,
          std::enable_if_t<ana::counter::template is_bookkeeper_v<V>, bool>>
auto ana::dataflow<T>::delayed<Bld>::varied::operator[](
    const std::string &var_name) const -> delayed<V> const & {
  if (!this->has_variation(var_name)) {
    throw std::out_of_range("variation does not exist");
  }
  return this->get_variation(var_name);
}

template <typename T>
template <typename Bld>
template <typename... Args, typename V,
          std::enable_if_t<ana::column::template is_evaluator_v<V>, bool>>
auto ana::dataflow<T>::delayed<Bld>::varied::evaluate(Args &&...args) ->
    typename ana::dataflow<T>::template lazy<
        column::template evaluated_t<V>>::varied {
  using syst_type = typename ana::dataflow<T>::template lazy<
      column::template evaluated_t<V>>::varied;
  auto syst = syst_type(
      this->get_nominal().evaluate(std::forward<Args>(args).get_nominal()...));
  for (auto const &var_name :
       list_all_variation_names(*this, std::forward<Args>(args)...)) {
    syst.set_variation(
        var_name, get_variation(var_name).evaluate(
                      std::forward<Args>(args).get_variation(var_name)...));
  }
  return syst;
}

template <typename T>
template <typename Bld>
template <typename... Nodes, typename V,
          std::enable_if_t<ana::selection::template is_applicator_v<V>, bool>>
auto ana::dataflow<T>::delayed<Bld>::varied::apply(Nodes const &...columns) ->
    typename lazy<selection>::varied {

  using syst_type = typename lazy<selection>::varied;
  auto syst = syst_type(this->get_nominal().apply(columns.get_nominal()...));

  for (auto const &var_name : list_all_variation_names(*this, columns...)) {
    syst.set_variation(var_name, variation(var_name).apply(
                                     columns.get_variation(var_name)...));
  }

  return syst;
}

template <typename T>
template <typename Bld>
template <typename... Nodes, typename V,
          std::enable_if_t<ana::counter::template is_booker_v<V>, bool>>
auto ana::dataflow<T>::delayed<Bld>::varied::fill(Nodes const &...columns)
    -> varied {
  auto syst =
      varied(std::move(this->get_nominal().fill(columns.get_nominal()...)));
  for (auto const &var_name : list_all_variation_names(*this, columns...)) {
    syst.set_variation(var_name, std::move(get_variation(var_name).fill(
                                     columns.get_variation(var_name)...)));
  }
  return syst;
}

template <typename T>
template <typename Bld>
template <typename Node, typename V,
          std::enable_if_t<ana::counter::template is_booker_v<V>, bool>>
auto ana::dataflow<T>::delayed<Bld>::varied::at(Node const &selection) ->
    typename lazy<counter::booked_t<V>>::varied
// varied version of booking counter at a selection operation
{
  using syst_type = typename lazy<counter::booked_t<V>>::varied;
  auto syst = syst_type(this->get_nominal().at(selection.get_nominal()));
  for (auto const &var_name : list_all_variation_names(*this, selection)) {
    syst.set_variation(var_name, get_variation(var_name).at(
                                     selection.get_variation(var_name)));
  }
  return syst;
}

template <typename T>
template <typename Bld>
template <typename... Nodes, typename V,
          std::enable_if_t<ana::counter::template is_booker_v<V>, bool>>
auto ana::dataflow<T>::delayed<Bld>::varied::at(Nodes const &...selections) ->
    typename delayed<counter::bookkeeper<counter::booked_t<V>>>::varied
// varied version of booking counter at a selection operation
{
  using syst_type =
      typename delayed<counter::bookkeeper<counter::booked_t<V>>>::varied;
  auto syst = syst_type(this->get_nominal().at(selections.get_nominal()...));
  for (auto const &var_name : list_all_variation_names(*this, selections...)) {
    syst.set_variation(var_name, get_variation(var_name).at(
                                     selections.get_variation(var_name)...));
  }
  return syst;
}

template <typename T>
template <typename Bld>
template <typename... Args, typename V,
          std::enable_if_t<ana::column::template is_evaluator_v<V>, bool>>
auto ana::dataflow<T>::delayed<Bld>::varied::vary(const std::string &var_name,
                                                  Args &&...args) -> varied {
  auto syst = varied(std::move(*this));
  syst.set_variation(var_name,
                     std::move(syst.m_df->vary_equation(
                         syst.get_nominal(), std::forward<Args>(args)...)));
  return std::move(syst);
}

template <typename T>
template <typename Bld>
template <typename... Args>
auto ana::dataflow<T>::delayed<Bld>::varied::operator()(Args &&...args) ->
    typename lazy<typename decltype(std::declval<delayed<Bld>>().operator()(
        std::forward<Args>(args).get_nominal()...))::action_type>::varied {

  using syst_type =
      typename lazy<typename decltype(std::declval<delayed<Bld>>().operator()(
          std::forward<Args>(args).get_nominal()...))::action_type>::varied;

  auto syst = syst_type(this->get_nominal().operator()(
      std::forward<Args>(args).get_nominal()...));
  for (auto const &var_name :
       list_all_variation_names(*this, std::forward<Args>(args)...)) {
    syst.set_variation(
        var_name, get_variation(var_name).operator()(
                      std::forward<Args>(args).get_variation(var_name)...));
  }
  return std::move(syst);
}
