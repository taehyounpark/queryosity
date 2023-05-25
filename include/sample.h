#pragma once

#include "ana/concurrent.h"
#include "ana/input.h"
#include "ana/looper.h"

namespace ana
{

template <typename T>
class sample
{

public:
  using dataset_reader_type = read_dataset_t<T>;

public:
  sample();
  virtual ~sample() = default;

  template <typename... Args>
  void open(Args&&... args);

	sample(sample const&) = delete;
	sample& operator=(sample const&) = delete;

	sample(sample&&) = default;
	sample& operator=(sample&&) = default;

  void limit(long long max_entries=-1);
  void scale(double scale);

  long long get_limited_entries() const;
  double get_normalized_scale() const;

protected:
  void prepare();

protected:
  // open
  std::unique_ptr<T>                      m_dataset;
  // prepare
  bool                                    m_prepared;
  long long                               m_max_entries;
  double                                  m_scale;
  input::partition                        m_partition;
  concurrent<dataset_reader_type>         m_readers;
  concurrent<looper<dataset_reader_type>> m_loopers;

};

}

template <typename T>
ana::sample<T>::sample() :
  m_dataset(nullptr),
  m_prepared(false),
  m_max_entries(-1),
  m_scale(1.0)
{}

template <typename T>
template <typename... Args>
void ana::sample<T>::open(Args&&... args)
{
  if (m_dataset) throw std::logic_error("sample dataset already opened");
  m_dataset = std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T>
void ana::sample<T>::limit(long long max_entries)
{
  if (m_prepared) throw std::logic_error("sample dataset partition already allocated");
  m_max_entries = max_entries;
}

template <typename T>
void ana::sample<T>::scale(double scale)
{
  if (m_prepared) throw std::logic_error("sample dataset normalization already applied");
  m_scale *= scale;
}

template <typename T>
void ana::sample<T>::prepare()
{
  // can never be prepared twice
  if (m_prepared) return;

  // dataset must exist
  if (!m_dataset) throw std::runtime_error("no sample dataset opened");

  // 1. allocate the dataset partition
  // 2. truncate entries to the maximum
  // 3. downsize concurrency to the maximum 
	m_partition = m_dataset->allocate_partition().truncate(m_max_entries).merge(ana::multithread::concurrency());

  // calculate a normalization factor
  // scale the sample by its inverse
  m_scale /= m_dataset->normalize_scale();

  // open dataset reader and looper for each thread
  m_readers.clear();
  m_loopers.clear();
  for (unsigned int islot=0 ; islot<m_partition.size() ; ++islot) {
    auto rdr = m_dataset->read_dataset(m_partition.get_part(islot));
    auto lpr = std::make_shared<looper<dataset_reader_type>>(*rdr,m_scale);
    m_readers.add_slot(rdr);
    m_loopers.add_slot(lpr);
	}

  // sample is prepared
  m_prepared = true;
}

template <typename T>
long long ana::sample<T>::get_limited_entries() const
{
  this->prepare();
  return m_partition.total().entries();
}

template <typename T>
double ana::sample<T>::get_normalized_scale() const
{
  this->prepare();
  return m_scale;
}
