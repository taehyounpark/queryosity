#pragma once

#include "column_computation.h"
#include "counter_experiment.h"

namespace ana {

namespace dataset {

class player : public ana::column::computation, public counter::experiment {

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

} // namespace ana

#include "dataset_source.h"

inline ana::dataset::player::player(dataset::source &ds, double scale)
    : counter::experiment(scale), m_ds(&ds) {}

inline void ana::dataset::player::play(unsigned int slot,
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
  for (auto cnt : m_counters) {
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
    for (auto cnt : m_counters) {
      cnt->execute(slot, entry);
    }
  }

  // finalize (in reverse order)
  for (auto cnt : m_counters) {
    cnt->finalize(slot);
  }
  for (auto sel : m_selections) {
    sel->finalize(slot);
  }
  for (auto col : m_columns) {
    col->finalize(slot);
  }
  m_ds->finalize(slot);

  // clear out counters (should not be re-played)
  m_counters.clear();
}