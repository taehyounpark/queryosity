#pragma once

#include <memory>
#include <vector>

#include "selection_cutflow.h"

#include "query.h"
#include "query_book.h"

namespace queryosity {

class query::experiment : public selection::cutflow {

public:
  experiment() = default;
  ~experiment() = default;

public:
  template <typename Qry, typename... Args>
  std::unique_ptr<query::book<Qry>> make(Args &&...args);

  template <typename Qry>
  auto book(query::book<Qry> const &bkr, const selection::node &sel) -> Qry *;

  void scale_current_queries(double scale);
  void clear_current_queries();

protected:
  template <typename Qry> auto add_query(std::unique_ptr<Qry> qry) -> Qry *;

protected:
  std::vector<std::unique_ptr<query::node>> m_query_history;
  std::vector<query::node *> m_queries;
};

} // namespace queryosity

inline void queryosity::query::experiment::clear_current_queries() {
  m_queries.clear();
}

inline void queryosity::query::experiment::scale_current_queries(double scale) {
  for (auto const &qry : m_queries) {
    qry->apply_scale(scale);
  }
}

template <typename Qry, typename... Args>
std::unique_ptr<queryosity::query::book<Qry>>
queryosity::query::experiment::make(Args &&...args) {
  auto bkr = std::make_unique<query::book<Qry>>(std::forward<Args>(args)...);
  return bkr;
}

template <typename Qry>
auto queryosity::query::experiment::book(query::book<Qry> const &bkr,
                                         const selection::node &sel) -> Qry * {
  auto qry = bkr.set_selection(sel);
  return this->add_query(std::move(qry));
}

template <typename Qry>
auto queryosity::query::experiment::add_query(std::unique_ptr<Qry> qry)
    -> Qry * {
  auto out = qry.get();
  m_queries.push_back(out);
  m_query_history.push_back(std::move(qry));
  return out;
}