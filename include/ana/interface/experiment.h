#pragma once

#include <memory>
#include <vector>

#include "counter.h"
#include "cutflow.h"

namespace ana {

class counter::experiment : public selection::cutflow {

public:
  experiment(double scale);
  ~experiment() = default;

public:
  template <typename Cnt, typename... Args>
  std::shared_ptr<booker<Cnt>> book(Args &&...args);

  template <typename Cnt>
  auto select_counter(booker<Cnt> const &bkr, const selection &sel)
      -> std::shared_ptr<Cnt>;

  template <typename Cnt, typename... Sels>
  auto select_counters(booker<Cnt> const &bkr, Sels const &...sels)
      -> std::shared_ptr<booker<Cnt>>;

  void clear_counters();

protected:
  void add_counter(counter &cnt);

protected:
  std::vector<counter *> m_counters;
  const double m_norm;
};

} // namespace ana

inline ana::counter::experiment::experiment(double norm) : m_norm(norm) {}

inline void ana::counter::experiment::add_counter(ana::counter &cnt) {
  m_counters.push_back(&cnt);
}

inline void ana::counter::experiment::clear_counters() { m_counters.clear(); }

template <typename Cnt, typename... Args>
std::shared_ptr<ana::counter::booker<Cnt>>
ana::counter::experiment::book(Args &&...args) {
  auto bkr = std::make_shared<booker<Cnt>>(std::forward<Args>(args)...);
  return bkr;
}

template <typename Cnt>
auto ana::counter::experiment::select_counter(booker<Cnt> const &bkr,
                                              const selection &sel)
    -> std::shared_ptr<Cnt> {
  auto cnt = bkr.select_counter(sel);
  cnt->apply_scale(m_norm);
  this->add_counter(*cnt);
  return cnt;
}
template <typename Cnt, typename... Sels>
auto ana::counter::experiment::select_counters(booker<Cnt> const &bkr,
                                               Sels const &...sels)
    -> std::shared_ptr<booker<Cnt>> {
  // get a booker that has all the selections added
  auto bkr2 = bkr.select_counters(sels...);
  // add all the counters (each with one selection) into the experiment
  for (auto const &sel_path : bkr2->list_selection_paths()) {
    auto cnt = bkr2->get_counter(sel_path);
    cnt->apply_scale(m_norm);
    this->add_counter(*cnt);
  }
  return bkr2;
}