#pragma once

#include "delayed.h"

namespace ana {

template <typename T>
template <typename Bld>
class dataflow<T>::delayed<Bld>::varied : public node<delayed<Bld>> {

public:
  using dataflow_type = typename node<delayed<Bld>>::dataflow_type;
  using dataset_type = typename node<delayed<Bld>>::dataset_type;
  using nominal_type = typename node<delayed<Bld>>::nominal_type;

public:
  varied(dataflow<T> &df, delayed<Bld> nom);
  ~varied() = default;

  virtual void set_variation(const std::string &var_name,
                             const delayed &var) override;

  virtual delayed get_nominal() const override;
  virtual delayed get_variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> list_variation_names() const override;

public:
  template <
      typename... Args, typename V = Bld,
      std::enable_if_t<ana::column::template is_evaluator_v<V>, bool> = false>
  auto evaluate(Args &&...args) -> typename ana::dataflow<T>::template lazy<
      column::template evaluated_t<V>>::varied;

  template <
      typename... Nodes, typename V = Bld,
      std::enable_if_t<ana::counter::template is_booker_v<V>, bool> = false>
  auto ana::dataflow<T>::delayed<Bld>::varied::fill(Nodes const &...columns) ->
      typename ana::dataflow<T>::template delayed<booked_counter_t<V>>::varied;

protected:
  delayed<Bld> m_nom;
  std::unordered_map<std::string, delayed<Bld>> m_var_lookup;
  std::set<std::string> m_var_names;
};

} // namespace ana

#include "dataflow.h"
#include "lazy.h"
#include "selection.h"

template <typename T>
template <typename Bld>
void ana::dataflow<T>::delayed<Bld>::varied::set_variation(
    const std::string &var_name, delayed const &var) {
  m_var_lookup.insert(std::make_pair(var_name, var));
  m_var_names.insert(var_name);
}

template <typename T>
template <typename Bld>
auto ana::dataflow<T>::delayed<Bld>::varied::get_nominal() const -> delayed {
  return m_nom;
}

template <typename T>
template <typename Bld>
auto ana::dataflow<T>::delayed<Bld>::varied::get_variation(
    const std::string &var_name) const -> delayed {
  return (this->has_variation(var_name) ? m_var_lookup.at(var_name) : m_nom);
}

template <typename T>
template <typename Bld>
bool ana::dataflow<T>::delayed<Bld>::varied::has_variation(
    const std::string &var_name) const {
  return m_var_lookup.find(var_name) != m_var_lookup.end();
}

template <typename T>
template <typename Bld>
std::set<std::string>
ana::dataflow<T>::delayed<Bld>::varied::list_variation_names() const {
  return m_var_names;
}

template <typename T>
template <typename Bld>
template <typename V, typename std::enable_if<
                          ana::counter::template is_booker_v<V> ||
                              ana::counter::template is_implemented_v<V>,
                          void>::type *ptr>
auto ana::dataflow<T>::delayed<Bld>::varied::operator[](
    const std::string &var_name) const -> lazy<V> {
  if (!this->has_variation(var_name)) {
    throw std::out_of_range("variation does not exist");
  }
  return m_var_lookup.at(var_name);
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
    typename ana::dataflow<T>::template lazy<selection>::varied {
  using syst_type = typename ana::dataflow<T>::template lazy<selection>::varied;
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
auto ana::dataflow<T>::delayed<Bld>::varied::fill(Nodes const &...columns) ->
    typename ana::dataflow<T>::template delayed<booked_counter_t<V>>::varied
// varied version of filling a counter with columns
{
  using syst_type =
      typename ana::dataflow<T>::template delayed<booked_counter_t<V>>::varied;
  auto syst = syst_type(this->get_nominal().fill(columns.get_nominal()...));
  for (auto const &var_name : list_all_variation_names(*this, columns...)) {
    syst.set_variation(var_name, get_variation(var_name).fill(
                                     columns.get_variation(var_name)...));
  }
  return syst;
}

template <typename T>
template <typename Bld>
template <typename... Nodes, typename V,
          std::enable_if_t<ana::counter::template is_booker_v<V>, bool>>
auto ana::dataflow<T>::delayed<Bld>::varied::at(Nodes const &...selections)
    -> varied<typename decltype(std::declval<lazy<V>>().at(
        selections.get_nominal()...))::nominal_type>
// varied version of booking counter at a selection operation
{
  varied<typename decltype(std::declval<lazy<V>>().at(
      selections.get_nominal()...))::nominal_type>
      syst(this->get_nominal().at(selections.get_nominal()...));
  for (auto const &var_name : list_all_variation_names(*this, selections...)) {
    syst.set_variation(var_name, get_variation(var_name).at(
                                     selections.get_variation(var_name)...));
  }
  return syst;
}

template <typename T>
template <typename Bld>
template <
    typename... Args, typename V,
    std::enable_if_t<
        ana::is_column_v<V> || ana::column::template is_evaluator_v<V>, bool>>
auto ana::dataflow<T>::delayed<Bld>::varied::vary(const std::string &var_name,
                                                  Args &&...args) -> varied<V> {
  auto syst = varied<V>(this->get_nominal());
  for (auto const &var_name : this->list_variation_names()) {
    syst.set_variation(var_name, this->get_variation(var_name));
  }
  // set new variation
  syst.set_variation(var_name, this->get_nominal()
                                   .vary(var_name, std::forward<Args>(args)...)
                                   .get_variation(var_name));
  return syst;
}

template <typename T>
template <typename Bld>
template <typename... Args>
auto ana::dataflow<T>::delayed<Bld>::varied::operator()(Args &&...args)
    -> varied<typename decltype(std::declval<lazy<Bld>>().operator()(
        std::forward<Args>(args).get_nominal()...))::nominal_type> {
  auto syst = varied<typename decltype(std::declval<lazy<Bld>>().operator()(
      std::forward<Args>(args).get_nominal()...))::nominal_type>(
      this->get_nominal().operator()(
          std::forward<Args>(args).get_nominal()...));
  for (auto const &var_name :
       list_all_variation_names(*this, std::forward<Args>(args)...)) {
    syst.set_variation(
        var_name, get_variation(var_name).operator()(
                      std::forward<Args>(args).get_variation(var_name)...));
  }
  return syst;
}
