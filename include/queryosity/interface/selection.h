#pragma once

#include <memory>
#include <string>
#include <vector>

#include "action.h"
#include "column.h"

namespace queryosity {

/**
 * @brief Apply cuts and weights to entries.
 */
namespace selection {

class cutflow;

class cut;
class weight;

template <typename T> class apply;

class node : public column::calculation<double> {

public:
  node(const selection::node *presel, column::variable<double> dec);
  virtual ~node() = default;

public:
  bool is_initial() const noexcept;
  const selection::node *get_previous() const noexcept;

  template <typename T> void set_decision(column::valued<T> const &dec);

  virtual bool passed_cut() const = 0;
  virtual double get_weight() const = 0;

protected:
  const selection::node *const m_preselection;
  column::variable<double> m_decision;
};

template <typename T> struct is_applicable : std::false_type {};
template <typename T>
struct is_applicable<selection::apply<T>> : std::true_type {};
template <typename T>
static constexpr bool is_applicable_v = is_applicable<T>::value;

template <typename F>
using apply_t = typename selection::template apply<
    queryosity::column::template equation_t<F>>;

} // namespace selection

template <typename T>
constexpr bool is_selection_v = std::is_base_of_v<selection::node, T>;
} // namespace queryosity

#include "column_equation.h"
#include "query.h"

inline queryosity::selection::node::node(const selection::node *presel,
                                         column::variable<double> dec)
    : m_preselection(presel), m_decision(std::move(dec)) {}

inline bool queryosity::selection::node::is_initial() const noexcept {
  return m_preselection ? false : true;
}

inline const queryosity::selection::node *
queryosity::selection::node::get_previous() const noexcept {
  return m_preselection;
}