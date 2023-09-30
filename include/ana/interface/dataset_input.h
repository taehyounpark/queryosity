#pragma once

#include "dataset.h"

namespace ana {
namespace dataset {

template <typename DS> class input {

public:
  partition allocate_partition();
  double normalize_scale();
  void initialize_dataset();
  void finalize_dataset();

  decltype(auto) open_rows(const range &part);

  template <typename Val>
  decltype(auto) read_column(const ana::dataset::range &part,
                             const std::string &name);

  std::unique_ptr<ana::dataset::row> open(const ana::dataset::range &part);

  partition allocate();
  double normalize();
  void initialize();
  void finalize();
};

} // namespace dataset

} // namespace ana

#include "dataset_row.h"

template <typename DS>
ana::dataset::partition ana::dataset::input<DS>::allocate_partition() {
  return this->allocate();
}

template <typename DS>
ana::dataset::partition ana::dataset::input<DS>::allocate() {
  return static_cast<DS *>(this)->allocate();
}

template <typename DS> double ana::dataset::input<DS>::normalize_scale() {
  return static_cast<DS *>(this)->normalize();
}

template <typename DS> inline double ana::dataset::input<DS>::normalize() {
  return 1.0;
}

template <typename DS> void ana::dataset::input<DS>::initialize_dataset() {
  static_cast<DS *>(this)->initialize();
}

template <typename DS> void ana::dataset::input<DS>::finalize_dataset() {
  static_cast<DS *>(this)->finalize();
}

template <typename DS> void ana::dataset::input<DS>::initialize() {
  // nothing to do (yet)
}

template <typename DS> void ana::dataset::input<DS>::finalize() {
  // nothing to do (yet)
}

template <typename DS>
decltype(auto)
ana::dataset::input<DS>::open_rows(const ana::dataset::range &part) {

  using result_type = decltype(static_cast<DS *>(this)->open(part));
  static_assert(is_unique_ptr_v<result_type>,
                "method must return a std::unique_ptr");

  using row_type = typename result_type::element_type;
  static_assert(std::is_base_of_v<row, row_type>,
                "must be an implementation of dataest row");

  return static_cast<DS *>(this)->open(part);
}

template <typename DS>
template <typename Val>
decltype(auto)
ana::dataset::input<DS>::read_column(const ana::dataset::range &part,
                                     const std::string &name) {

  // using result_type =
  //     decltype(static_cast<DS *>(this)->template read<Val>(part, name));
  // static_assert(is_unique_ptr_v<result_type>,
  //               "method must return a std::unique_ptr");

  // using column_type = typename result_type::element_type;
  // static_assert(std::is_base_of_v<column<Val>, column_type>,
  //               "must be an implementation of dataset column");

  return static_cast<DS *>(this)->template read<Val>(part, name);
}

template <typename DS>
std::unique_ptr<ana::dataset::row>
ana::dataset::input<DS>::open(const ana::dataset::range &) {
  return std::make_unique<ana::dataset::row>();
}