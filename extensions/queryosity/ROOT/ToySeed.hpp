#pragma once

#include "BootstrapGenerator/BootstrapGenerator.h"

#include <queryosity.hpp>

#include <memory>
#include <string>
#include <vector>

namespace queryosity {

namespace ROOT {

class ToySeed
    : public qty::query::callback<unsigned int, unsigned int, unsigned int> {

public:
  ToySeed(unsigned int ntoy);
  virtual ~ToySeed() = default;

  virtual void fill(qty::column::observable<unsigned int>,
                    qty::column::observable<unsigned int>,
                    qty::column::observable<unsigned int>,
                    double) final override;

  BootstrapGenerator* get_generator() const;

protected:
  // histogram
  mutable std::shared_ptr<BootstrapGenerator> m_bootgen; //!
};

} // namespace ROOT

} // namespace queryosity

queryosity::ROOT::ToySeed::ToySeed(unsigned int ntoy)
    : m_bootgen(
          std::make_shared<BootstrapGenerator>("bootgen", "bootgen", ntoy)) {}

void queryosity::ROOT::ToySeed::fill(
    qty::column::observable<unsigned int> run_number, qty::column::observable<unsigned int> event_number, qty::column::observable<unsigned int> channel_number, double) {
      m_bootgen->Generate(run_number.value(), event_number.value(), channel_number.value());
}

BootstrapGenerator* queryosity::ROOT::ToySeed::get_generator() const {
  return m_bootgen.get();
}
