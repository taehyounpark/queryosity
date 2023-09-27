#pragma once

#include "dataset.h"

namespace ana {
namespace dataset {

template <typename DS> class dataset {

public:
  dataset() = default;
  ~dataset() = default;

public:
  partition allocate_partition();
  double normalize_scale();
  void initialize_dataset();
  void finalize_dataset();

  decltype(auto) read_dataset(const range &part) const;

  template <typename Val>
  decltype(auto) read_column(const ana::dataset::range &part,
                             const std::string &name) const;

  std::unique_ptr<ana::dataset::reader>
  play(const ana::dataset::range &part) const;

  partition allocate();
  double normalize();
  void initialize();
  void finalize();
};

} // namespace dataset

} // namespace ana

#include "dataset_reader.h"

template <typename DS>
ana::dataset::partition ana::dataset::dataset<DS>::allocate_partition() {
  return this->allocate();
}

template <typename DS>
ana::dataset::partition ana::dataset::dataset<DS>::allocate() {
  return static_cast<DS *>(this)->allocate();
}

template <typename DS> double ana::dataset::dataset<DS>::normalize_scale() {
  return static_cast<DS *>(this)->normalize();
}

template <typename DS> inline double ana::dataset::dataset<DS>::normalize() {
  return 1.0;
}

template <typename DS> void ana::dataset::dataset<DS>::initialize_dataset() {
  static_cast<DS *>(this)->initialize();
}

template <typename DS> void ana::dataset::dataset<DS>::finalize_dataset() {
  static_cast<DS *>(this)->finalize();
}

template <typename DS> void ana::dataset::dataset<DS>::initialize() {
  // nothing to do (yet)
}

template <typename DS> void ana::dataset::dataset<DS>::finalize() {
  // nothing to do (yet)
}

template <typename DS>
decltype(auto)
ana::dataset::dataset<DS>::read_dataset(const ana::dataset::range &part) const {

  // using result_type = decltype(static_cast<const DS *>(this)->read(part));
  // static_assert(is_unique_ptr_v<result_type>,
  //               "method must return a std::unique_ptr");

  // using reader_type = typename result_type::element_type;
  // static_assert(std::is_base_of_v<reader, reader_type>,
  //               "must be an implementation of dataest::reader");

  return static_cast<const DS *>(this)->play(part);
}

template <typename DS>
template <typename Val>
decltype(auto)
ana::dataset::dataset<DS>::read_column(const ana::dataset::range &part,
                                       const std::string &name) const {

  // using result_type =
  //     decltype(static_cast<const DS *>(this)->template read<Val>(part,
  //     name));
  // static_assert(is_unique_ptr_v<result_type>,
  //               "method must return a std::unique_ptr");

  // using reader_type = typename result_type::element_type;
  // static_assert(std::is_base_of_v<column::reader<Val>, reader_type>,
  // "must be an implementation of column::reader");

  return static_cast<const DS *>(this)->template read<Val>(part, name);
}

template <typename DS>
std::unique_ptr<ana::dataset::reader>
ana::dataset::dataset<DS>::play(const ana::dataset::range &part) const {
  return std::make_unique<ana::dataset::reader>();
}