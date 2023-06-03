#pragma once

#include <memory>
#include <vector>

#include "selection_cutflow.h"

#include "counter.h"
#include "counter_booker.h"
#include "counter_bookkeeper.h"

namespace ana {

class counter::experiment : public selection::cutflow {

public:
  experiment(double scale);
  ~experiment() = default;

public:
  template <typename Cnt, typename... Args>
  std::unique_ptr<booker<Cnt>> book(Args &&...args);

  template <typename Cnt>
  auto select_counter(booker<Cnt> const &bkr, const selection &sel)
      -> std::unique_ptr<Cnt>;

  template <typename Cnt, typename... Sels>
  auto select_counters(booker<Cnt> const &bkr, Sels const &...sels)
      -> std::pair<std::unique_ptr<bookkeeper<Cnt>>,
                   std::vector<std::unique_ptr<Cnt>>>;

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
std::unique_ptr<ana::counter::booker<Cnt>>
ana::counter::experiment::book(Args &&...args) {
  auto bkr = std::make_unique<booker<Cnt>>(std::forward<Args>(args)...);
  return std::move(bkr);
}

template <typename Cnt>
auto ana::counter::experiment::select_counter(booker<Cnt> const &bkr,
                                              const selection &sel)
    -> std::unique_ptr<Cnt> {
  auto cnt = bkr.select_counter(sel);
  cnt->apply_scale(m_norm);
  this->add_counter(*cnt);
  return std::move(cnt);
}

template <typename Cnt, typename... Sels>
auto ana::counter::experiment::select_counters(booker<Cnt> const &bkr,
                                               Sels const &...sels)
    -> std::pair<std::unique_ptr<bookkeeper<Cnt>>,
                 std::vector<std::unique_ptr<Cnt>>> {
  // get a booker that has all the selections added
  auto bkpr_and_cntrs = bkr.select_counters(sels...);

  for (auto const &cntr : bkpr_and_cntrs.second) {
    this->add_counter(*cntr);
  }

  return std::move(bkpr_and_cntrs);
}