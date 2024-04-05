#pragma once

#include <iostream>
#include <memory>
#include <set>
#include <type_traits>
#include <vector>

#include "dataflow.h"
#include "detail.h"
#include "query.h"
#include "systematic_resolver.h"

namespace queryosity {

/**
 * @ingroup api
 * @brief Lazy action over dataset.
 * @tparam Action Action to be performed.
 */
template <typename Action>
class lazy : public dataflow::node,
             public ensemble::slotted<Action>,
             public systematic::resolver<lazy<Action>>,
             public query::result_if_aggregation<Action> {

public:
  class varied;

public:
  using action_type = Action;

public:
  friend class dataflow;
  template <typename> friend class lazy;
  template <typename> friend struct column::series; // access to dataflow

public:
  lazy(dataflow &df, std::vector<Action *> const &slots)
      : dataflow::node(df), m_slots(slots) {}

  template <typename Derived>
  lazy(dataflow &df, std::vector<Derived *> const &slots);
  template <typename Derived>
  lazy(dataflow &df, std::vector<std::unique_ptr<Derived>> const &slots);

  template <typename Base> operator lazy<Base>() const;

  lazy(const lazy &) = default;
  lazy &operator=(const lazy &) = default;

  lazy(lazy &&) = default;
  lazy &operator=(lazy &&) = default;

  virtual ~lazy() = default;

  virtual std::vector<Action *> const &get_slots() const override;

  virtual void set_variation(const std::string &var_name, lazy var) override;

  virtual lazy &nominal() override;
  virtual lazy &variation(const std::string &var_name) override;
  virtual lazy const &nominal() const override;
  virtual lazy const &variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> get_variation_names() const override;

  template <typename To, typename V = Action,
            std::enable_if_t<queryosity::is_column_v<V>, bool> = false>
  auto to() const -> lazy<column::valued<To>>;

  /**
   * @brief Compound a weight to from this selection.
   * @tparam Col Column type.
   * @param[in] column Input lazy column used as weight decision.
   * @details The current selection becomes its prerequisite in the cutflow: all
   * prerequisites must pass in order for a downstream selection to pass.
   * @return Compounded lazy weight.
   */
  template <typename Col> auto filter(lazy<Col> const &column) const;

  /**
   * @brief Compound a weight to from this selection.
   * @tparam Col Column type.
   * @param[in] column Input lazy column used as weight decision.
   * @return Compounded lazy weight.
   */
  template <typename Col> auto weight(lazy<Col> const &column) const;

  /**
   * @brief Compound a varied cut to this selection.
   * @tparam Col Varied lazy column type.
   * @param[in] column Input varied column used as cut decision.
   * @return Varied lazy cut.
   */
  template <typename Col> auto filter(Col const &column) const;

  /**
   * @brief Compound a varied weight to this selection.
   * @tparam Col Varied lazy column type.
   * @param[in] column Input varied column used as weight decision.
   * @return Varied lazy weight.
   */
  template <typename Col> auto weight(Col const &column) const;

  /**
   * @brief Compound a cut to this selection.
   * @tparam Expr Callable type (C++ function, functor, lambda, etc.).
   * @param[in] column Input lazy column used as cut decision.
   * @return Selection evaluator.
   */
  template <typename Expr>
  auto filter(queryosity::column::expression<Expr> const &expr) const;

  /**
   * @brief Compound a weight to from this selection.
   * @tparam Expr Callable type (C++ function, functor, lambda, etc.).
   * @param[in] expr Expression used to evaluate the weight decision.
   * @return Selection evaluator.
   */
  template <typename Expr>
  auto weight(queryosity::column::expression<Expr> const &expr) const;

  /**
   * @brief Book a query at this selection.
   * @tparam Qry (Varied) query booker type.
   * @param[in] qry Query booker.
   * @details The query booker should have already been filled with input
   * columns (if applicable).
   * @return (Varied) lazy query.
   */
  template <typename Qry> auto book(Qry &&qry) const;

  /**
   * @brief Book multiple queries at this selection.
   * @tparam Qrys (Varied) query booker types.
   * @param[in] qrys Query bookers.
   * @details The query bookers should have already been filled with input
   * columns (if applicable).
   * @return (Varied) lazy queries.
   */
  template <typename... Qrys> auto book(Qrys &&...qrys) const;

  /**
   * @brief Get a column series for the entries passing the selection.
   * @tparam Col Lazy column type.
   * @return Lazy query.
   * @attention The weight value does not apply in the population of the
   * series.
   */
  template <typename Col,
            std::enable_if_t<queryosity::is_nominal_v<Col>, bool> = false>
  auto get(column::series<Col> const &col)
      -> lazy<query::series<typename column::series<Col>::value_type>>;

  /**
   * @brief Get a column series for the entries passing the selection.
   * @tparam Col Varied column type.
   * @return Varied lazy query.
   * @attention The weight value does not apply in the population of the
   * series.
   */
  template <typename Col,
            std::enable_if_t<queryosity::is_varied_v<Col>, bool> = false>
  auto get(column::series<Col> const &col) -> typename lazy<
      query::series<typename column::series<Col>::value_type>>::varied;

  /**
   * @brief (Process and) retrieve the result of a query.
   * @return Query result.
   * @attention Invoking this turns *all* lazy actions in the dataflow *eager*.
   */
  template <
      typename V = Action,
      std::enable_if_t<queryosity::query::is_aggregation_v<V>, bool> = false>
  auto result() -> decltype(std::declval<V>().result());

  /**
   * @brief Shortcut for `result()`.
   */
  template <
      typename V = Action,
      std::enable_if_t<queryosity::query::is_aggregation_v<V>, bool> = false>
  auto operator->() -> decltype(std::declval<V>().result()) {
    return this->result();
  }

  DEFINE_LAZY_UNARY_OP(logical_not, !)
  DEFINE_LAZY_UNARY_OP(minus, -)
  DEFINE_LAZY_BINARY_OP(equality, ==)
  DEFINE_LAZY_BINARY_OP(inequality, !=)
  DEFINE_LAZY_BINARY_OP(addition, +)
  DEFINE_LAZY_BINARY_OP(subtraction, -)
  DEFINE_LAZY_BINARY_OP(multiplication, *)
  DEFINE_LAZY_BINARY_OP(division, /)
  DEFINE_LAZY_BINARY_OP(logical_or, ||)
  DEFINE_LAZY_BINARY_OP(logical_and, &&)
  DEFINE_LAZY_BINARY_OP(greater_than, >)
  DEFINE_LAZY_BINARY_OP(less_than, <)
  DEFINE_LAZY_BINARY_OP(greater_than_or_equal_to, >=)
  DEFINE_LAZY_BINARY_OP(less_than_or_equal_to, <=)
  DEFINE_LAZY_INDEX_OP()

protected:
  template <
      typename V = Action,
      std::enable_if_t<queryosity::query::is_aggregation_v<V>, bool> = false>
  void merge_results();

protected:
  std::vector<Action *> m_slots;
};

} // namespace queryosity

#include "column.h"
#include "column_series.h"
#include "lazy_varied.h"
#include "query_series.h"
#include "todo.h"

template <typename Action>
template <typename Derived>
queryosity::lazy<Action>::lazy(dataflow &df,
                               std::vector<Derived *> const &slots)
    : dataflow::node(df) {
  m_slots.clear();
  m_slots.reserve(slots.size());
  for (auto slot : slots) {
    m_slots.push_back(static_cast<Action *>(slot));
  }
}

template <typename Action>
template <typename Derived>
queryosity::lazy<Action>::lazy(
    dataflow &df, std::vector<std::unique_ptr<Derived>> const &slots)
    : dataflow::node(df) {
  m_slots.clear();
  m_slots.reserve(slots.size());
  for (auto const &slot : slots) {
    m_slots.push_back(static_cast<Action *>(slot.get()));
  }
}

template <typename Action>
template <typename Base>
queryosity::lazy<Action>::operator lazy<Base>() const {
  return lazy<Base>(*this->m_df, this->m_slots);
}

template <typename Action>
std::vector<Action *> const &queryosity::lazy<Action>::get_slots() const {
  return this->m_slots;
}

template <typename Action>
void queryosity::lazy<Action>::set_variation(const std::string &, lazy) {
  // should never be called
  throw std::logic_error("cannot set variation to a nominal-only action");
}

template <typename Action> auto queryosity::lazy<Action>::nominal() -> lazy & {
  // this is nominal
  return *this;
}

template <typename Action>
auto queryosity::lazy<Action>::variation(const std::string &) -> lazy & {
  // propagation of variations must occur "transparently"
  return *this;
}

template <typename Action>
auto queryosity::lazy<Action>::nominal() const -> lazy const & {
  // this is nominal
  return *this;
}

template <typename Action>
auto queryosity::lazy<Action>::variation(const std::string &) const
    -> lazy const & {
  // propagation of variations must occur "transparently"
  return *this;
}

template <typename Action>
std::set<std::string> queryosity::lazy<Action>::get_variation_names() const {
  // no variations to list
  return std::set<std::string>();
}

template <typename Action>
bool queryosity::lazy<Action>::has_variation(const std::string &) const {
  // always false
  return false;
}

template <typename Action>
template <typename To, typename V,
          std::enable_if_t<queryosity::is_column_v<V>, bool>>
auto queryosity::lazy<Action>::to() const -> lazy<column::valued<To>> {
  if constexpr (std::is_same_v<To, column::value_t<V>> ||
                std::is_base_of_v<To, column::value_t<V>>) {
    return lazy<column::valued<To>>(*this->m_df, this->get_slots());
  } else {
    return lazy<column::valued<To>>(
        *this->m_df, this->m_df->template _convert<To>(*this).get_slots());
  }
}

template <typename Action>
template <typename Col>
auto queryosity::lazy<Action>::filter(lazy<Col> const &col) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    return this->m_df->template _apply<selection::cut>(*this, col);
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "filter must be called from a selection");
  }
}

template <typename Action>
template <typename Col>
auto queryosity::lazy<Action>::weight(lazy<Col> const &col) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    return this->m_df->template _apply<selection::weight>(*this, col);
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "filter must be called from a selection");
  }
}

template <typename Action>
template <typename Col>
auto queryosity::lazy<Action>::filter(Col const &col) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    using varied_type = typename lazy<selection::node>::varied;
    auto syst = varied_type(this->filter(col.nominal()));
    for (auto const &var_name : col.get_variation_names()) {
      syst.set_variation(var_name, this->filter(col.variation(var_name)));
    }
    return syst;
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "filter must be called from a selection");
  }
}

template <typename Action>
template <typename Col>
auto queryosity::lazy<Action>::weight(Col const &col) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    using varied_type = typename lazy<selection::node>::varied;
    auto syst = varied_type(this->weight(col.nominal()));
    for (auto const &var_name : col.get_variation_names()) {
      syst.set_variation(var_name, this->weight(col.variation(var_name)));
    }
    return syst;
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "weight must be called from a selection");
  }
}

template <typename Action>
template <typename Expr>
auto queryosity::lazy<Action>::filter(
    queryosity::column::expression<Expr> const &expr) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    return this->m_df->template _select<selection::cut>(*this, expr);
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "filter must be called from a selection");
  }
}

template <typename Action>
template <typename Expr>
auto queryosity::lazy<Action>::weight(
    queryosity::column::expression<Expr> const &expr) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    return this->m_df->template _select<selection::weight>(*this, expr);
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "filter must be called from a selection");
  }
}

template <typename Action>
template <typename Qry>
auto queryosity::lazy<Action>::book(Qry &&qry) const {
  static_assert(std::is_base_of_v<selection::node, Action>,
                "book must be called from a selection");
  return qry.at(*this);
}

template <typename Action>
template <typename... Qrys>
auto queryosity::lazy<Action>::book(Qrys &&...qrys) const {
  static_assert(std::is_base_of_v<selection::node, Action>,
                "book must be called from a selection");
  return std::make_tuple((qrys.at(*this), ...));
}

template <typename Action>
template <typename Col, std::enable_if_t<queryosity::is_nominal_v<Col>, bool>>
auto queryosity::lazy<Action>::get(queryosity::column::series<Col> const &col)
    -> lazy<query::series<typename column::series<Col>::value_type>> {
  return col.make(*this);
}

template <typename Action>
template <typename Col, std::enable_if_t<queryosity::is_varied_v<Col>, bool>>
auto queryosity::lazy<Action>::get(queryosity::column::series<Col> const &col)
    -> typename lazy<
        query::series<typename column::series<Col>::value_type>>::varied {
  return col.make(*this);
}

template <typename Action>
template <typename V,
          std::enable_if_t<queryosity::query::is_aggregation_v<V>, bool>>
auto queryosity::lazy<Action>::result()
    -> decltype(std::declval<V>().result()) {
  this->m_df->analyze();
  this->merge_results();
  return this->m_result;
}

template <typename Action>
template <typename V,
          std::enable_if_t<queryosity::query::is_aggregation_v<V>, bool> e>
void queryosity::lazy<Action>::merge_results() {
  if (this->m_merged)
    return;
  auto model = this->get_slot(0);
  using result_type = decltype(model->result());
  const auto nslots = this->size();
  if (nslots == 1) {
    this->m_result = std::move(model->result());
  } else {
    std::vector<result_type> results;
    results.reserve(nslots);
    for (size_t islot = 0; islot < nslots; ++islot) {
      results.push_back(std::move(this->get_slot(islot)->result()));
    }
    this->m_result = std::move(model->merge(results));
  }
  this->m_merged = true;
}