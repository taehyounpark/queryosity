#pragma once

#include <iostream>
#include <memory>

#include "dataset_processor.h"
#include "dataset_reader.h"
#include "multithread.h"

namespace ana {

namespace sample {

struct weight {
  weight(double value) : value(value) {}
  double value;
};

} // namespace sample

} // namespace ana