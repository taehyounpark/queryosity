#pragma once

#include <iostream>
#include <memory>

#include "dataset_processor.h"
#include "dataset_reader.h"
#include "multithread.h"

namespace ana {

template <typename T> class sample {

public:
  using dataset_reader_type = read_dataset_t<T>;
  using dataset_processor_type =
      typename dataset::processor<dataset_reader_type>;

public:
  sample();
  virtual ~sample() = default;

  sample(sample const &) = delete;
  sample &operator=(sample const &) = delete;

  sample(sample &&) = default;
  sample &operator=(sample &&) = default;

  void limit_entries(long long max_entries = -1);
  void scale_weights(double scale);
  unsigned long long get_processed_entries() const;
  double get_normalized_scale() const;

protected:
  template <typename... Args> void prepare(Args &&...args);
  void initialize();

protected:
  std::unique_ptr<T> m_dataset; //!
  bool m_initialized;

  long long m_max_entries;
  double m_scale;

  dataset::partition m_partition;
  concurrent<dataset_reader_type> m_readers;       //!
  concurrent<dataset_processor_type> m_processors; //!
};

} // namespace ana

template <typename T>
ana::sample<T>::sample()
    : m_dataset(nullptr), m_initialized(false), m_max_entries(-1),
      m_scale(1.0) {}

template <typename T>
template <typename... Args>
void ana::sample<T>::prepare(Args &&...args) {
  if (m_dataset)
    throw std::logic_error("dataset already prepared");
  m_dataset = std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T>
void ana::sample<T>::limit_entries(long long max_entries) {
  if (m_initialized)
    throw std::logic_error("sample already initialized");
  m_max_entries = max_entries;
}

template <typename T> void ana::sample<T>::scale_weights(double scale) {
  if (m_initialized)
    throw std::logic_error("sample already initialized");
  m_scale *= scale;
}

template <typename T> void ana::sample<T>::initialize() {
  // can never be initialized twice
  if (m_initialized)
    return;

  // dataset must exist
  if (!m_dataset)
    throw std::runtime_error("no sample dataset opened");

  // 1. allocate the dataset partition
  m_partition = m_dataset->allocate_partition();
  // 2. truncate entries to limit
  m_partition.truncate(m_max_entries);
  // 3. merge parts to concurrency limit
  m_partition.merge(ana::multithread::concurrency());

  // calculate a normalization factor
  // scale the sample by its inverse
  m_scale /= m_dataset->normalize_scale();

  // open dataset reader and processor for each thread
  // model reprents whole dataset
  auto part = m_partition.total();
  auto rdr = m_dataset->read_dataset();
  auto proc = std::make_unique<dataset_processor_type>(part, *rdr, m_scale);
  m_readers.set_model(std::move(rdr));
  m_processors.set_model(std::move(proc));
  // slot for each partition range
  m_readers.clear_slots();
  m_processors.clear_slots();
  for (unsigned int islot = 0; islot < m_partition.size(); ++islot) {
    auto part = m_partition.get_part(islot);
    auto rdr = m_dataset->read_dataset();
    auto proc = std::make_unique<dataset_processor_type>(part, *rdr, m_scale);
    m_readers.add_slot(std::move(rdr));
    m_processors.add_slot(std::move(proc));
  }

  // sample is initialized
  m_initialized = true;
}

template <typename T>
unsigned long long ana::sample<T>::get_processed_entries() const {
  this->initialize();
  return m_partition.total().entries();
}

template <typename T> double ana::sample<T>::get_normalized_scale() const {
  this->initialize();
  return m_scale;
}
