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
  sample(long long max_entries=-1);
  virtual ~sample() = default;

  // general case (cannot handle arguments with initializer braces)
  template <typename... Args>
  void open(Args&&... args);

  // shortcuts for file paths provided with initializer braces
  template <typename U = T, typename std::enable_if_t<std::is_constructible_v<U,std::string,std::initializer_list<std::string>>, U>* = nullptr>
  void open(const std::string& key, std::initializer_list<std::string> file_paths);
  // shortcuts for file paths provided with initializer braces
  template <typename U = T, typename std::enable_if_t<std::is_constructible_v<U,std::initializer_list<std::string>,std::string>, U>* = nullptr>
  void open(std::initializer_list<std::string> file_paths, const std::string& key);

  template <typename... Args>
  void prepare(std::unique_ptr<T> dataset);

  void scale(double w);

  long long get_entries() const;
  double get_weight() const;

protected:
  long long                                  m_max_entries;
  double                                     m_scale;
  std::unique_ptr<T>                         m_dataset;
  input::partition                           m_partition;
  concurrent<dataset_reader_type>            m_readers;
  concurrent<looper<dataset_reader_type>> m_loopers;

};

}

template <typename T>
ana::sample<T>::sample(long long max_entries) :
  m_max_entries(max_entries),
  m_scale(1.0)
{}

template <typename T>
template <typename... Args>
void ana::sample<T>::open(Args&&... args)
// make the dataset according to user implementation
{
  this->prepare(std::make_unique<T>(std::forward<Args>(args)...));
}

template <typename T>
template <typename U, typename std::enable_if_t<std::is_constructible_v<U,std::string,std::initializer_list<std::string>>, U>* ptr >
void ana::sample<T>::open(const std::string& key, std::initializer_list<std::string> file_paths)
{
  this->prepare(std::make_unique<T>(key, file_paths));
}

template <typename T>
template <typename U, typename std::enable_if_t<std::is_constructible_v<U,std::initializer_list<std::string>,std::string>, U>* ptr >
void ana::sample<T>::open(std::initializer_list<std::string> file_paths, const std::string& key)
{
  this->prepare(std::make_unique<T>(file_paths, key));
}

template <typename T>
template <typename... Args>
void ana::sample<T>::prepare(std::unique_ptr<T> dataset)
{
  m_dataset = std::move(dataset);

  // first, allocate the dataset partition according to user implementation
  // then, truncate to the maximum requested entries
  // finally, downsize to the maximum requested concurrency
	m_partition = m_dataset->allocate_partition().truncate(m_max_entries).merge(ana::multithread::concurrency());

  // calculate a normalization factor according to user implementation
  // globally scale the sample by the inverse
  m_scale /= m_dataset->normalize_scale();

  // open the dataset reader and looper for each available thread
  m_readers.clear();
  m_loopers.clear();
  for (unsigned int islot=0 ; islot<m_partition.size() ; ++islot) {
    auto rdr = m_dataset->read_dataset(m_partition.get_part(islot));
    m_readers.add_slot(rdr);
    auto lpr = std::make_shared<looper<dataset_reader_type>>(*rdr,m_scale);
    m_loopers.add_slot(lpr);
	}

  // done -- sample is opened and ready for analysis
  return;
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
