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

template <typename T, typename U> class applicator;

struct count_t;

class counter;

template <typename... Ts> struct yield;

class node : public column::calculation<double> {

public:
  node(const selection::node *presel, column::variable<double> dec);
  virtual ~node() = default;

public:
  bool is_initial() const noexcept;
  const selection::node *get_previous() const noexcept;

  virtual bool passed_cut() const = 0;
  virtual double get_weight() const = 0;

  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) final override;
  virtual void execute(unsigned int slot, unsigned long long entry) final override;
  virtual void finalize(unsigned int slot) final override;

protected:
  const selection::node *const m_preselection;
  column::variable<double> m_decision;
};

template <typename T> struct is_applicable : std::false_type {};
template <typename T, typename U>
struct is_applicable<selection::applicator<T,U>> : std::true_type {};
template <typename T>
static constexpr bool is_applicable_v = is_applicable<T>::value;

template <typename T>
using applied_t = typename T::selection_type;

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

inline void queryosity::selection::node::initialize(unsigned int slot, unsigned long long begin,
                                               unsigned long long end) {
  column::calculation<double>::initialize(slot, begin, end);
                                               }

inline void queryosity::selection::node::execute(unsigned int slot, unsigned long long entry) {
  column::calculation<double>::execute(slot, entry);
}

inline void queryosity::selection::node::finalize(unsigned int slot) {
  column::calculation<double>::finalize(slot);
}