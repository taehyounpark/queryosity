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
class varied<todo<Helper>> : public dataflow::node,
                             systematic::resolver<todo<Helper>> {

public:
  varied(todo<Helper> &&nom);
  ~varied() = default;

  varied(varied &&) = default;
  varied &operator=(varied &&) = default;

  virtual void set_variation(const std::string &var_name,
                             todo<Helper> var) final override;

  virtual todo<Helper> &nominal() final override;
  virtual todo<Helper> &variation(const std::string &var_name) final override;
  virtual todo<Helper> const &nominal() const final override;
  virtual todo<Helper> const &
  variation(const std::string &var_name) const final override;

  virtual bool has_variation(const std::string &var_name) const final override;
  virtual std::set<std::string> get_variation_names() const final override;

public:
  template <
      typename... Cols, typename V = Helper,
      std::enable_if_t<queryosity::column::is_evaluatable_v<V>, bool> = false>
  auto evaluate(Cols &&...cols) -> varied<
      decltype(this->nominal().evaluate(std::forward<Cols>(cols.nominal)...))>;

  template <
      typename... Cols, typename V = Helper,
      std::enable_if_t<queryosity::selection::is_applicable_v<V>, bool> = false>
  auto apply(Cols &&...cols) -> varied<lazy<selection::applied_t<V>>>;

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
  auto at(Node const &selection) -> varied<lazy<query::booked_t<V>>>;

  /**
   * @brief Book the query at multiple selections.
   * @param[in] selection Lazy selection to book queries at.
   * @return Delayed query containing booked lazy queries.
   */
  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::query::is_bookable_v<V>, bool> = false>
  auto at(Nodes const &...selections)
      -> std::array<varied<lazy<query::booked_t<V>>>, sizeof...(Nodes)>;

  template <typename... Cols>
  auto operator()(Cols &&...cols)
      -> varied<decltype(std::declval<todo<Helper>>().operator()(
          std::forward<Cols>(cols).nominal()...))>;

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
queryosity::varied<queryosity::todo<Helper>>::varied(
    queryosity::todo<Helper> &&nom)
    : dataflow::node(*nom.m_df), m_nominal(std::move(nom)) {}

template <typename Helper>
void queryosity::varied<queryosity::todo<Helper>>::set_variation(
    const std::string &var_name, queryosity::todo<Helper> var) {
  m_variation_map.insert(std::move(std::make_pair(var_name, std::move(var))));
  m_variation_names.insert(var_name);
}

template <typename Helper>
auto queryosity::varied<queryosity::todo<Helper>>::nominal() -> todo<Helper> & {
  return m_nominal;
}

template <typename Helper>
auto queryosity::varied<queryosity::todo<Helper>>::nominal() const
    -> todo<Helper> const & {
  return m_nominal;
}

template <typename Helper>
auto queryosity::varied<queryosity::todo<Helper>>::variation(
    const std::string &var_name) -> todo<Helper> & {
  return (this->has_variation(var_name) ? m_variation_map.at(var_name)
                                        : m_nominal);
}

template <typename Helper>
auto queryosity::varied<queryosity::todo<Helper>>::variation(
    const std::string &var_name) const -> todo<Helper> const & {
  return (this->has_variation(var_name) ? m_variation_map.at(var_name)
                                        : m_nominal);
}

template <typename Helper>
bool queryosity::varied<queryosity::todo<Helper>>::has_variation(
    const std::string &var_name) const {
  return m_variation_map.find(var_name) != m_variation_map.end();
}

template <typename Helper>
std::set<std::string>
queryosity::varied<queryosity::todo<Helper>>::get_variation_names() const {
  return m_variation_names;
}

template <typename Helper>
template <typename... Cols, typename V,
          std::enable_if_t<queryosity::column::is_evaluatable_v<V>, bool>>
auto queryosity::varied<queryosity::todo<Helper>>::evaluate(Cols &&...cols)
    -> varied<decltype(this->nominal().evaluate(
        std::forward<Cols>(cols.nominal)...))> {
  using varied_type = varied<decltype(this->nominal().evaluate(
      std::forward<Cols>(cols.nominal)...))>;
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
auto queryosity::varied<queryosity::todo<Helper>>::apply(Cols &&...cols)
    -> varied<queryosity::lazy<selection::applied_t<V>>> {
  using varied_type = varied<queryosity::lazy<selection::applied_t<V>>>;
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
auto queryosity::varied<queryosity::todo<Helper>>::fill(Nodes const &...columns)
    -> varied {
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
auto queryosity::varied<queryosity::todo<Helper>>::at(Node const &selection)
    -> varied<lazy<query::booked_t<V>>> {
  using varied_type = varied<lazy<query::booked_t<V>>>;
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
auto queryosity::varied<queryosity::todo<Helper>>::at(
    Nodes const &...selections)
    -> std::array<varied<lazy<query::booked_t<V>>>, sizeof...(Nodes)> {
  // variations
  using varied_type = varied<lazy<query::booked_t<V>>>;
  using array_of_varied_type =
      std::array<varied<lazy<query::booked_t<V>>>, sizeof...(Nodes)>;
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
auto queryosity::varied<queryosity::todo<Helper>>::operator()(Cols &&...cols)
    -> varied<decltype(std::declval<todo<Helper>>().operator()(
        std::forward<Cols>(cols).nominal()...))> {

  using varied_type = varied<decltype(std::declval<todo<Helper>>().operator()(
      std::forward<Cols>(cols).nominal()...))>;

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
