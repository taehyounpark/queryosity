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
            slot_t slot, std::vector<part_t> const &parts);
};

} // namespace dataset

} // namespace queryosity

#include "dataset_reader.h"

inline void queryosity::dataset::player::play(
    std::vector<std::unique_ptr<source>> const &sources, double scale,
    slot_t slot, std::vector<part_t> const &parts) {

  // apply dataset scale in effect for all queries
  for (auto const &qry : m_queries) {
    qry->apply_scale(scale);
  }

  // traverse each part
  for (auto const &part : parts) {
    // initialize
    for (auto const &ds : sources) {
      ds->initialize(slot, part.first, part.second);
    }
    for (auto const &col : m_columns) {
      col->initialize(slot, part.first, part.second);
    }
    for (auto const &sel : m_selections) {
      sel->initialize(slot, part.first, part.second);
    }
    for (auto const &qry : m_queries) {
      qry->initialize(slot, part.first, part.second);
    }
    // execute
    for (auto entry = part.first; entry < part.second; ++entry) {
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
  }

  // clear out queries (should not be re-played)
  m_queries.clear();
}