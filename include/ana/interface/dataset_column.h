#pragma once

#include "column.h"

namespace ana {

namespace dataset {

struct range;

/**
 * @brief Abstract base class to read values of existing columns in an input
 * dataset.
 * @tparam T column data type.
 */
template <typename T> class column : public ana::term<T> {

public:
  column();
  virtual ~column() = default;

  /**
   * @brief Read the value of the column at current entry.
   * @return Column value
   */
  virtual T const &read(const range &part, unsigned long long entry) const = 0;

  /**
   * @brief Get the value of the column at current entry.
   * @return Column value
   */
  virtual T const &value() const override;

  virtual void execute(const range &part,
                       unsigned long long entry) final override;

protected:
  mutable T const *m_addr;
  mutable bool m_updated;

  const range *m_part;
  unsigned long long m_entry;
};

} // namespace dataset

} // namespace ana

#include "dataset.h"

template <typename T>
ana::dataset::column<T>::column()
    : m_addr(nullptr), m_updated(false), m_part(nullptr), m_entry(0) {}

template <typename T> T const &ana::dataset::column<T>::value() const {
  if (!this->m_updated) {
    m_addr = &(this->read(*this->m_part, m_entry));
    m_updated = true;
  }
  return *m_addr;
}

template <typename T>
void ana::dataset::column<T>::execute(const ana::dataset::range &part,
                                      unsigned long long entry) {
  this->m_entry = entry;
  this->m_part = &part;
  this->m_updated = false;
}