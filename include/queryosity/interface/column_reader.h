#pragma once

#include "column.h"

namespace queryosity {

namespace column {

struct range;

/**
 * @brief Read columns from a dataset.
 * @tparam T column data type.
 */
template <typename T> class reader : public valued<T> {

public:
  using const_reference = T const &;

public:
  reader();
  virtual ~reader() = default;

  /**
   * Read the value of the column at current entry.
   * @param[in] slot Multithreaded slot enumerator.
   * @param[in] entry Dataset global entry enumerator.
   * @return Column value at current entry.
   */
  virtual const_reference read(unsigned int, unsigned long long) const = 0;

  virtual const_reference value() const override;

  virtual void execute(unsigned int, unsigned long long) final override;

protected:
  mutable T const *m_addr;
  mutable bool m_updated;

private:
  unsigned int m_slot;
  unsigned long long m_entry;
};

} // namespace column

} // namespace queryosity

#include "dataset.h"

template <typename T>
queryosity::column::reader<T>::reader()
    : m_addr(nullptr), m_updated(false), m_slot(0), m_entry(0) {}

template <typename T> T const &queryosity::column::reader<T>::value() const {
  if (!this->m_updated) {
    m_addr = &(this->read(this->m_slot, this->m_entry));
    m_updated = true;
  }
  return *m_addr;
}

template <typename T>
void queryosity::column::reader<T>::execute(unsigned int slot,
                                            unsigned long long entry) {
  this->m_slot = slot;
  this->m_entry = entry;
  this->m_updated = false;
}