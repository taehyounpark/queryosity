#pragma once

#include <cassert>
#include <iterator>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

namespace ana {

template <typename T> class sample;

namespace input {

/**
 * @brief Range of a dataset to process by one thread slot.
*/
struct range {
  range(size_t slot, long long begin, long long end);
  ~range() = default;

  range operator+(const range &next);
  range &operator+=(const range &next);

  long long entries() const;

  /**
   * @brief Thread index that the processed range belongs to.
  */
  size_t slot;
  /**
   * @brief The first entry in the range
  */
  unsigned long long begin;
  /**
   * @brief The last entry+1 in the range
  */
  unsigned long long end;
};

/**
 * @brief Partition of a dataset.
 * @details A partition contains one or more ranges of the datset in sequential order of their entries.
 * They can dynamically truncated and merged according to the maximum limit of entries of the dataset and multithreading configuration as specified by the analyzer.
*/
struct partition {
  static std::vector<std::vector<ana::input::range>>
  group_parts(const std::vector<range> &parts, size_t n);
  static range sum_parts(const std::vector<range> &parts);

  partition() = default;
  ~partition() = default;

  /**
   * @brief Add a range to the partition.
   * @details The added range must satisfy that the `end` of the previous range (if it exists) is equal to the incoming `begin`.
   * Otherwise, the partition is considered to be in an invalid state, and `truncate()` and `merge()` operations respectively will fail assertions in place.
  */
  void add_part(size_t islot, long long begin, long long end);

  range get_part(size_t irange) const;
  range total() const;

  size_t size() const;

  void truncate(long long max_entries = -1);

  void merge(size_t max_parts);

  bool fixed;
  std::vector<range> parts;
};

/**
 * @brief 
 * @tparam T Input dataset format (CRTP: see above)
 * @details This class uses the [Curiously Recurring Template Parameter (CRPT)](https://en.cppreference.com/w/cpp/language/crtp) idiom.
 * A proper implementation should be written as follows:
 * ```cpp
 * #include "ana/abc.h"
 * class DataFormat : public ana::input::dataset<DataFormat> {};
 * ```
 */
template <typename T> class dataset {

public:
  dataset() = default;
  virtual ~dataset() = default;

  partition allocate_partition();
  double normalize_scale();
  decltype(auto) read_dataset() const;

  /**
   * @brief Allocate partition for multithreading.
   * @details This step **must** be implemented by the analyzer.
   * It is performed to scan the dataset and determine any partitioning of the dataset as specfieid by the analyzer.
   * If partitioning is perform to split the dataset, each range will be processed concurrently as allowed by `ana::multithread::enable();`.
   * Else, a partition over the entire dataset range will force the analysis to be performed sequentially.
   * The input reader (see `input::reader`) must support accessing each dataset range in the partition simultanesouly.
   * The method is non-\a const to allow the scan itself to alter the logical state of the dataset, e.g. check input file paths and remember only valid ones for use later.
  */
  partition allocate();

  /**
   * @brief Normalize the dataset.
   * @details This step is performed to scan the dataset and determine any normalization to be applied to the analysis.
   * By default, no normalization is applied (i.e. equal to 1.0).
   * The method is non-\a const to allow the scan itself to alter the logical state of the dataset, e.g. check input file paths and remember only valid ones for use later.
  */
  double normalize();

  /**
   * @details Open the dataset reader used to iterate over entries of the dataset.
   * @return The input reader.
   * @details **Important**: the return type is required to be a `std::shared_ptr` of a valid `input::reader` implementation as required for use in a `ana::concurrent` container.
   * This method is \a const as it is to be performed N times for each thread, and each subsequent call should not alter the logical state of this class.
  */
  decltype(auto) read() const;

  void start_dataset();
  void finish_dataset();

  void start();
  void finish();
};

template <typename T> class reader {

public:
public:
  reader() = default;
  ~reader() = default;

  // read a column of a data type with given name
  template <typename Val>
  decltype(auto) read_column(const range &part, const std::string &name) const;

  void start_part(const range &part);
  void read_entry(const range &part, unsigned long long entry);
  void finish_part(const range &part);
};

} // namespace input

template <typename T>
using read_dataset_t =
    typename decltype(std::declval<T const &>().read_dataset())::element_type;

template <typename T, typename Val>
using read_column_t =
    typename decltype(std::declval<T const &>().template read_column<Val>(
        std::declval<input::range const &>(),
        std::declval<std::string const &>()))::element_type;

template <typename T> struct is_shared_ptr : std::false_type {};
template <typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};
template <typename T>
static constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

} // namespace ana

#include "column.h"

inline ana::input::range::range(size_t slot, long long begin, long long end)
    : slot(slot), begin(begin), end(end) {}

inline long long ana::input::range::entries() const {
  assert(this->end > this->begin);
  return end - begin;
}

inline ana::input::range ana::input::range::operator+(const range &next) {
  assert(this->end == next.begin);
  return range(this->slot, this->begin, next.end);
}

inline ana::input::range &ana::input::range::operator+=(const range &next) {
  assert(this->end == next.begin);
  this->end = next.end;
  return *this;
}

inline std::vector<std::vector<ana::input::range>>
ana::input::partition::group_parts(const std::vector<range> &parts, size_t n) {
  std::vector<std::vector<range>> grouped_parts;
  size_t length = parts.size() / n;
  size_t remain = parts.size() % n;
  size_t begin = 0;
  size_t end = 0;
  for (size_t i = 0; i < std::min(n, parts.size()); ++i) {
    end += (remain > 0) ? (length + !!(remain--)) : length;
    grouped_parts.push_back(
        std::vector<range>(parts.begin() + begin, parts.begin() + end));
    begin = end;
  }
  return grouped_parts;
}

inline ana::input::range
ana::input::partition::sum_parts(const std::vector<range> &parts) {
  return std::accumulate(std::next(parts.begin()), parts.end(), parts.front());
}

inline void ana::input::partition::add_part(size_t islot, long long begin,
                                            long long end) {
  this->parts.push_back(range(islot, begin, end));
}

inline ana::input::range ana::input::partition::get_part(size_t islot) const {
  return this->parts[islot];
}

inline ana::input::range ana::input::partition::total() const {
  return sum_parts(this->parts);
}

inline size_t ana::input::partition::size() const { return this->parts.size(); }

inline void ana::input::partition::merge(size_t max_parts) {
  if (fixed)
    return;
  partition merged;
  auto groups = group_parts(this->parts, max_parts);
  for (const auto &group : groups) {
    merged.parts.push_back(sum_parts(group));
  }
  this->parts.clear();
  for (const auto &group : groups) {
    this->parts.push_back(sum_parts(group));
  }
}

inline void ana::input::partition::truncate(long long max_entries) {
  if (fixed)
    return;
  if (max_entries < 0)
    return;
  // remember the full parts
  auto full_parts = this->parts;
  // clear the parts to be added anew
  this->parts.clear();
  for (const auto &part : full_parts) {
    auto part_end = max_entries >= 0
                        ? std::min(part.begin + max_entries, part.end)
                        : part.end;
    this->parts.push_back(range(part.slot, part.begin, part_end));
    max_entries -= part_end;
    if (!max_entries)
      break;
  }
}

template <typename T>
ana::input::partition ana::input::dataset<T>::allocate_partition() {
  return this->allocate();
}

template <typename T>
ana::input::partition ana::input::dataset<T>::allocate() {
  return static_cast<T*>(this)->allocate();
}

template <typename T> double ana::input::dataset<T>::normalize_scale() {
  return static_cast<T *>(this)->normalize();
}

template <typename T> inline double ana::input::dataset<T>::normalize() {
  return 1.0;
}

template <typename T> void ana::input::dataset<T>::start_dataset() {
  static_cast<T *>(this)->start();
}

template <typename T> void ana::input::dataset<T>::finish_dataset() {
  static_cast<T *>(this)->finish();
}

template <typename T> void ana::input::dataset<T>::start() {
  // nothing to do (yet)
}

template <typename T> void ana::input::dataset<T>::finish() {
  // nothing to do (yet)
}

template <typename T>
decltype(auto) ana::input::dataset<T>::read_dataset() const {

  using result_type = decltype(static_cast<const T *>(this)->read());
  static_assert(is_shared_ptr_v<result_type>,
                "not a std::shared_ptr of ana::input::reader<T>");

  using reader_type = typename result_type::element_type;
  static_assert(std::is_base_of_v<input::reader<reader_type>, reader_type>,
                "not an implementation of ana::input::reader<T>");

  return this->read();
}

template <typename T>
decltype(auto) ana::input::dataset<T>::read() const {
  return static_cast<const T *>(this)->read();
}

template <typename T>
template <typename Val>
decltype(auto)
ana::input::reader<T>::read_column(const range &part,
                                   const std::string &name) const {

  using result_type =
      decltype(static_cast<const T *>(this)->template read<Val>(part, name));
  static_assert(is_shared_ptr_v<result_type>,
                "must be a std::shared_ptr of ana::column::reader<T>");

  return static_cast<const T *>(this)->template read<Val>(part, name);
}

template <typename T>
void ana::input::reader<T>::start_part(const ana::input::range &part) {
  static_cast<T *>(this)->start(part);
}

template <typename T>
void ana::input::reader<T>::read_entry(const ana::input::range &part,
                                       unsigned long long entry) {
  static_cast<T *>(this)->next(part, entry);
}

template <typename T>
void ana::input::reader<T>::finish_part(const ana::input::range &part) {
  static_cast<T *>(this)->finish(part);
}