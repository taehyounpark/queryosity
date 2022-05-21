#pragma once

#include "ana/concurrent.h"
#include "ana/table.h"
#include "ana/processor.h"

namespace ana
{

template <typename T>
class sample
{

public:
  // using reader_type = decltype(std::declval<T>().open_range(std::declval<const ana::table::range&>()));
  using reader_type = typename decltype(std::declval<T>().open_range())::element_type;

public:
  sample(long long maxEntries=-1);
  virtual ~sample() = default;

  void open(std::unique_ptr<T> data);

  template <typename... Args>
  void open(Args... args);

  void scale(double w);

  const table::partition& getpartition() const;
  long long getEntries() const;
  double get_weight() const;

private:
  void prepare(long long maxEntries=-1);

protected:
  long long                       m_maxEntries;
  double                          m_scale;
  std::unique_ptr<T>              m_dataset;
  table::partition                m_partition;
  concurrent<reader_type>         m_dataRanges;
  concurrent<table::processor<T>> m_dataprocessors;

};

}

template <typename T>
ana::sample<T>::sample(long long maxEntries) :
  m_maxEntries(maxEntries),
  m_scale(1.0)
{}

template <typename T>
void ana::sample<T>::open(std::unique_ptr<T> data)
{
  m_dataset = std::move(data);

  // partition data
	m_partition = m_dataset->partition_data().truncate(m_maxEntries).merge(ana::multithread::concurrency());

  // normalize data
  m_scale /= m_dataset->normalize_data();

  // data ranges
  m_dataRanges.clear();
  for (unsigned int slot=0 ; slot<m_partition.size() ; ++slot) {
    m_dataRanges.add(m_dataset->open_range());
	}

  // data range processors
  m_dataprocessors.clear();
  for (size_t slot=0 ; slot<m_partition.size(); ++slot) {
    m_dataprocessors.add(std::make_shared<table::processor<T>>(m_dataRanges.slot(slot),m_scale));
  }
}

template <typename T>
template <typename... Args>
void ana::sample<T>::open(Args... args)
{
  m_dataset = std::make_unique<T>(args...);

  // partition data
	m_partition = m_dataset->partition_data().truncate(m_maxEntries).merge(ana::multithread::concurrency());

  // normalize data
  m_scale /= m_dataset->normalize_data();

  // data ranges
  m_dataRanges.clear();
  for (unsigned int slot=0 ; slot<m_partition.size() ; ++slot) {
    auto dataRange = m_dataset->open_range();
    m_dataRanges.add(dataRange);
	}

  // data range processors
  m_dataprocessors.clear();
  for (size_t slot=0 ; slot<m_partition.size(); ++slot) {
    m_dataprocessors.add(std::make_shared<table::processor<T>>(m_dataRanges.slot(slot),m_scale));
  }
}

template <typename T>
void ana::sample<T>::scale(double s)
{
  m_scale *= s;
}

template <typename T>
const ana::table::partition& ana::sample<T>::getpartition() const
{
  return m_partition;
}

template <typename T>
long long ana::sample<T>::getEntries() const
{
  return m_partition.total().entries();
}

template <typename T>
double ana::sample<T>::get_weight() const
{
  return m_scale;
}
