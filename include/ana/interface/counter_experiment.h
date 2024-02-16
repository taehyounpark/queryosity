#pragma once

#include <memory>
#include <vector>

#include "selection_cutflow.h"

#include "counter.h"
#include "counter_booker.h"

namespace ana {

class counter::experiment : public selection::cutflow {

public:
  experiment(double scale);
  ~experiment() = default;

public:
  template <typename Cnt, typename... Args>
  std::unique_ptr<booker<Cnt>> agg(Args &&...args);

  template <typename Cnt>
  auto book(booker<Cnt> const &bkr, const selection &sel)
      -> std::unique_ptr<Cnt>;

  void clear_counters();

protected:
  void add_counter(counter::counter_base &cnt);

protected:
  std::vector<counter::counter_base *> m_counters;
  const double m_scale;
};

} // namespace ana

inline ana::counter::experiment::experiment(double scale) : m_scale(scale) {}

inline void
ana::counter::experiment::add_counter(ana::counter::counter_base &cnt) {
  m_counters.push_back(&cnt);
}

inline void ana::counter::experiment::clear_counters() { m_counters.clear(); }

template <typename Cnt, typename... Args>
std::unique_ptr<ana::counter::booker<Cnt>>
ana::counter::experiment::agg(Args &&...args) {
  auto bkr = std::make_unique<booker<Cnt>>(std::forward<Args>(args)...);
  return bkr;
}

template <typename Cnt>
auto ana::counter::experiment::book(booker<Cnt> const &bkr,
                                    const selection &sel)
    -> std::unique_ptr<Cnt> {
  auto cnt = bkr.set_selection(sel);
  cnt->apply_scale(m_scale);
  this->add_counter(*cnt);
  return cnt;
}