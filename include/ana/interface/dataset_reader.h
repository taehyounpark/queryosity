#pragma once

#include "dataset.h"

namespace ana {

namespace dataset {

template <typename T> class reader {

private:
  reader() = default;
  ~reader() = default;
  friend T;

public:
  // read a column of a data type with given name
  template <typename Val>
  decltype(auto) read_column(const range &part, const std::string &name) const;

  void start_part(const range &part);
  void read_entry(const range &part, unsigned long long entry);
  void finish_part(const range &part);
};

} // namespace dataset

} // namespace ana

template <typename T>
template <typename Val>
decltype(auto)
ana::dataset::reader<T>::read_column(const range &part,
                                     const std::string &name) const {

  using result_type =
      decltype(static_cast<const T *>(this)->template read<Val>(part, name));
  static_assert(is_unique_ptr_v<result_type>,
                "must be a std::unique_ptr of ana::column::reader<T>");

  return static_cast<const T *>(this)->template read<Val>(part, name);
}

template <typename T>
void ana::dataset::reader<T>::start_part(const ana::dataset::range &part) {
  static_cast<T *>(this)->start(part);
}

template <typename T>
void ana::dataset::reader<T>::read_entry(const ana::dataset::range &part,
                                         unsigned long long entry) {
  static_cast<T *>(this)->next(part, entry);
}

template <typename T>
void ana::dataset::reader<T>::finish_part(const ana::dataset::range &part) {
  static_cast<T *>(this)->finish(part);
}