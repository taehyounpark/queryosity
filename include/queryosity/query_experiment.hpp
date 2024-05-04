#pragma once

#include <memory>
#include <vector>

#include "selection_cutflow.hpp"

#include "query.hpp"
#include "query_booker.hpp"

namespace queryosity {

class query::experiment : public selection::cutflow {

public:
  experiment() = default;
  ~experiment() = default;

public:
  template <typename Qry, typename... Args>
  std::unique_ptr<query::booker<Qry>> make(Args &&...args);

  template <typename Qry>
  auto book(query::booker<Qry> const &bkr, const selection::node &sel) -> Qry *;

protected:
  template <typename Qry> auto add_query(std::unique_ptr<Qry> qry) -> Qry *;

protected:
  std::vector<query::node *> m_queries;
  std::vector<std::unique_ptr<query::node>> m_queries_history;
};

} // namespace queryosity

template <typename Qry, typename... Args>
std::unique_ptr<queryosity::query::booker<Qry>>
queryosity::query::experiment::make(Args &&...args) {
  auto bkr = std::make_unique<query::booker<Qry>>(std::forward<Args>(args)...);
  return bkr;
}

template <typename Qry>
auto queryosity::query::experiment::book(query::booker<Qry> const &bkr,
                                         const selection::node &sel) -> Qry * {
  auto qry = bkr.set_selection(sel);
  return this->add_query(std::move(qry));
}

template <typename Qry>
auto queryosity::query::experiment::add_query(std::unique_ptr<Qry> qry)
    -> Qry * {
  auto out = qry.get();
  m_queries_history.push_back(std::move(qry));
  m_queries.push_back(m_queries_history.back().get());
  return out;
}