#pragma once

#include "column.hpp"

namespace queryosity {

//------------------------------------------------------------------------------
// fixed: value set manually
//------------------------------------------------------------------------------
template <typename Val> class column::fixed : public valued<Val> {

public:
  fixed(Val const &val);
  template <typename... Args> fixed(Args &&...args);
  virtual ~fixed() = default;

  const Val &value() const final override;

  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) final override;
  virtual void execute(unsigned int slot,
                       unsigned long long entry) final override;
  virtual void finalize(unsigned int slot) final override;

protected:
  Val m_value;
};

} // namespace queryosity

template <typename Val>
queryosity::column::fixed<Val>::fixed(Val const &val) : m_value(val) {}

template <typename Val>
template <typename... Args>
queryosity::column::fixed<Val>::fixed(Args &&...args)
    : m_value(std::forward<Args>(args)...) {}

template <typename Val>
const Val &queryosity::column::fixed<Val>::value() const {
  return m_value;
}

template <typename Val>
void queryosity::column::fixed<Val>::initialize(unsigned int slot,
                                                unsigned long long begin,
                                                unsigned long long end) {
  valued<Val>::initialize(slot, begin, end);
}

template <typename Val>
void queryosity::column::fixed<Val>::execute(unsigned int slot,
                                             unsigned long long entry) {
  valued<Val>::execute(slot, entry);
}

template <typename Val>
void queryosity::column::fixed<Val>::finalize(unsigned int slot) {
  valued<Val>::finalize(slot);
}