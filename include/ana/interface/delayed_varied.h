#pragma once

#include "delayed.h"

namespace ana {

/**
 * @brief Varied version of a delayed node.
 * @details A delayed varied operation is functionally equivalent to a delayed
 * node, except that it contains multiple nodes corresponding to nominal and
 * systematic variations.
 */
template <typename Bld>
class delayed<Bld>::varied : public systematic::resolver<delayed<Bld>> {

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
  auto evaluate(Args &&...args) ->
      typename ana::lazy<column::template evaluated_t<V>>::varied;

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<ana::selection::template is_applicator_v<V>,
                             bool> = false>
  auto apply(Nodes const &...columns) -> typename lazy<selection>::varied;

  /**
   * @brief Fill aggregation logic with input columns.
   */
  template <
      typename... Nodes, typename V = Bld,
      std::enable_if_t<ana::aggregation::template is_booker_v<V>, bool> = false>
  auto fill(Nodes const &...columns) -> varied;

  /**
   * @brief Book the aggregation logic at a selection.
   * @param selection Lazy(varied) selection node to book aggregation
   * at.
   */
  template <
      typename Node, typename V = Bld,
      std::enable_if_t<ana::aggregation::template is_booker_v<V>, bool> = false>
  auto book(Node const &selection) ->
      typename lazy<aggregation::booked_t<V>>::varied;

  /**
   * @brief Book the aggregation logic at multiple selections.
   * @param selection Lazy selection nodes to book aggregations at.
   * @return Lazy aggregation node.
   */
  template <
      typename... Nodes, typename V = Bld,
      std::enable_if_t<ana::aggregation::template is_booker_v<V>, bool> = false>
  auto book(Nodes const &...selections) -> typename delayed<
      aggregation::bookkeeper<aggregation::booked_t<V>>>::varied;

  /**
   * @brief Evaluate/apply the column definition/selection with input columns.
   * @param args... Lazy input column nodes
   * @return Lazy column definition node
   */
  template <typename... Args>
  auto operator()(Args &&...args) ->
      typename lazy<typename decltype(std::declval<delayed<Bld>>().operator()(
          std::forward<Args>(args).nominal()...))::operation_type>::varied;

  /**
   * @brief Access the delayed node of a specific systematic variation.
   * @param var_name Name of the systematic variation.
   * @return Delayed node corresponding to systematic variation.
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
#include "lazy.h"
#include "selection.h"

template <typename Bld>
ana::delayed<Bld>::varied::varied(delayed<Bld> &&nom)
    : systematic::resolver<delayed<Bld>>::resolver(*nom.m_df),
      m_nominal(std::move(nom)) {}

template <typename Bld>
void ana::delayed<Bld>::varied::set_variation(const std::string &var_name,
                                              delayed &&var) {
  m_variation_map.insert(std::move(std::make_pair(var_name, std::move(var))));
  m_variation_names.insert(var_name);
}

template <typename Bld>
auto ana::delayed<Bld>::varied::nominal() const -> delayed const & {
  return m_nominal;
}

template <typename Bld>
auto ana::delayed<Bld>::varied::variation(const std::string &var_name) const
    -> delayed const & {
  return (this->has_variation(var_name) ? m_variation_map.at(var_name)
                                        : m_nominal);
}

template <typename Bld>
bool ana::delayed<Bld>::varied::has_variation(
    const std::string &var_name) const {
  return m_variation_map.find(var_name) != m_variation_map.end();
}

template <typename Bld>
std::set<std::string> ana::delayed<Bld>::varied::list_variation_names() const {
  return m_variation_names;
}

template <typename Bld>
template <typename V,
          std::enable_if_t<ana::aggregation::template is_bookkeeper_v<V>, bool>>
auto ana::delayed<Bld>::varied::operator[](const std::string &var_name) const
    -> delayed<V> const & {
  if (!this->has_variation(var_name)) {
    throw std::out_of_range("variation does not exist");
  }
  return this->variation(var_name);
}

template <typename Bld>
template <typename... Args, typename V,
          std::enable_if_t<ana::column::template is_evaluator_v<V>, bool>>
auto ana::delayed<Bld>::varied::evaluate(Args &&...args) ->
    typename ana::lazy<column::template evaluated_t<V>>::varied {
  using varied_type =
      typename ana::lazy<column::template evaluated_t<V>>::varied;
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
auto ana::delayed<Bld>::varied::apply(Nodes const &...columns) ->
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
auto ana::delayed<Bld>::varied::fill(Nodes const &...columns) -> varied {
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
auto ana::delayed<Bld>::varied::book(Node const &selection) ->
    typename lazy<aggregation::booked_t<V>>::varied {
  using varied_type = typename lazy<aggregation::booked_t<V>>::varied;
  auto syst = varied_type(this->nominal().book(selection.nominal()));
  for (auto const &var_name : list_all_variation_names(*this, selection)) {
    syst.set_variation(var_name,
                       variation(var_name).book(selection.variation(var_name)));
  }
  return syst;
}

template <typename Bld>
template <typename... Nodes, typename V,
          std::enable_if_t<ana::aggregation::template is_booker_v<V>, bool>>
auto ana::delayed<Bld>::varied::book(Nodes const &...selections) ->
    typename delayed<
        aggregation::bookkeeper<aggregation::booked_t<V>>>::varied {
  using varied_type = typename delayed<
      aggregation::bookkeeper<aggregation::booked_t<V>>>::varied;
  auto syst = varied_type(this->nominal().book(selections.nominal()...));
  for (auto const &var_name : list_all_variation_names(*this, selections...)) {
    syst.set_variation(
        var_name, variation(var_name).book(selections.variation(var_name)...));
  }
  return syst;
}

template <typename Bld>
template <typename... Args>
auto ana::delayed<Bld>::varied::operator()(Args &&...args) ->
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
