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

  /**
   * @brief Inform the dataset of parallelism.
   */
  virtual void parallelize(unsigned int concurrency) = 0;

  /**
   * @brief Initialize dataset processing
   */
  virtual void initialize();

  /**
   * @brief Determine dataset partition for parallel processing.
   * @return Dataset partition
   *
   * @details
   * A non-empty partition **MUST** begin from the `0` and be sorted contiguous
   * order, e.g.:
   * @code{.cpp} {{0,100},{100,200}, ..., {900,1000}} @endcode
   * If a dataset returns an empty partition, it relinquishes its control over
   * the entry loop to another dataset with a non-empty partition.
   * @attention
   * - Non-empty partitions reported from multiple datasets need to be aligned
   * to form a common denominator partition over which the dataset processing is
   * parallelized. As such, they **MUST** have (1) at minimum, the same total
   * number of entries, and (2) ideally, shared sub-range boundaries.
   * - Any dataset reporting an empty partition **MUST** be able to fulfill
   * `dataset::source::execute()` calls for any entry number as requested by the
   * other datasets loaded in the dataflow.
   *
   */
  virtual std::vector<std::pair<unsigned long long, unsigned long long>>
  partition() = 0;

  /**
   * @brief Enter an entry loop.
   * @param[in] slot Thread slot number.
   * @param[in] begin First entry number processed.
   * @param[in] end Loop stops after `end-1`-th entry has been processed.
   */
  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) override;

  /**
   * @brief Process an entry.
   * @param[in] slot Thread slot number.
   * @param[in] entry Entry being processed.
   */
  virtual void execute(unsigned int slot, unsigned long long entry) override;

  /**
   * @brief Exit an entry loop.
   * @param[in] slot Thread slot number.
   * @param[in] entry Entry being processed.
   */
  virtual void finalize(unsigned int slot) override;

  /**
   * @brief Finalize processing the dataset.
   */
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

  /**
   * @brief Read a column.
   * @tparam Val Column value type.
   * @param slot Thread slot number.
   * @param name Column name.
   * @return Column implementation.
   */
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