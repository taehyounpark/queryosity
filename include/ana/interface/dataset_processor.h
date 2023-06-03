#pragma once

#include "action.h"
#include "column_computation.h"
#include "counter_experiment.h"

namespace ana {

namespace dataset {

template <typename T>
class processor : public action,
                  public column::computation<T>,
                  public counter::experiment {

public:
  processor(const dataset::range &part, dataset::reader<T> &reader,
            double scale);
  virtual ~processor() = default;

public:
  virtual void initialize(const dataset::range &part) override;
  virtual void execute(const dataset::range &part,
                       unsigned long long entry) override;
  virtual void finalize(const dataset::range &part) override;

  void process();
};

} // namespace dataset

} // namespace ana

#include "counter.h"
#include "selection.h"

template <typename T>
ana::dataset::processor<T>::processor(const ana::dataset::range &part,
                                      dataset::reader<T> &reader, double scale)
    : action(), column::computation<T>(part, reader),
      counter::experiment(scale) {}

template <typename T>
void ana::dataset::processor<T>::initialize(const ana::dataset::range &part) {
  this->m_reader->start_part(this->m_part);

  for (auto const &col : this->m_columns) {
    col->initialize(part);
  }
  for (auto const &sel : this->m_selections) {
    sel->initialize(part);
  }
  for (auto const &cnt : this->m_counters) {
    cnt->initialize(part);
  }
}

template <typename T>
void ana::dataset::processor<T>::execute(const ana::dataset::range &part,
                                         unsigned long long entry) {
  this->m_reader->read_entry(part, entry);
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

template <typename T>
void ana::dataset::processor<T>::finalize(const ana::dataset::range &part) {
  for (auto const &col : this->m_columns) {
    col->finalize(part);
  }
  for (auto const &sel : this->m_selections) {
    sel->finalize(part);
  }
  for (auto const &cnt : this->m_counters) {
    cnt->finalize(part);
  }
  this->m_reader->finish_part(this->m_part);
}

template <typename T> void ana::dataset::processor<T>::process() {
  // start processing
  this->initialize(this->m_part);

  // processing
  for (unsigned long long entry = this->m_part.begin; entry < this->m_part.end;
       ++entry) {
    this->execute(this->m_part, entry);
  }

  // finish processing
  this->finalize(this->m_part);
}