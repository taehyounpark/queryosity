#pragma once

#include "column_computation.h"
#include "query_experiment.h"

namespace queryosity {

namespace dataset {

class player : public query::experiment {

public:
  player() = default;
  virtual ~player() = default;

public:
  void play(std::vector<std::unique_ptr<source>> const &sources, double scale,
            unsigned int slot, unsigned long long begin,
            unsigned long long end);
};

} // namespace dataset

} // namespace queryosity

#include "dataset_reader.h"

inline void queryosity::dataset::player::play(
    std::vector<std::unique_ptr<source>> const &sources, double scale,
    unsigned int slot, unsigned long long begin, unsigned long long end) {

  for (auto const &qry : m_queries) {
    qry->apply_scale(scale);
  }

  // initialize
  for (auto const &ds : sources) {
    ds->initialize(slot, begin, end);
  }
  for (auto const &col : m_columns) {
    col->initialize(slot, begin, end);
  }
  for (auto const &sel : m_selections) {
    sel->initialize(slot, begin, end);
  }
  for (auto const &qry : m_queries) {
    qry->initialize(slot, begin, end);
  }

  // execute
  for (auto entry = begin; entry < end; ++entry) {
    for (auto const &ds : sources) {
      ds->execute(slot, entry);
    }
    for (auto const &col : m_columns) {
      col->execute(slot, entry);
    }
    for (auto const &sel : m_selections) {
      sel->execute(slot, entry);
    }
    for (auto const &qry : m_queries) {
      qry->execute(slot, entry);
    }
  }

  // finalize (in reverse order)
  for (auto const &qry : m_queries) {
    qry->finalize(slot);
  }
  for (auto const &sel : m_selections) {
    sel->finalize(slot);
  }
  for (auto const &col : m_columns) {
    col->finalize(slot);
  }
  for (auto const &ds : sources) {
    ds->finalize(slot);
  }

  // clear out queries (should not be re-played)
  m_queries.clear();
}