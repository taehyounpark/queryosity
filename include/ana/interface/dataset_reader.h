#pragma once

#include "dataset.h"

namespace ana {

namespace dataset {

class reader : public operation {

public:
  reader() = default;
  ~reader() = default;

public:
  virtual void initialize(const range &part) override;
  virtual void execute(const range &part, unsigned long long entry) override;
  virtual void finalize(const range &part) override;
};

} // namespace dataset

} // namespace ana

void ana::dataset::reader::initialize(const ana::dataset::range &part) {}

void ana::dataset::reader::execute(const ana::dataset::range &part,
                                   unsigned long long entry) {}

void ana::dataset::reader::finalize(const ana::dataset::range &part) {}