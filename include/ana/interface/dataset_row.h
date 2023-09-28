#pragma once

#include "operation.h"

namespace ana {

namespace dataset {

struct range;

class row : public operation {

public:
  row() = default;
  virtual ~row() = default;

public:
  virtual void initialize(const range &) override;
  virtual void execute(const range &, unsigned long long) override;
  virtual void finalize(const range &) override;
};

} // namespace dataset

} // namespace ana

#include "dataset.h"

void ana::dataset::row::initialize(const ana::dataset::range &) {}

void ana::dataset::row::execute(const ana::dataset::range &,
                                unsigned long long) {}

void ana::dataset::row::finalize(const ana::dataset::range &) {}