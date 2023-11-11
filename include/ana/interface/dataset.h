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

struct partition;

template <typename T> class input;

class player;

template <typename T> class reader;

template <typename T> class column;

struct first {
  first(long long nrows) : nrows(nrows) {}
  long long nrows;
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