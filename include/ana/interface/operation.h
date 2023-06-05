#pragma once

#include "dataset.h"

namespace ana {

/**
 * @class operation
 * @brief abstract base class with initialization, execution, finalization steps
 */
class operation {

public:
  operation() = default;
  virtual ~operation() = default;

  virtual void initialize(const dataset::range &part) = 0;
  virtual void execute(const dataset::range &part,
                       unsigned long long entry) = 0;
  virtual void finalize(const dataset::range &part) = 0;
};

} // namespace ana