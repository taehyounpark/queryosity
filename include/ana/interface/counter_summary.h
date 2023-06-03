#pragma once

#include "counter.h"

namespace ana {

template <typename T> class counter::summary {

public:
  // version for lazy<counter>
  template <typename Res>
  void record(const std::string &selection_path,
              std::decay_t<Res> counter_result) {
    static_cast<T *>(this)->record(selection_path, counter_result);
  }

  // version for varied<counter>
  template <typename Res>
  void record(const std::string &variation_name,
              const std::string &selection_path,
              std::decay_t<Res> counter_result) {
    static_cast<T *>(this)->record(variation_name, selection_path,
                                   counter_result);
  }

  template <typename Dest> void output(Dest &destination) {
    static_cast<T *>(this)->output(destination);
  }
};

} // namespace ana