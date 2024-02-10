#pragma once

#include <cassert>
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

#include "multithread.h"

namespace ana {

namespace dataset {

struct range;

using row = unsigned long long;

struct partition;

template <typename T> class input;

template <typename T> class opened;

class player;

template <typename T> class reader;

struct limit {
  limit(long long nrows) : nrows(nrows) {}
  long long nrows;
  operator long long() { return nrows; }
};

struct offset {
  offset(unsigned long long pos) : pos(pos) {}
  unsigned long long pos;
  operator unsigned long long() { return pos; }
};

struct weight {
  weight(double value) : value(value) {}
  double value;
  operator double() { return value; }
};

} // namespace dataset

template <typename T>
using open_player_t = typename decltype(std::declval<T const &>().open_player(
    std::declval<const ana::dataset::range &>()))::element_type;

template <typename T, typename Val>
using read_column_t =
    typename decltype(std::declval<T>().template read_column<Val>(
        std::declval<dataset::range const &>(),
        std::declval<std::string const &>()))::element_type;

template <typename T> struct is_unique_ptr : std::false_type {};
template <typename T>
struct is_unique_ptr<std::unique_ptr<T>> : std::true_type {};
template <typename T>
static constexpr bool is_unique_ptr_v = is_unique_ptr<T>::value;

} // namespace ana

#include "column.h"
#include "dataset_player.h"
#include "dataset_range.h"