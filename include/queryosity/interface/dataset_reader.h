#pragma once

#include "action.h"
#include "column.h"

namespace queryosity {

namespace dataset {

/**
 * @ingroup abc
 * @brief Custom dataset source
 */
class source : public action {
public:
  source() = default;
  virtual ~source() = default;

  virtual void parallelize(unsigned int concurrency) = 0;

  virtual double normalize();
  virtual void initialize();

  /**
   * @brief Determine dataset partition for parallel processing.
   * @return Dataset entry partition
   *
   * @details
   *
   * - The dataflow *must* load at least one dataset with a valid partition.
   * - A valid partition *must* begin at 0 and be in sorted contiguous order,
   * e.g. `{{0,100},{100,200}}`.
   * - If a dataset returns an empty partition, it relinquishes the control to
   * another dataset in the dataflow.
   * @attention The empty-partition dataset *must* be able to fulfill
   * @code{.cpp}execute(entry)@endcode calls for any `entry` as
   * requested by the other datasets in the dataflow.
   *
   * Valid partitions reported by loaded datasets undergo the following changes:
   * 1. A common alignment partition is calculated across all loaded datasets.
   * @attention All non-empty partitions in the dataflow *must* have the same
   * total number of entries in order them to be alignable.
   * 2. Entries past the maximum to be processed are truncated.
   * 3. Neighbouring ranges are merged to match thread concurrency.
   *
   */
  virtual std::vector<std::pair<unsigned long long, unsigned long long>>
  partition() = 0;

  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) override;
  virtual void execute(unsigned int slot, unsigned long long entry) override;
  virtual void finalize(unsigned int slot) override;
  virtual void finalize();
};

/**
 * @ingroup abc
 * @brief Custom dataset reader
 */
template <typename DS> class reader : public source {

public:
  virtual ~reader() = default;

  template <typename Val>
  std::unique_ptr<queryosity::column::reader<Val>>
  read_column(unsigned int slot, const std::string &name);

  template <typename Val>
  std::unique_ptr<queryosity::column::reader<Val>>
  read(unsigned int slot, const std::string &name);
};

} // namespace dataset

} // namespace queryosity

template <typename T, typename Val>
using read_column_t =
    typename decltype(std::declval<T>().template read_column<Val>(
        std::declval<unsigned int>(),
        std::declval<std::string const &>()))::element_type;

#include "column_reader.h"

inline double queryosity::dataset::source::normalize() { return 1.0; }

inline void queryosity::dataset::source::initialize() {}

inline void queryosity::dataset::source::initialize(unsigned int,
                                                    unsigned long long,
                                                    unsigned long long) {}

inline void queryosity::dataset::source::execute(unsigned int,
                                                 unsigned long long) {}

inline void queryosity::dataset::source::finalize(unsigned int) {}

inline void queryosity::dataset::source::finalize() {}

template <typename DS>
template <typename Val>
std::unique_ptr<queryosity::column::reader<Val>>
queryosity::dataset::reader<DS>::read_column(unsigned int slot,
                                             const std::string &name) {
  auto col_rdr = static_cast<DS *>(this)->template read<Val>(slot, name);
  if (!col_rdr)
    throw std::runtime_error("dataset column cannot be read");
  return col_rdr;
}

template <typename DS>
template <typename Val>
std::unique_ptr<queryosity::column::reader<Val>>
queryosity::dataset::reader<DS>::read(unsigned int, const std::string &) {
  return nullptr;
}