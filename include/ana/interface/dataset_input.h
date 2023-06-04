#pragma once

#include "dataset.h"

namespace ana {
namespace dataset {

/**
 * @brief
 * @tparam T Input dataset type (CRTP: see above)
 * @details This class uses the [Curiously Recurring Template Parameter
 * (CRPT)](https://en.cppreference.com/w/cpp/language/crtp) idiom. A proper
 * implementation should be written as follows:
 * ```cpp
 * #include "ana/analysis.h"
 * class DataFormat : public ana::dataset::dataset<DataFormat> {};
 * ```
 */
template <typename T> class input {

private:
  input() = default;
  ~input() = default;
  friend T;

public:
  partition allocate_partition();
  double normalize_scale();
  decltype(auto) read_dataset() const;

  /**
   * @brief Allocate partition for multithreading.
   * @details This step **must** be implemented by the analyzer.
   * It is performed to scan the dataset and determine any partitioning of the
   * dataset as specfieid by the analyzer. Each range in the partition as
   * allocated by this function will be processed concurrently as allowed by
   * `ana::multithread::enable()`, provided that the implemented input reader
   * can access each dataset range from the dataset in a thread-safe manner.
   * This method is non-\a const to allow altering the logical state of the
   * dataset input itself, e.g. check file paths and filter out invalid/missing
   * files.
   */
  partition allocate();

  /**
   * @brief Normalize the dataset.
   * @details This step can be optionally done to determine a global
   * normalization factor applied to all selection in the analysis. By default,
   * normalization is not applied, i.e. is equal to 1. This method is non-\a
   * const to allow altering the logical state of the dataset input itself, e.g.
   * check file paths and filter out invalid/missing files.
   */
  double normalize();

  /**
   * @details Open the dataset reader used to iterate over entries of the
   * dataset.
   * @return The input reader.
   * @details **Important**: the return type is required to be a
   * `std::unique_ptr` of a valid `dataset::reader` implementation as required
   * for use in a `ana::concurrent` container. This method is \a const as it is
   * to be performed N times for each thread, and each subsequent call should
   * not alter the logical state of this class.
   */
  decltype(auto) read() const;

  void start_dataset();
  void finish_dataset();

  void start();
  void finish();
};

} // namespace dataset

} // namespace ana

#include "dataset_reader.h"

template <typename T>
ana::dataset::partition ana::dataset::input<T>::allocate_partition() {
  return this->allocate();
}

template <typename T>
ana::dataset::partition ana::dataset::input<T>::allocate() {
  return static_cast<T *>(this)->allocate();
}

template <typename T> double ana::dataset::input<T>::normalize_scale() {
  return static_cast<T *>(this)->normalize();
}

template <typename T> inline double ana::dataset::input<T>::normalize() {
  return 1.0;
}

template <typename T> void ana::dataset::input<T>::start_dataset() {
  static_cast<T *>(this)->start();
}

template <typename T> void ana::dataset::input<T>::finish_dataset() {
  static_cast<T *>(this)->finish();
}

template <typename T> void ana::dataset::input<T>::start() {
  // nothing to do (yet)
}

template <typename T> void ana::dataset::input<T>::finish() {
  // nothing to do (yet)
}

template <typename T>
decltype(auto) ana::dataset::input<T>::read_dataset() const {

  using result_type = decltype(static_cast<const T *>(this)->read());
  static_assert(is_unique_ptr_v<result_type>,
                "not a std::unique_ptr of ana::dataset::reader<T>");

  using reader_type = typename result_type::element_type;
  static_assert(std::is_base_of_v<dataset::reader<reader_type>, reader_type>,
                "not an implementation of ana::dataset::reader<T>");

  return this->read();
}

template <typename T> decltype(auto) ana::dataset::input<T>::read() const {
  return static_cast<const T *>(this)->read();
}
