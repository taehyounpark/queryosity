#pragma once

#include <memory>
#include <vector>

#include "selection_cutflow.h"

#include "query.h"
#include "query_book.h"

namespace queryosity {

class query::experiment : public selection::cutflow {

public:
  experiment(double scale);
  ~experiment() = default;

public:
  template <typename Cnt, typename... Args>
  std::unique_ptr<query::book<Cnt>> agg(Args &&...args);

  template <typename Cnt>
  auto book(query::book<Cnt> const &bkr, const selection::node &sel)
      -> std::unique_ptr<Cnt>;

  void clear_querys();

protected:
  void add_query(query::node &cnt);

protected:
  std::vector<query::node *> m_querys;
  const double m_scale;
};

} // namespace queryosity

inline queryosity::query::experiment::experiment(double scale)
    : m_scale(scale) {}

inline void
queryosity::query::experiment::add_query(queryosity::query::node &cnt) {
  m_querys.push_back(&cnt);
}

inline void queryosity::query::experiment::clear_querys() { m_querys.clear(); }

template <typename Cnt, typename... Args>
std::unique_ptr<queryosity::query::book<Cnt>>
queryosity::query::experiment::agg(Args &&...args) {
  auto bkr = std::make_unique<query::book<Cnt>>(std::forward<Args>(args)...);
  return bkr;
}

template <typename Cnt>
auto queryosity::query::experiment::book(query::book<Cnt> const &bkr,
                                         const selection::node &sel)
    -> std::unique_ptr<Cnt> {
  auto cnt = bkr.set_selection(sel);
  cnt->apply_scale(m_scale);
  this->add_query(*cnt);
  return cnt;
}