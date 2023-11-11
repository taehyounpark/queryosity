#pragma once

#include <cassert>
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

#include "dataset.h"
#include "multithread.h"

namespace ana {

namespace dataset {

/**
 * @brief Range of a dataset to process by one thread slot.
 */
struct range {
  range(size_t slot, unsigned long long begin, unsigned long long end);
  ~range() = default;

  range operator+(const range &next);
  range &operator+=(const range &next);

  unsigned long long entries() const;

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

} // namespace dataset

} // namespace ana

inline ana::dataset::range::range(size_t slot, unsigned long long begin,
                                  unsigned long long end)
    : slot(slot), begin(begin), end(end) {}

inline unsigned long long ana::dataset::range::entries() const {
  assert(this->end > this->begin);
  return end - begin;
}

inline ana::dataset::range ana::dataset::range::operator+(const range &next) {
  assert(this->end == next.begin);
  return range(this->slot, this->begin, next.end);
}

inline ana::dataset::range &ana::dataset::range::operator+=(const range &next) {
  assert(this->end == next.begin);
  this->end = next.end;
  return *this;
}