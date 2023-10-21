#pragma once

#include "operation.h"

namespace ana {

namespace dataset {

struct range;

class player : public operation {

public:
  player() = default;
  virtual ~player() = default;

public:
  virtual void initialize(const range &) override;
  virtual void execute(const range &, unsigned long long) override;
  virtual void finalize(const range &) override;
};

} // namespace dataset

} // namespace ana

#include "dataset.h"

inline void ana::dataset::player::initialize(const ana::dataset::range &) {}

inline void ana::dataset::player::execute(const ana::dataset::range &,
                                          unsigned long long) {}

inline void ana::dataset::player::finalize(const ana::dataset::range &) {}