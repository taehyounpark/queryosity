#pragma once

#include <memory>
#include <string>
#include <vector>

#include "action.h"
#include "computation.h"
#include "experiment.h"

namespace ana {

template <typename T>
class processor : public action,
                  public column::computation<T>,
                  public counter::experiment {

public:
  processor(const input::range &part, input::reader<T> &reader, double scale);
  virtual ~processor() = default;

public:
  virtual void initialize() override;
  virtual void execute() override;
  virtual void finalize() override;

  void process();
};

} // namespace ana
#include "counter.h"
#include "selection.h"

template <typename T>
ana::processor<T>::processor(const input::range &part, input::reader<T> &reader,
                             double scale)
    : action(), column::computation<T>(part, reader), counter::experiment(
                                                          scale) {}

template <typename T> void ana::processor<T>::initialize() {
  for (auto const &col : this->m_columns) {
    col->initialize();
  }
  for (auto const &sel : this->m_selections) {
    sel->initialize();
  }
  for (auto const &cnt : this->m_counters) {
    cnt->initialize();
  }
}

template <typename T> void ana::processor<T>::execute() {
  for (auto const &col : this->m_columns) {
    col->execute();
  }
  for (auto const &sel : this->m_selections) {
    sel->execute();
  }
  for (auto const &cnt : this->m_counters) {
    cnt->execute();
  }
}

template <typename T> void ana::processor<T>::finalize() {
  for (auto const &col : this->m_columns) {
    col->finalize();
  }
  for (auto const &sel : this->m_selections) {
    sel->finalize();
  }
  for (auto const &cnt : this->m_counters) {
    cnt->finalize();
  }
}

template <typename T> void ana::processor<T>::process() {
  // start
  this->m_reader->start_part(this->m_part);
  this->initialize();

  // per-entry
  for (unsigned long long entry = this->m_part.begin; entry < this->m_part.end;
       ++entry) {
    this->m_reader->read_entry(this->m_part, entry);
    this->execute();
  }

  // finish
  this->finalize();
  this->m_reader->finish_part(this->m_part);
}