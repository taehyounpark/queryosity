#pragma once

#include "aggregation.h"

namespace ana {

template <typename T> class aggregation::summary {

public:
  // version for lazy<aggregation>
  template <typename Res>
  void record(const std::string &selection_path,
              std::decay_t<Res> aggregation_result) {
    static_cast<T *>(this)->record(selection_path, aggregation_result);
  }

  // version for varied<aggregation>
  template <typename Res>
  void record(const std::string &variation_name,
              const std::string &selection_path,
              std::decay_t<Res> aggregation_result) {
    static_cast<T *>(this)->record(variation_name, selection_path,
                                   aggregation_result);
  }

  template <typename Dest> void output(Dest &destination) {
    static_cast<T *>(this)->output(destination);
  }
};

} // namespace ana