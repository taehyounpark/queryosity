#pragma once

#include "systematic.h"

namespace queryosity {

/**
 * @class action
 * @brief abstract base class with initialization, execution, finalization steps
 */
class action : public systematic::mode {

public:
  action() = default;
  virtual ~action() = default;

  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) = 0;
  virtual void execute(unsigned int slot, unsigned long long entry) = 0;
  virtual void finalize(unsigned int slot) = 0;

protected:
};

} // namespace queryosity