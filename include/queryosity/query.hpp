#pragma once

#include <functional>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "action.hpp"

/**
 * @brief All queryosity namespaces and classes.
 */
namespace queryosity {

namespace column {

template <typename T> class view;

template <typename T> class valued;

template <typename T> class variable;

template <typename T> class observable;

} // namespace column

namespace selection {

class node;

}

/**
 * @brief Perform a query.
 */
namespace query {

class experiment;

template <typename T> class aggregation;

template <typename... Ts> class fillable;

template <typename T> class definition;

template <typename T> class booker;

template <typename T> class series;

template <typename T> class calculation;

template <typename T> struct output;

class node : public action {

public:
  node();
  virtual ~node() = default;

  void apply_scale(double scale);
  void use_weight(bool use = true);

  void set_selection(const selection::node &selection);
  const selection::node *get_selection() const;

  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) override;
  virtual void execute(unsigned int slot, unsigned long long entry) final override;
  virtual void finalize(unsigned int slot) override;

  virtual void count(double w) = 0;

protected:
  double m_scale;
  const selection::node *m_selection;
};

template <typename T>
constexpr std::true_type check_implemented(query::aggregation<T> const &);
constexpr std::false_type check_implemented(...);

template <typename... Vals>
constexpr std::true_type
check_fillable(query::fillable<Vals...> const &);
constexpr std::false_type check_fillable(...);

template <typename T> struct is_bookable : std::false_type {};
template <typename T> struct is_bookable<query::booker<T>> : std::true_type {};

template <typename T>
constexpr bool is_aggregation_v =
    decltype(check_implemented(std::declval<T>()))::value;

template <typename T>
constexpr bool is_fillable_v =
    decltype(check_fillable(std::declval<T>()))::value;

template <typename T> constexpr bool is_bookable_v = is_bookable<T>::value;

template <typename Bkr> using booked_t = typename Bkr::booked_type;

// mixin class to conditionally add a member variable
template <typename Action, typename Enable = void> struct result_of {};

// Specialization for types satisfying is_query
template <typename Action>
struct result_of<Action, std::enable_if_t<query::is_aggregation_v<Action>>> {
  using result_type = decltype(std::declval<Action>().result());
  result_of() : m_merged(false) {}
  virtual ~result_of() = default;

protected:
  result_type m_result;
  bool m_merged;
};

} // namespace query

} // namespace queryosity

#include "column.hpp"
#include "selection.hpp"

inline queryosity::query::node::node() : m_scale(1.0), m_selection(nullptr) {}

inline void
queryosity::query::node::set_selection(const selection::node &selection) {
  m_selection = &selection;
}

inline const queryosity::selection::node *
queryosity::query::node::get_selection() const {
  return m_selection;
}

inline void queryosity::query::node::apply_scale(double scale) {
  m_scale *= scale;
}

inline void queryosity::query::node::initialize(unsigned int,
                                                unsigned long long,
                                                unsigned long long) {
  if (!m_selection)
    throw std::runtime_error("no booked selection");
}

inline void queryosity::query::node::execute(unsigned int, unsigned long long) {
  if (m_selection->passed_cut()) {
    this->count(m_scale * m_selection->get_weight());
  }
}

inline void queryosity::query::node::finalize(unsigned int) {}