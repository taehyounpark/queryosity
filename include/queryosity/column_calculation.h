#pragma once

#include "column.h"

namespace queryosity {

/**
 * @brief Calculate a column value for each dataset entry.
 * @tparam Val Column value type.
 * @details A calculation is performed once per-entry (if needed) and its value
 * is stored for multiple accesses by downstream actions within the entry.
 * The type `Val` must be *CopyConstructible* and *CopyAssignable*.
 */
template <typename Val> class column::calculation : public valued<Val> {

public:
  calculation();
  virtual ~calculation() = default;

protected:
  template <typename... Args> calculation(Args &&...args);

public:
  virtual const Val &value() const final override;

  virtual Val calculate() const = 0;

  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) override;
  virtual void execute(unsigned int slot, unsigned long long entry) override;
  virtual void finalize(unsigned int slot) override;

protected:
  void update() const;
  void reset() const;

protected:
  mutable Val m_value;
  mutable bool m_updated;
};

} // namespace queryosity

template <typename Val>
queryosity::column::calculation<Val>::calculation()
    : m_value(), m_updated(false) {}

template <typename Val>
template <typename... Args>
queryosity::column::calculation<Val>::calculation(Args &&...args)
    : m_value(std::forward<Args>(args)...) {}

template <typename Val>
const Val &queryosity::column::calculation<Val>::value() const {
  if (!m_updated)
    this->update();
  return m_value;
}

template <typename Val>
void queryosity::column::calculation<Val>::update() const {
  m_value = std::move(this->calculate());
  m_updated = true;
}

template <typename Val>
void queryosity::column::calculation<Val>::reset() const {
  m_updated = false;
}

template <typename Val>
void queryosity::column::calculation<Val>::initialize(unsigned int,
                                                      unsigned long long,
                                                      unsigned long long) {}

template <typename Val>
void queryosity::column::calculation<Val>::execute(unsigned int,
                                                   unsigned long long) {
  this->reset();
}

template <typename Val>
void queryosity::column::calculation<Val>::finalize(unsigned int) {}