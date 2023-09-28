#pragma once

#include "operation.h"

namespace ana {

namespace dataset {

struct range;

class reader : public operation {

public:
  reader() = default;
  virtual ~reader() = default;

public:
  virtual void initialize(const range &) override;
  virtual void execute(const range &, unsigned long long) override;
  virtual void finalize(const range &) override;
};

} // namespace dataset

} // namespace ana

#include "dataset.h"

void ana::dataset::reader::initialize(const ana::dataset::range &) {}

void ana::dataset::reader::execute(const ana::dataset::range &,
                                   unsigned long long) {}

void ana::dataset::reader::finalize(const ana::dataset::range &) {}