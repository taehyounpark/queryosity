#pragma once

#include "column_computation.h"
#include "query_experiment.h"

namespace queryosity {

namespace dataset {

class player : public queryosity::column::computation,
               public query::experiment {

public:
  player(source &ds, double scale);
  virtual ~player() = default;

public:
  void play(unsigned int slot, unsigned long long begin,
            unsigned long long end);

protected:
  source *m_ds;
};

} // namespace dataset

} // namespace queryosity

#include "dataset_reader.h"

inline queryosity::dataset::player::player(dataset::source &ds, double scale)
    : query::experiment(scale), m_ds(&ds) {}

inline void queryosity::dataset::player::play(unsigned int slot,
                                              unsigned long long begin,
                                              unsigned long long end) {

  // initialize
  m_ds->initialize(slot, begin, end);
  for (auto col : m_columns) {
    col->initialize(slot, begin, end);
  }
  for (auto sel : m_selections) {
    sel->initialize(slot, begin, end);
  }
  for (auto cnt : m_querys) {
    cnt->initialize(slot, begin, end);
  }

  // execute
  for (auto entry = begin; entry < end; ++entry) {
    m_ds->execute(slot, entry);
    for (auto col : m_columns) {
      col->execute(slot, entry);
    }
    for (auto sel : m_selections) {
      sel->execute(slot, entry);
    }
    for (auto cnt : m_querys) {
      cnt->execute(slot, entry);
    }
  }

  // finalize (in reverse order)
  for (auto cnt : m_querys) {
    cnt->finalize(slot);
  }
  for (auto sel : m_selections) {
    sel->finalize(slot);
  }
  for (auto col : m_columns) {
    col->finalize(slot);
  }
  m_ds->finalize(slot);

  // clear out querys (should not be re-played)
  m_querys.clear();
}