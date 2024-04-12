#pragma once

#include "queryosity.h"

namespace queryosity {

namespace ak {

template <typename T> class view;

template <typename T> class record;

template <typename Array> class view : public dataset::reader<view<Array>> {
public:
  using record_type = decltype(std::declval<Array>()[0]);

public:
  view(Array const &array);
  ~view() = default;

  virtual void parallelize(unsigned int nslots) final override;

  virtual std::vector<std::pair<unsigned long long, unsigned long long>>
  partition() final override;

  template <typename T>
  std::unique_ptr<record<Array>> read(unsigned int slot,
                                      const std::string &key) const;

protected:
  Array const &m_array;
  unsigned int m_nslots;
};

template <typename Array> using record_t = typename view<Array>::record_type;

template <typename Array>
class record : public column::reader<record_t<Array>> {
public:
  using value_type = record_t<Array>;

public:
  record(Array const &array);
  ~record() = default;

  record_t<Array> const &read(unsigned int,
                              unsigned long long entry) const final override;

protected:
  Array const &m_array;
};

} // namespace ak

} // namespace queryosity

template <typename Array>
queryosity::ak::view<Array>::view(Array const &array)
    : m_array(array), m_nslots(1) {}

template <typename Array>
void queryosity::ak::view<Array>::parallelize(unsigned int nslots) {
  m_nslots = nslots;
}

template <typename Array>
std::vector<std::pair<unsigned long long, unsigned long long>>
queryosity::ak::view<Array>::partition() {
  if (m_nslots == 1)
    return {{0, m_array.size()}};

  // take division & remainder
  const unsigned int nentries_per_slot = m_array.size() / m_nslots;
  const unsigned int nentries_remainder = m_array.size() % m_nslots;

  // divide entries evenly between parts
  std::vector<std::pair<unsigned long long, unsigned long long>> parts;
  for (unsigned int islot = 0; islot < m_nslots; ++islot) {
    parts.emplace_back(islot * nentries_per_slot,
                       (islot + 1) * nentries_per_slot);
  }
  // add remaining entries to last part
  parts.back().second += nentries_remainder;
  // sanity check
  assert(parts.back().second == m_array.size());

  return parts;
}

template <typename Array>
template <typename T>
std::unique_ptr<queryosity::ak::record<Array>>
queryosity::ak::view<Array>::read(unsigned int, const std::string &name) const {
  if (!name.empty()) {
    throw std::runtime_error(
        "C++ Awkward Array fields cannot be read-in by their names at runtime");
  }
  return std::make_unique<queryosity::ak::record<Array>>(this->m_array);
}

template <typename Array>
queryosity::ak::record<Array>::record(Array const &array) : m_array(array) {}

template <typename Array>
queryosity::ak::record_t<Array> const &
queryosity::ak::record<Array>::read(unsigned int, unsigned long long i) const {
  // std::cout << this->m_array[i][0].x() << std::endl;
  return this->m_array[i];
}