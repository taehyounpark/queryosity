#pragma once

#include <functional>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "action.h"

namespace ana {

template <typename T> class term;

template <typename T> class cell;

template <typename T> class variable;

template <typename T> class observable;

namespace selection {

class node;

}

namespace counter {

class experiment;

template <typename T> class aggregation;

template <typename T> class definition;

template <typename T> class book;

template <typename T> class output;

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
  virtual void execute(unsigned int slot, unsigned long long entry) override;

  virtual void count(double w) = 0;

protected:
  bool m_raw;
  double m_scale;
  const selection::node *m_selection;
};

template <typename T>
constexpr std::true_type check_implemented(const counter::aggregation<T> &);
constexpr std::false_type check_implemented(...);

template <typename Out, typename... Vals>
constexpr std::true_type
check_fillable(const typename counter::definition<Out(Vals...)> &);
constexpr std::false_type check_fillable(...);

template <typename T> struct is_book : std::false_type {};
template <typename T> struct is_book<counter::book<T>> : std::true_type {};

template <typename T>
constexpr bool is_implemented_v =
    decltype(check_implemented(std::declval<T>()))::value;

template <typename T>
constexpr bool is_fillable_v =
    decltype(check_fillable(std::declval<T>()))::value;

template <typename T> constexpr bool is_bookable_v = is_book<T>::value;

template <typename Bkr> using booked_t = typename Bkr::counter_type;

} // namespace counter

} // namespace ana

#include "column.h"
#include "selection.h"

inline ana::counter::node::node()
    : m_raw(false), m_scale(1.0), m_selection(nullptr) {}

inline void
ana::counter::node::set_selection(const selection::node &selection) {
  m_selection = &selection;
}

inline const ana::selection::node *ana::counter::node::get_selection() const {
  return m_selection;
}

inline void ana::counter::node::apply_scale(double scale) { m_scale *= scale; }

inline void ana::counter::node::use_weight(bool use) { m_raw = !use; }

inline void ana::counter::node::initialize(unsigned int, unsigned long long,
                                           unsigned long long) {
  if (!m_selection)
    throw std::runtime_error("no booked selection");
}

inline void ana::counter::node::execute(unsigned int, unsigned long long) {
  if (m_selection->passed_cut()) {
    this->count(m_raw ? 1.0 : m_scale * m_selection->get_weight());
  }
}
