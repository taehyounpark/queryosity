#pragma once

#include <cassert>
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "multithread.h"

namespace ana {

namespace dataset {

using range = std::pair<unsigned long long, unsigned long long>;

class source;

class player;

template <typename T> class reader;

template <typename T> struct input;

template <typename T> class opened;

template <typename T> class column;

template <typename... Ts> class columns;

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

} // namespace ana

#include "column.h"
#include "dataset_source.h"
