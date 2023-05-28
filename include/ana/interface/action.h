#pragma once

#include <algorithm>
#include <atomic>
#include <iostream>
#include <memory>
#include <vector>

namespace ana {

class action {

public:
  action() = default;
  virtual ~action() = default;

  virtual void initialize() = 0;
  virtual void execute() = 0;
  virtual void finalize() = 0;
};

} // namespace ana