#pragma once

#include <functional>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "operation.h"

namespace ana {

template <typename T> class term;

template <typename T> class cell;

template <typename T> class variable;

template <typename T> class observable;

class selection;

namespace counter {

class experiment;

template <typename T> class implementation;

template <typename T> class definition;

template <typename T> class booker;

template <typename T> class output;

class counter_base : public operation {

public:
  counter_base();
  virtual ~counter_base() = default;

  void apply_scale(double scale);
  void use_weight(bool use = true);

  void set_selection(const selection &selection);
  const selection *get_selection() const;

  virtual void initialize(const dataset::range &part) override;
  virtual void execute(const dataset::range &part,
                       unsigned long long entry) override;

  virtual void count(double w) = 0;

protected:
  bool m_raw;
  double m_scale;
  const selection *m_selection;
};

template <typename T>
constexpr std::true_type check_implemented(const counter::implementation<T> &);
constexpr std::false_type check_implemented(...);

template <typename Out, typename... Vals>
constexpr std::true_type
check_fillable(const typename counter::definition<Out(Vals...)> &);
constexpr std::false_type check_fillable(...);

template <typename T> struct is_booker : std::false_type {};
template <typename T> struct is_booker<counter::booker<T>> : std::true_type {};

template <typename T>
constexpr bool is_implemented_v =
    decltype(check_implemented(std::declval<T>()))::value;

template <typename T>
constexpr bool is_fillable_v =
    decltype(check_fillable(std::declval<T>()))::value;

template <typename T> constexpr bool is_booker_v = is_booker<T>::value;

template <typename Bkr> using booked_t = typename Bkr::counter_type;

} // namespace counter

} // namespace ana

#include "column.h"
#include "selection.h"

inline ana::counter::counter_base::counter_base()
    : m_raw(false), m_scale(1.0), m_selection(nullptr) {}

inline void
ana::counter::counter_base::set_selection(const selection &selection) {
  m_selection = &selection;
}

inline const ana::selection *ana::counter::counter_base::get_selection() const {
  return m_selection;
}

inline void ana::counter::counter_base::apply_scale(double scale) {
  m_scale *= scale;
}

inline void ana::counter::counter_base::use_weight(bool use) { m_raw = !use; }

inline void
ana::counter::counter_base::initialize(const ana::dataset::range &) {
  if (!m_selection)
    throw std::runtime_error("no booked selection");
}

inline void ana::counter::counter_base::execute(const ana::dataset::range &,
                                                unsigned long long) {
  if (m_selection->passed_cut()) {
    this->count(m_raw ? 1.0 : m_scale * m_selection->get_weight());
  }
}
