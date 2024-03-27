#pragma once

#include <iostream>
#include <memory>
#include <set>
#include <type_traits>
#include <vector>

#include "dataflow.h"
#include "detail.h"
#include "systematic_resolver.h"

namespace queryosity {

// mixin class to conditionally add a member variable
template <typename Action, typename Enable = void>
struct result_if_aggregation {};

// Specialization for types satisfying is_query
template <typename Action>
struct result_if_aggregation<
    Action, std::enable_if_t<query::is_aggregation_v<Action>>> {
  using result_type = decltype(std::declval<Action>().result());
  result_if_aggregation() : m_merged(false) {}
  virtual ~result_if_aggregation() = default;

protected:
  result_type m_result;
  bool m_merged;
};

/**
 * @ingroup api
 * @brief Lazy action over dataset.
 * @tparam Action Action to be performed.
 */
template <typename Action>
class lazy : public dataflow::node,
             public ensemble::slotted<Action>,
             public systematic::resolver<lazy<Action>>,
             public result_if_aggregation<Action> {

public:
  class varied;

public:
  using action_type = Action;

public:
  friend class dataflow;
  template <typename> friend class lazy;

public:
  lazy(dataflow &df, std::vector<Action *> const &slots)
      : dataflow::node(df), m_slots(slots) {}

  template <typename Derived>
  lazy(dataflow &df, std::vector<Derived *> const &slots);
  template <typename Derived>
  lazy(dataflow &df, std::vector<std::unique_ptr<Derived>> const &slots);

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

  template <typename Col> auto filter(lazy<Col> const &col) const;
  template <typename Col> auto weight(lazy<Col> const &col) const;

  template <typename Col> auto filter(Col const &col) const;
  template <typename Col> auto weight(Col const &col) const;

  template <typename Expr, typename... Cols>
  auto filter(queryosity::column::expression<Expr> const &expr,
              Cols const &...cols) const;

  template <typename Expr, typename... Cols>
  auto weight(queryosity::column::expression<Expr> const &expr,
              Cols const &...cols) const;

  template <typename Agg> auto book(Agg &&agg) const;
  template <typename... Aggs> auto book(Aggs &&...aggs) const;

  template <typename V = Action,
            std::enable_if_t<queryosity::query::is_aggregation_v<V>,
                             bool> = false>
  auto result() -> decltype(std::declval<V>().result());

  template <typename V = Action,
            std::enable_if_t<queryosity::query::is_aggregation_v<V>,
                             bool> = false>
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
  template <typename V = Action,
            std::enable_if_t<queryosity::query::is_aggregation_v<V>,
                             bool> = false>
  void merge_results();

protected:
  std::vector<Action *> m_slots;
};

} // namespace queryosity

#include "column.h"
#include "lazy_varied.h"
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
    return this->m_df->template _select<selection::cut>(*this, col);
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "filter must be called from a selection");
  }
}

template <typename Action>
template <typename Col>
auto queryosity::lazy<Action>::weight(lazy<Col> const &col) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    return this->m_df->template _select<selection::weight>(*this, col);
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
    auto syst = varied_type(*this->m_df, this->filter(col.nominal()));
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
    auto syst = varied_type(*this->m_df, this->weight(col.nominal()));
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
template <typename Expr, typename... Cols>
auto queryosity::lazy<Action>::filter(
    queryosity::column::expression<Expr> const &expr,
    Cols const &...cols) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    auto col = this->m_df->define(expr, cols...);
    return this->filter(col);
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "filter must be called from a selection");
  }
}

template <typename Action>
template <typename Expr, typename... Cols>
auto queryosity::lazy<Action>::weight(
    queryosity::column::expression<Expr> const &expr,
    Cols const &...cols) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    auto col = this->m_df->define(expr, cols...);
    return this->weight(col);
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "filter must be called from a selection");
  }
}

template <typename Action>
template <typename Agg>
auto queryosity::lazy<Action>::book(Agg &&agg) const {
  static_assert(std::is_base_of_v<selection::node, Action>,
                "book must be called from a selection");
  return agg.book(*this);
}

template <typename Action>
template <typename... Aggs>
auto queryosity::lazy<Action>::book(Aggs &&...aggs) const {
  static_assert(std::is_base_of_v<selection::node, Action>,
                "book must be called from a selection");
  return std::make_tuple((aggs.book(*this), ...));
}

template <typename Action>
template <
    typename V,
    std::enable_if_t<queryosity::query::is_aggregation_v<V>, bool>>
auto queryosity::lazy<Action>::result()
    -> decltype(std::declval<V>().result()) {
  this->m_df->analyze();
  this->merge_results();
  return this->m_result;
}

template <typename Action>
template <
    typename V,
    std::enable_if_t<queryosity::query::is_aggregation_v<V>, bool> e>
void queryosity::lazy<Action>::merge_results() {
  if (this->m_merged)
    return;
  auto model = this->get_slot(0);
  using result_type = decltype(model->result());
  const auto nslots = this->size();
  if (nslots == 1) {
    this->m_result = model->result();
  } else {
    std::vector<result_type> results;
    results.reserve(nslots);
    for (size_t islot = 0; islot < nslots; ++islot) {
      results.push_back(std::move(this->get_slot(islot)->result()));
    }
    this->m_result = model->merge(results);
  }
  this->m_merged = true;
}