#pragma once

#include "column_computation.h"
#include "counter_experiment.h"
#include "operation.h"

namespace ana {

namespace dataset {

class processor : public ana::column::computation, public counter::experiment {

public:
  processor(double scale);
  virtual ~processor() = default;

public:
  void process(player &player, range const &part);
};

} // namespace dataset

} // namespace ana

#include "counter.h"
#include "selection.h"

inline ana::dataset::processor::processor(double scale)
    : counter::experiment(scale) {}

inline void ana::dataset::processor::process(ana::dataset::player &plyr,
                                             ana::dataset::range const &part) {

  // initialize
  plyr.initialize(part);
  for (auto const &col : this->m_columns) {
    col->initialize(part);
  }
  for (auto const &sel : this->m_selections) {
    sel->initialize(part);
  }
  for (auto const &cnt : this->m_counters) {
    cnt->initialize(part);
  }

  // execute
  for (unsigned long long entry = part.begin; entry < part.end; ++entry) {
    plyr.execute(part, entry);
    for (auto const &col : this->m_columns) {
      col->execute(part, entry);
    }
    for (auto const &sel : this->m_selections) {
      sel->execute(part, entry);
    }
    for (auto const &cnt : this->m_counters) {
      cnt->execute(part, entry);
    }
  }

  // finalize (in reverse order)
  for (auto const &cnt : this->m_counters) {
    cnt->finalize(part);
  }
  for (auto const &sel : this->m_selections) {
    sel->finalize(part);
  }
  for (auto const &col : this->m_columns) {
    col->finalize(part);
  }
  plyr.finalize(part);
}