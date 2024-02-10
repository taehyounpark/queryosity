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

class aggregation : public operation {

public:
  class experiment;

  template <typename T> class output;

  template <typename T> class logic;

  template <typename T> class booker;

  template <typename T> class bookkeeper;

  template <typename T> class summary;

public:
  aggregation();
  virtual ~aggregation() = default;

  void apply_scale(double scale);
  void use_weight(bool use = true);

  void set_selection(const selection &selection);
  const selection *get_selection() const;

  virtual void initialize(const dataset::range &part) override;
  virtual void execute(const dataset::range &part,
                       unsigned long long entry) override;

  virtual void aggregate(double w) = 0;

protected:
  bool m_raw;
  double m_scale;
  const selection *m_selection;

public:
  template <typename T>
  static constexpr std::true_type
  check_implemented(const aggregation::output<T> &);
  static constexpr std::false_type check_implemented(...);

  template <typename Out, typename... Vals>
  static constexpr std::true_type
  check_fillable(const typename aggregation::logic<Out(Vals...)> &);
  static constexpr std::false_type check_fillable(...);

  template <typename T> struct is_booker : std::false_type {};
  template <typename T>
  struct is_booker<aggregation::booker<T>> : std::true_type {};

  template <typename T>
  static constexpr bool has_output_v =
      decltype(check_implemented(std::declval<T>()))::value;

  template <typename T>
  static constexpr bool is_fillable_v =
      decltype(check_fillable(std::declval<T>()))::value;

  template <typename T> static constexpr bool is_booker_v = is_booker<T>::value;

  template <typename Bkr> using booked_t = typename Bkr::aggregation_type;
};

} // namespace ana

#include "column.h"
#include "selection.h"

inline ana::aggregation::aggregation()
    : m_raw(false), m_scale(1.0), m_selection(nullptr) {}

inline void ana::aggregation::set_selection(const selection &selection) {
  m_selection = &selection;
}

inline const ana::selection *ana::aggregation::get_selection() const {
  return m_selection;
}

inline void ana::aggregation::apply_scale(double scale) { m_scale *= scale; }

inline void ana::aggregation::use_weight(bool use) { m_raw = !use; }

inline void ana::aggregation::initialize(const ana::dataset::range &) {
  if (!m_selection)
    throw std::runtime_error("no booked selection");
}

inline void ana::aggregation::execute(const ana::dataset::range &,
                                      unsigned long long) {
  if (m_selection->passed_cut()) {
    this->aggregate(m_raw ? 1.0 : m_scale * m_selection->get_weight());
  }
}
