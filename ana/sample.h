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
  using dataset_type = T;
  // using reader_type = typename decltype(std::declval<T>().open_reader(std::declval<const table::range&>()))::element_type;
  using reader_type = table::read_t<T>;

public:
  sample(std::unique_ptr<T> dataset);
  template <typename... Args>
  sample(Args&&... args);
  virtual ~sample() = default;

  void open(long long max_entries=-1);
  void scale(double w);

  long long get_entries() const;
  double get_weight() const;

protected:
  double                          m_scale;
  std::unique_ptr<T>              m_dataset;
  table::partition                m_partition;
  concurrent<reader_type>         m_readers;
  concurrent<table::processor<reader_type>> m_processors;

};

}

template <typename T>
ana::sample<T>::sample(std::unique_ptr<T> dataset) :
  m_scale(1.0),
  m_dataset(std::move(dataset))
{}

template <typename T>
template <typename... Args>
ana::sample<T>::sample(Args&&... args) :
  m_scale(1.0),
  m_dataset(std::make_unique<T>(args...))
{}

template <typename T>
void ana::sample<T>::open(long long max_entries)
{
  // partition data
	m_partition = m_dataset->allocate().truncate(max_entries).merge(ana::multithread::concurrency());

  // normalize data
  m_scale /= m_dataset->normalize();

  // open readers & processors
  m_readers.clear();
  m_processors.clear();
  for (unsigned int islot=0 ; islot<m_partition.size() ; ++islot) {
    auto reader = m_dataset->open_reader(m_partition.part(islot));
    m_readers.add(reader);
    auto processor = std::make_shared<table::processor<reader_type>>(*reader,m_scale);
    m_processors.add(processor);
	}

}

template <typename T>
void ana::sample<T>::scale(double s)
{
  m_scale *= s;
}

template <typename T>
long long ana::sample<T>::get_entries() const
{
  return m_partition.total().entries();
}

template <typename T>
double ana::sample<T>::get_weight() const
{
  return m_scale;
}
