#pragma once

#include <memory>
#include <vector>

#include "selection_cutflow.h"

#include "aggregation.h"
#include "aggregation_booker.h"

namespace ana {

class aggregation::experiment : public selection::cutflow {

public:
  experiment(double scale);
  ~experiment() = default;

public:
  template <typename Cnt, typename... Args>
  std::unique_ptr<booker<Cnt>> agg(Args &&...args);

  template <typename Cnt>
  auto select_aggregation(booker<Cnt> const &bkr, const selection &sel)
      -> std::unique_ptr<Cnt>;

  template <typename Cnt, typename... Sels>
  auto select_aggregations(booker<Cnt> const &bkr, Sels const &...sels)
      -> std::array<std::unique_ptr<Cnt>, sizeof...(Sels)>;

  void clear_aggregations();

protected:
  void add_aggregation(aggregation &cnt);

protected:
  std::vector<aggregation *> m_aggregations;
  const double m_scale;
};

} // namespace ana

inline ana::aggregation::experiment::experiment(double scale)
    : m_scale(scale) {}

inline void
ana::aggregation::experiment::add_aggregation(ana::aggregation &cnt) {
  m_aggregations.push_back(&cnt);
}

inline void ana::aggregation::experiment::clear_aggregations() {
  m_aggregations.clear();
}

template <typename Cnt, typename... Args>
std::unique_ptr<ana::aggregation::booker<Cnt>>
ana::aggregation::experiment::agg(Args &&...args) {
  auto bkr = std::make_unique<booker<Cnt>>(std::forward<Args>(args)...);
  return bkr;
}

template <typename Cnt>
auto ana::aggregation::experiment::select_aggregation(booker<Cnt> const &bkr,
                                                      const selection &sel)
    -> std::unique_ptr<Cnt> {
  auto cnt = bkr.select_aggregation(sel);
  cnt->apply_scale(m_scale);
  this->add_aggregation(*cnt);
  return cnt;
}

template <typename Cnt, typename... Sels>
auto ana::aggregation::experiment::select_aggregations(booker<Cnt> const &bkr,
                                                       Sels const &...sels)
    -> std::array<std::unique_ptr<Cnt>, sizeof...(Sels)> {

  return std::array<std::unique_ptr<Cnt>, sizeof...(Sels)>{
      this->select_aggregation(bkr, sels)...};
  // get a booker that has all the selections added
  // auto bkpr_and_cntrs = bkr.select_aggregations(sels...);

  // for (auto const &cntr : bkpr_and_cntrs.second) {
  //   this->add_aggregation(*cntr);
  // }

  // return std::move(bkpr_and_cntrs);
}