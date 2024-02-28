#pragma once

#include <memory>
#include <vector>

#include "selection_cutflow.h"

#include "counter.h"
#include "counter_book.h"

namespace ana {

class counter::experiment : public selection::cutflow {

public:
  experiment(double scale);
  ~experiment() = default;

public:
  template <typename Cnt, typename... Args>
  std::unique_ptr<counter::book<Cnt>> agg(Args &&...args);

  template <typename Cnt>
  auto book(counter::book<Cnt> const &bkr, const selection::node &sel)
      -> std::unique_ptr<Cnt>;

  void clear_counters();

protected:
  void add_counter(counter::node &cnt);

protected:
  std::vector<counter::node *> m_counters;
  const double m_scale;
};

} // namespace ana

inline ana::counter::experiment::experiment(double scale) : m_scale(scale) {}

inline void ana::counter::experiment::add_counter(ana::counter::node &cnt) {
  m_counters.push_back(&cnt);
}

inline void ana::counter::experiment::clear_counters() { m_counters.clear(); }

template <typename Cnt, typename... Args>
std::unique_ptr<ana::counter::book<Cnt>>
ana::counter::experiment::agg(Args &&...args) {
  auto bkr = std::make_unique<counter::book<Cnt>>(std::forward<Args>(args)...);
  return bkr;
}

template <typename Cnt>
auto ana::counter::experiment::book(counter::book<Cnt> const &bkr,
                                    const selection::node &sel)
    -> std::unique_ptr<Cnt> {
  auto cnt = bkr.set_selection(sel);
  cnt->apply_scale(m_scale);
  this->add_counter(*cnt);
  return cnt;
}