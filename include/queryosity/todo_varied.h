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
template <typename Helper>
class todo<Helper>::varied : public dataflow::node,
                             systematic::resolver<todo<Helper>> {

public:
  varied(todo<Helper> &&nom);
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
  template <
      typename... Cols, typename V = Helper,
      std::enable_if_t<queryosity::column::is_evaluatable_v<V>, bool> = false>
  auto evaluate(Cols &&...cols) -> typename decltype(this->nominal().evaluate(
      std::forward<Cols>(cols.nominal)...))::varied;

  template <
      typename... Cols, typename V = Helper,
      std::enable_if_t<queryosity::selection::is_applicable_v<V>, bool> = false>
  auto apply(Cols &&...cols) ->
      typename queryosity::lazy<selection::applied_t<V>>::varied;

  /**
   * @brief Fill the query with input columns.
   * @param[in] columns... Input columns to fill the query with.
   * @return A new todo query node with input columns filled.
   */
  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::query::is_bookable_v<V>, bool> = false>
  auto fill(Nodes const &...columns) -> varied;

  /**
   * @brief Book the query at a selection.
   * @param[in] selection Lazy selection to book query at.
   * @return Lazy query booked at selection.
   */
  template <typename Node, typename V = Helper,
            std::enable_if_t<queryosity::query::is_bookable_v<V>, bool> = false>
  auto at(Node const &selection) -> typename lazy<query::booked_t<V>>::varied;

  /**
   * @brief Book the query at multiple selections.
   * @param[in] selection Lazy selection to book queries at.
   * @return Delayed query containing booked lazy queries.
   */
  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::query::is_bookable_v<V>, bool> = false>
  auto at(Nodes const &...selections)
      -> std::array<typename lazy<query::booked_t<V>>::varied,
                    sizeof...(Nodes)>;

  /**
   * @brief Shortcut for `evaluate()`/`apply()`/`fill()` for
   * columns/selections/queries.
   * @tparam Cols (Varied) Input column types.
   * @param[in] cols... Input columns.
   * @return Lazy column definition
   */
  template <typename... Cols>
  auto operator()(Cols &&...cols) ->
      typename decltype(std::declval<todo<Helper>>().operator()(
          std::forward<Cols>(cols).nominal()...))::varied;

protected:
  todo<Helper> m_nominal;
  std::unordered_map<std::string, todo<Helper>> m_variation_map;
  std::set<std::string> m_variation_names;
};

} // namespace queryosity

#include "dataflow.h"
#include "lazy.h"
#include "selection.h"

template <typename Helper>
queryosity::todo<Helper>::varied::varied(todo<Helper> &&nom)
    : dataflow::node(*nom.m_df), m_nominal(std::move(nom)) {}

template <typename Helper>
void queryosity::todo<Helper>::varied::set_variation(
    const std::string &var_name, todo var) {
  m_variation_map.insert(std::move(std::make_pair(var_name, std::move(var))));
  m_variation_names.insert(var_name);
}

template <typename Helper>
auto queryosity::todo<Helper>::varied::nominal() -> todo & {
  return m_nominal;
}

template <typename Helper>
auto queryosity::todo<Helper>::varied::nominal() const -> todo const & {
  return m_nominal;
}

template <typename Helper>
auto queryosity::todo<Helper>::varied::variation(const std::string &var_name)
    -> todo & {
  return (this->has_variation(var_name) ? m_variation_map.at(var_name)
                                        : m_nominal);
}

template <typename Helper>
auto queryosity::todo<Helper>::varied::variation(
    const std::string &var_name) const -> todo const & {
  return (this->has_variation(var_name) ? m_variation_map.at(var_name)
                                        : m_nominal);
}

template <typename Helper>
bool queryosity::todo<Helper>::varied::has_variation(
    const std::string &var_name) const {
  return m_variation_map.find(var_name) != m_variation_map.end();
}

template <typename Helper>
std::set<std::string>
queryosity::todo<Helper>::varied::get_variation_names() const {
  return m_variation_names;
}

template <typename Helper>
template <typename... Cols, typename V,
          std::enable_if_t<queryosity::column::is_evaluatable_v<V>, bool>>
auto queryosity::todo<Helper>::varied::evaluate(Cols &&...cols) ->
    typename decltype(this->nominal().evaluate(
        std::forward<Cols>(cols.nominal)...))::varied {
  using varied_type = typename decltype(this->nominal().evaluate(
      std::forward<Cols>(cols.nominal)...))::varied;
  auto syst = varied_type(
      this->nominal().evaluate(std::forward<Cols>(cols).nominal()...));
  for (auto const &var_name :
       systematic::get_variation_names(*this, std::forward<Cols>(cols)...)) {
    syst.set_variation(var_name,
                       variation(var_name).evaluate(
                           std::forward<Cols>(cols).variation(var_name)...));
  }
  return syst;
}

template <typename Helper>
template <typename... Cols, typename V,
          std::enable_if_t<queryosity::selection::is_applicable_v<V>, bool>>
auto queryosity::todo<Helper>::varied::apply(Cols &&...cols) ->
    typename queryosity::lazy<selection::applied_t<V>>::varied {
  using varied_type =
      typename queryosity::lazy<selection::applied_t<V>>::varied;
  auto syst =
      varied_type(this->nominal().apply(std::forward<Cols>(cols).nominal()...));
  for (auto const &var_name :
       systematic::get_variation_names(*this, std::forward<Cols>(cols)...)) {
    syst.set_variation(var_name,
                       variation(var_name).apply(
                           std::forward<Cols>(cols).variation(var_name)...));
  }
  return syst;
}

template <typename Helper>
template <typename... Nodes, typename V,
          std::enable_if_t<queryosity::query::is_bookable_v<V>, bool>>
auto queryosity::todo<Helper>::varied::fill(Nodes const &...columns) -> varied {
  auto syst = varied(std::move(this->nominal().fill(columns.nominal()...)));
  for (auto const &var_name :
       systematic::get_variation_names(*this, columns...)) {
    syst.set_variation(var_name, std::move(variation(var_name).fill(
                                     columns.variation(var_name)...)));
  }
  return syst;
}

template <typename Helper>
template <typename Node, typename V,
          std::enable_if_t<queryosity::query::is_bookable_v<V>, bool>>
auto queryosity::todo<Helper>::varied::at(Node const &selection) ->
    typename lazy<query::booked_t<V>>::varied {
  using varied_type = typename lazy<query::booked_t<V>>::varied;
  auto syst = varied_type(this->nominal().at(selection.nominal()));
  for (auto const &var_name :
       systematic::get_variation_names(*this, selection)) {
    syst.set_variation(
        var_name, this->variation(var_name).at(selection.variation(var_name)));
  }
  return syst;
}

template <typename Helper>
template <typename... Nodes, typename V,
          std::enable_if_t<queryosity::query::is_bookable_v<V>, bool>>
auto queryosity::todo<Helper>::varied::at(Nodes const &...selections)
    -> std::array<typename lazy<query::booked_t<V>>::varied, sizeof...(Nodes)> {
  // variations
  using varied_type = typename lazy<query::booked_t<V>>::varied;
  using array_of_varied_type =
      std::array<typename lazy<query::booked_t<V>>::varied, sizeof...(Nodes)>;
  auto var_names = systematic::get_variation_names(*this, selections...);
  auto _book_varied =
      [var_names,
       this](systematic::resolver<lazy<selection::node>> const &sel) {
        auto syst = varied_type(this->nominal().at(sel.nominal()));
        for (auto const &var_name : var_names) {
          syst.set_variation(
              var_name, this->variation(var_name).at(sel.variation(var_name)));
        }
        return syst;
      };
  return array_of_varied_type{_book_varied(selections)...};
}

template <typename Helper>
template <typename... Cols>
auto queryosity::todo<Helper>::varied::operator()(Cols &&...cols) ->
    typename decltype(std::declval<todo<Helper>>().operator()(
        std::forward<Cols>(cols).nominal()...))::varied {

  using varied_type = typename decltype(std::declval<todo<Helper>>().operator()(
      std::forward<Cols>(cols).nominal()...))::varied;

  auto syst = varied_type(
      this->nominal().operator()(std::forward<Cols>(cols).nominal()...));
  for (auto const &var_name :
       systematic::get_variation_names(*this, std::forward<Cols>(cols)...)) {
    syst.set_variation(var_name,
                       variation(var_name).operator()(
                           std::forward<Cols>(cols).variation(var_name)...));
  }
  return syst;
}
