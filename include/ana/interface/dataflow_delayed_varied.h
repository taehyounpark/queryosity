#pragma once

#include "dataflow_delayed.h"
#include "dataflow_systematic.h"

namespace ana {

/**
 * @brief Varied version of a delayed operation.
 * @details A delayed varied operation can be considered to be functionally
 * equivalent to a delayed operation, except that it contains multipled delayed
 * ones for which the operation is applied. The nominal delayed operation can be
 * accessed by `nominal()`, and a systematic variation by `["variation name"]`.
 */
template <typename Bld>
class dataflow::delayed<Bld>::varied : public systematic<delayed<Bld>> {

public:
  varied(delayed<Bld> &&nom);
  ~varied() = default;

  varied(varied &&) = default;
  varied &operator=(varied &&) = default;

  virtual void set_variation(const std::string &var_name,
                             delayed &&var) override;

  virtual delayed const &nominal() const override;
  virtual delayed const &variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> list_variation_names() const override;

public:
  template <
      typename... Args, typename V = Bld,
      std::enable_if_t<ana::column::template is_evaluator_v<V>, bool> = false>
  auto vary(const std::string &var_name, Args &&...args) -> varied;

  template <
      typename... Args, typename V = Bld,
      std::enable_if_t<ana::column::template is_evaluator_v<V>, bool> = false>
  auto evaluate(Args &&...args) -> typename ana::dataflow::template lazy<
      column::template evaluated_t<V>>::varied;

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<ana::selection::template is_applicator_v<V>,
                             bool> = false>
  auto apply(Nodes const &...columns) -> typename lazy<selection>::varied;

  template <
      typename... Nodes, typename V = Bld,
      std::enable_if_t<ana::aggregation::template is_booker_v<V>, bool> = false>
  auto fill(Nodes const &...columns) -> varied;

  template <
      typename Node, typename V = Bld,
      std::enable_if_t<ana::aggregation::template is_booker_v<V>, bool> = false>
  auto at(Node const &selection) ->
      typename lazy<aggregation::booked_t<V>>::varied;

  template <
      typename... Nodes, typename V = Bld,
      std::enable_if_t<ana::aggregation::template is_booker_v<V>, bool> = false>
  auto at(Nodes const &...selections) -> typename delayed<
      aggregation::bookkeeper<aggregation::booked_t<V>>>::varied;

  template <typename... Args>
  auto operator()(Args &&...args) ->
      typename lazy<typename decltype(std::declval<delayed<Bld>>().operator()(
          std::forward<Args>(args).nominal()...))::operation_type>::varied;

  /**
   * @brief Access the delayed operation of a specific systematic variation.
   * @param var_name The name of the systematic variation.
   */
  template <typename V = Bld,
            std::enable_if_t<ana::aggregation::template is_bookkeeper_v<V>,
                             bool> = false>
  auto operator[](const std::string &var_name) const -> delayed<V> const &;

protected:
  delayed<Bld> m_nominal;
  std::unordered_map<std::string, delayed<Bld>> m_variation_map;
  std::set<std::string> m_variation_names;
};

} // namespace ana

#include "dataflow.h"
#include "dataflow_lazy.h"
#include "selection.h"

template <typename Bld>
ana::dataflow::delayed<Bld>::varied::varied(delayed<Bld> &&nom)
    : systematic<delayed<Bld>>::systematic(*nom.m_df),
      m_nominal(std::move(nom)) {}

template <typename Bld>
void ana::dataflow::delayed<Bld>::varied::set_variation(
    const std::string &var_name, delayed &&var) {
  m_variation_map.insert(std::move(std::make_pair(var_name, std::move(var))));
  m_variation_names.insert(var_name);
}

template <typename Bld>
auto ana::dataflow::delayed<Bld>::varied::nominal() const -> delayed const & {
  return m_nominal;
}

template <typename Bld>
auto ana::dataflow::delayed<Bld>::varied::variation(
    const std::string &var_name) const -> delayed const & {
  return (this->has_variation(var_name) ? m_variation_map.at(var_name)
                                        : m_nominal);
}

template <typename Bld>
bool ana::dataflow::delayed<Bld>::varied::has_variation(
    const std::string &var_name) const {
  return m_variation_map.find(var_name) != m_variation_map.end();
}

template <typename Bld>
std::set<std::string>
ana::dataflow::delayed<Bld>::varied::list_variation_names() const {
  return m_variation_names;
}

template <typename Bld>
template <typename V,
          std::enable_if_t<ana::aggregation::template is_bookkeeper_v<V>, bool>>
auto ana::dataflow::delayed<Bld>::varied::operator[](
    const std::string &var_name) const -> delayed<V> const & {
  if (!this->has_variation(var_name)) {
    throw std::out_of_range("variation does not exist");
  }
  return this->variation(var_name);
}

template <typename Bld>
template <typename... Args, typename V,
          std::enable_if_t<ana::column::template is_evaluator_v<V>, bool>>
auto ana::dataflow::delayed<Bld>::varied::evaluate(Args &&...args) ->
    typename ana::dataflow::template lazy<
        column::template evaluated_t<V>>::varied {
  using varied_type = typename ana::dataflow::template lazy<
      column::template evaluated_t<V>>::varied;
  auto syst = varied_type(
      this->nominal().evaluate(std::forward<Args>(args).nominal()...));
  for (auto const &var_name :
       list_all_variation_names(*this, std::forward<Args>(args)...)) {
    syst.set_variation(var_name,
                       variation(var_name).evaluate(
                           std::forward<Args>(args).variation(var_name)...));
  }
  return syst;
}

template <typename Bld>
template <typename... Nodes, typename V,
          std::enable_if_t<ana::selection::template is_applicator_v<V>, bool>>
auto ana::dataflow::delayed<Bld>::varied::apply(Nodes const &...columns) ->
    typename lazy<selection>::varied {

  using varied_type = typename lazy<selection>::varied;
  auto syst = varied_type(this->nominal().apply(columns.nominal()...));

  for (auto const &var_name : list_all_variation_names(*this, columns...)) {
    syst.set_variation(
        var_name, variation(var_name).apply(columns.variation(var_name)...));
  }

  return syst;
}

template <typename Bld>
template <typename... Nodes, typename V,
          std::enable_if_t<ana::aggregation::template is_booker_v<V>, bool>>
auto ana::dataflow::delayed<Bld>::varied::fill(Nodes const &...columns)
    -> varied {
  auto syst = varied(std::move(this->nominal().fill(columns.nominal()...)));
  for (auto const &var_name : list_all_variation_names(*this, columns...)) {
    syst.set_variation(var_name, std::move(variation(var_name).fill(
                                     columns.variation(var_name)...)));
  }
  return syst;
}

template <typename Bld>
template <typename Node, typename V,
          std::enable_if_t<ana::aggregation::template is_booker_v<V>, bool>>
auto ana::dataflow::delayed<Bld>::varied::at(Node const &selection) ->
    typename lazy<aggregation::booked_t<V>>::varied {
  using varied_type = typename lazy<aggregation::booked_t<V>>::varied;
  auto syst = varied_type(this->nominal().at(selection.nominal()));
  for (auto const &var_name : list_all_variation_names(*this, selection)) {
    syst.set_variation(var_name,
                       variation(var_name).at(selection.variation(var_name)));
  }
  return syst;
}

template <typename Bld>
template <typename... Nodes, typename V,
          std::enable_if_t<ana::aggregation::template is_booker_v<V>, bool>>
auto ana::dataflow::delayed<Bld>::varied::at(Nodes const &...selections) ->
    typename delayed<
        aggregation::bookkeeper<aggregation::booked_t<V>>>::varied {
  using varied_type = typename delayed<
      aggregation::bookkeeper<aggregation::booked_t<V>>>::varied;
  auto syst = varied_type(this->nominal().at(selections.nominal()...));
  for (auto const &var_name : list_all_variation_names(*this, selections...)) {
    syst.set_variation(
        var_name, variation(var_name).at(selections.variation(var_name)...));
  }
  return syst;
}

template <typename Bld>
template <typename... Args, typename V,
          std::enable_if_t<ana::column::template is_evaluator_v<V>, bool>>
auto ana::dataflow::delayed<Bld>::varied::vary(const std::string &var_name,
                                               Args &&...args) -> varied {
  auto syst = varied(std::move(*this));
  syst.set_variation(var_name,
                     std::move(syst.m_df->vary_evaluator(
                         syst.nominal(), std::forward<Args>(args)...)));
  return syst;
}

template <typename Bld>
template <typename... Args>
auto ana::dataflow::delayed<Bld>::varied::operator()(Args &&...args) ->
    typename lazy<typename decltype(std::declval<delayed<Bld>>().operator()(
        std::forward<Args>(args).nominal()...))::operation_type>::varied {

  using varied_type =
      typename lazy<typename decltype(std::declval<delayed<Bld>>().operator()(
          std::forward<Args>(args).nominal()...))::operation_type>::varied;

  auto syst = varied_type(
      this->nominal().operator()(std::forward<Args>(args).nominal()...));
  for (auto const &var_name :
       list_all_variation_names(*this, std::forward<Args>(args)...)) {
    syst.set_variation(var_name,
                       variation(var_name).operator()(
                           std::forward<Args>(args).variation(var_name)...));
  }
  return syst;
}
