#pragma once

#include "todo.h"

namespace queryosity {

/**
 * @ingroup api
 * @brief Varied version of a todo item.
 * @details A todo varied item is functionally equivalent to a todo
 * node with each method being propagated to independent todo nodes
 * corresponding to nominal and systematic variations.
 */
template <typename Bld>
class todo<Bld>::varied : public dataflow::node,
                          systematic::resolver<todo<Bld>> {

public:
  varied(todo<Bld> &&nom);
  ~varied() = default;

  varied(varied &&) = default;
  varied &operator=(varied &&) = default;

  virtual void set_variation(const std::string &var_name, todo var) override;

  virtual todo &nominal() override;
  virtual todo &variation(const std::string &var_name) override;
  virtual todo const &nominal() const override;
  virtual todo const &variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> get_variation_names() const override;

public:
  template <typename... Args, typename V = Bld,
            std::enable_if_t<queryosity::column::template is_evaluatable_v<V>,
                             bool> = false>
  auto evaluate(Args &&...args) ->
      typename queryosity::lazy<column::template evaluated_t<V>>::varied;

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<queryosity::selection::template is_applicable_v<V>,
                             bool> = false>
  auto apply(Nodes const &...columns) -> typename lazy<selection::node>::varied;

  /**
   * @brief Fill the query with input columns.
   * @param columns... Input columns to fill the query with.
   * @return A new todo query node with input columns filled.
   */
  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<queryosity::query::template is_bookable_v<V>,
                             bool> = false>
  auto fill(Nodes const &...columns) -> varied;

  /**
   * @brief Book the query logic at a selection.
   * @param selection Lazy selection to book query at.
   * @return Lazy query booked at selection.
   */
  template <typename Node, typename V = Bld,
            std::enable_if_t<queryosity::query::template is_bookable_v<V>,
                             bool> = false>
  auto book(Node const &selection) -> typename lazy<query::booked_t<V>>::varied;

  /**
   * @brief Book the query logic at multiple selections.
   * @param selection Lazy selection to book querys at.
   * @return Delayed query containing booked lazy querys.
   */
  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<queryosity::query::template is_bookable_v<V>,
                             bool> = false>
  auto book(Nodes const &...selections)
      -> std::array<typename lazy<query::booked_t<V>>::varied,
                    sizeof...(Nodes)>;

  /**
   * @brief Evaluate/apply the column definition/selection with input columns.
   * @param args... Lazy input columns
   * @return Lazy column definition
   */
  template <typename... Args>
  auto operator()(Args &&...args) ->
      typename lazy<typename decltype(std::declval<todo<Bld>>().operator()(
          std::forward<Args>(args).nominal()...))::action_type>::varied;

protected:
  todo<Bld> m_nominal;
  std::unordered_map<std::string, todo<Bld>> m_variation_map;
  std::set<std::string> m_variation_names;
};

} // namespace queryosity

#include "dataflow.h"
#include "lazy.h"
#include "selection.h"

template <typename Bld>
queryosity::todo<Bld>::varied::varied(todo<Bld> &&nom)
    : dataflow::node(*nom.m_df), m_nominal(std::move(nom)) {}

template <typename Bld>
void queryosity::todo<Bld>::varied::set_variation(const std::string &var_name,
                                                  todo var) {
  m_variation_map.insert(std::move(std::make_pair(var_name, std::move(var))));
  m_variation_names.insert(var_name);
}

template <typename Bld>
auto queryosity::todo<Bld>::varied::nominal() -> todo & {
  return m_nominal;
}

template <typename Bld>
auto queryosity::todo<Bld>::varied::nominal() const -> todo const & {
  return m_nominal;
}

template <typename Bld>
auto queryosity::todo<Bld>::varied::variation(const std::string &var_name)
    -> todo & {
  return (this->has_variation(var_name) ? m_variation_map.at(var_name)
                                        : m_nominal);
}

template <typename Bld>
auto queryosity::todo<Bld>::varied::variation(const std::string &var_name) const
    -> todo const & {
  return (this->has_variation(var_name) ? m_variation_map.at(var_name)
                                        : m_nominal);
}

template <typename Bld>
bool queryosity::todo<Bld>::varied::has_variation(
    const std::string &var_name) const {
  return m_variation_map.find(var_name) != m_variation_map.end();
}

template <typename Bld>
std::set<std::string>
queryosity::todo<Bld>::varied::get_variation_names() const {
  return m_variation_names;
}

template <typename Bld>
template <
    typename... Args, typename V,
    std::enable_if_t<queryosity::column::template is_evaluatable_v<V>, bool>>
auto queryosity::todo<Bld>::varied::evaluate(Args &&...args) ->
    typename queryosity::lazy<column::template evaluated_t<V>>::varied {
  using varied_type =
      typename queryosity::lazy<column::template evaluated_t<V>>::varied;
  auto syst = varied_type(
      this->nominal().evaluate(std::forward<Args>(args).nominal()...));
  for (auto const &var_name :
       systematic::get_variation_names(*this, std::forward<Args>(args)...)) {
    syst.set_variation(var_name,
                       variation(var_name).evaluate(
                           std::forward<Args>(args).variation(var_name)...));
  }
  return syst;
}

template <typename Bld>
template <
    typename... Nodes, typename V,
    std::enable_if_t<queryosity::selection::template is_applicable_v<V>, bool>>
auto queryosity::todo<Bld>::varied::apply(Nodes const &...columns) ->
    typename lazy<selection::node>::varied {

  using varied_type = typename lazy<selection::node>::varied;
  auto syst = varied_type(this->nominal().apply(columns.nominal()...));

  for (auto const &var_name :
       systematic::get_variation_names(*this, columns...)) {
    syst.set_variation(
        var_name, variation(var_name).apply(columns.variation(var_name)...));
  }

  return syst;
}

template <typename Bld>
template <typename... Nodes, typename V,
          std::enable_if_t<queryosity::query::template is_bookable_v<V>, bool>>
auto queryosity::todo<Bld>::varied::fill(Nodes const &...columns) -> varied {
  auto syst = varied(std::move(this->nominal().fill(columns.nominal()...)));
  for (auto const &var_name :
       systematic::get_variation_names(*this, columns...)) {
    syst.set_variation(var_name, std::move(variation(var_name).fill(
                                     columns.variation(var_name)...)));
  }
  return syst;
}

template <typename Bld>
template <typename Node, typename V,
          std::enable_if_t<queryosity::query::template is_bookable_v<V>, bool>>
auto queryosity::todo<Bld>::varied::book(Node const &selection) ->
    typename lazy<query::booked_t<V>>::varied {
  using varied_type = typename lazy<query::booked_t<V>>::varied;
  auto syst = varied_type(this->nominal().book(selection.nominal()));
  for (auto const &var_name :
       systematic::get_variation_names(*this, selection)) {
    syst.set_variation(var_name, this->variation(var_name).book(
                                     selection.variation(var_name)));
  }
  return syst;
}

template <typename Bld>
template <typename... Nodes, typename V,
          std::enable_if_t<queryosity::query::template is_bookable_v<V>, bool>>
auto queryosity::todo<Bld>::varied::book(Nodes const &...selections)
    -> std::array<typename lazy<query::booked_t<V>>::varied, sizeof...(Nodes)> {
  // variations
  using varied_type = typename lazy<query::booked_t<V>>::varied;
  using array_of_varied_type =
      std::array<typename lazy<query::booked_t<V>>::varied, sizeof...(Nodes)>;
  auto var_names = systematic::get_variation_names(*this, selections...);
  auto _book_varied =
      [var_names,
       this](systematic::resolver<lazy<selection::node>> const &sel) {
        auto syst =
            varied_type(this->m_df->_book(this->nominal(), sel.nominal()));
        for (auto const &var_name : var_names) {
          syst.set_variation(var_name,
                             this->m_df->_book(this->variation(var_name),
                                               sel.variation(var_name)));
        }
        return syst;
      };
  return array_of_varied_type{_book_varied(selections)...};
}

template <typename Bld>
template <typename... Args>
auto queryosity::todo<Bld>::varied::operator()(Args &&...args) ->
    typename lazy<typename decltype(std::declval<todo<Bld>>().operator()(
        std::forward<Args>(args).nominal()...))::action_type>::varied {

  using varied_type =
      typename lazy<typename decltype(std::declval<todo<Bld>>().operator()(
          std::forward<Args>(args).nominal()...))::action_type>::varied;

  auto syst = varied_type(
      this->nominal().operator()(std::forward<Args>(args).nominal()...));
  for (auto const &var_name :
       systematic::get_variation_names(*this, std::forward<Args>(args)...)) {
    syst.set_variation(var_name,
                       variation(var_name).operator()(
                           std::forward<Args>(args).variation(var_name)...));
  }
  return syst;
}
