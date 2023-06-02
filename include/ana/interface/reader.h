#pragma once

#include "column.h"

namespace ana {

/**
 * @brief Abstract base class to read values of existing columns in an input
 * dataset.
 * @tparam T column data type.
 */
template <typename T> class column::reader : public term<T> {
public:
  reader();
  virtual ~reader() = default;

  /**
   * @brief Read the value of the column at current entry.
   * @return Column value
   */
  virtual T const &read() const = 0;

  /**
   * @brief Get the value of the column at current entry.
   * @return Column value
   */
  virtual T const &value() const override;

  virtual void execute(const dataset::range &part,
                       unsigned long long entry) final override;

protected:
  void update() const;

protected:
  mutable T const *m_addr;
  mutable bool m_updated;
};

} // namespace ana

template <typename T> ana::column::reader<T>::reader() : m_addr(nullptr) {}

template <typename T>
void ana::column::reader<T>::execute(const ana::dataset::range &,
                                     unsigned long long) {
  m_updated = false;
}

template <typename T> T const &ana::column::reader<T>::value() const {
  if (!this->m_updated)
    this->update();
  return *m_addr;
}

template <typename T> void ana::column::reader<T>::update() const {
  m_addr = &(this->read());
  m_updated = true;
}