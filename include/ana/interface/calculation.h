#pragma once

#include "column.h"

namespace ana {

/**
 * @brief Calculate a column value for each dataset entry.
 * @tparam Val Column value type.
 * @details A calculation is performed once per-entry (if needed) and its value
 * is stored for multiple accesses by downstream actions within the entry. The
 * type `Val` must be *CopyConstructible* and *CopyAssignable*.
 */
template <typename Val> class column::calculation : public term<Val> {

public:
  calculation() = default;
  virtual ~calculation() = default;

protected:
  template <typename... Args> calculation(Args &&...args);

public:
  virtual const Val &value() const final override;

  virtual Val calculate() const = 0;

  virtual void initialize(const dataset::range &part) override;
  virtual void execute(const dataset::range &part,
                       unsigned long long entry) final override;
  virtual void finalize(const dataset::range &part) override;

protected:
  void update() const;
  void reset() const;

protected:
  mutable Val m_value;
  mutable bool m_updated;
};

} // namespace ana

template <typename Val>
template <typename... Args>
ana::column::calculation<Val>::calculation(Args &&...args)
    : m_value(std::forward<Args>(args)...) {}

template <typename Val>
const Val &ana::column::calculation<Val>::value() const {
  if (!m_updated)
    this->update();
  return m_value;
}

template <typename Val> void ana::column::calculation<Val>::update() const {
  m_value = this->calculate();
  m_updated = true;
}

template <typename Val> void ana::column::calculation<Val>::reset() const {
  m_updated = false;
}

template <typename Val>
void ana::column::calculation<Val>::initialize(const ana::dataset::range &) {}

template <typename Val>
void ana::column::calculation<Val>::execute(const ana::dataset::range &,
                                            unsigned long long) {
  this->reset();
}

template <typename Val>
void ana::column::calculation<Val>::finalize(const ana::dataset::range &) {}