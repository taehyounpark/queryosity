#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "queryosity/queryosity.h"

namespace queryosity {

/**
 * @ingroup ext
 * @brief JSON dataset for queryosity.
 */
class json : public queryosity::dataset::reader<json> {

public:
  template <typename T> class item;

public:
  json(const nlohmann::json &data);
  json(std::ifstream &data);
  ~json() = default;

  /**
   * @brief Parallelize the dataset for parallel processing.
   * @param[in] nslots Requested concurrency.
   */
  virtual void parallelize(unsigned int nslots) override;

  /**
   * @brief Partition the dataset for parallel processing.
   * @returns Dataset partition.
   */
  virtual std::vector<std::pair<unsigned long long, unsigned long long>>
  partition() override;

  /**
   * @brief Read a column.
   * @tparam T Column data type.
   * @param[in] slot Multithreading slot index.
   * @param[in] column_name Column name.
   */
  template <typename T>
  std::unique_ptr<item<T>> read(unsigned int slot,
                                const std::string &column_name) const;

protected:
  nlohmann::json const m_data;
  unsigned int m_nslots;
};

/**
 * @ingroup ext
 * @brief JSON item as "column" data.
 * @tparam T data type.
 */
template <typename T> class json::item : public queryosity::column::reader<T> {

public:
  /**
   * @param[in] data JSON data.
   * @param[in] key Object key.
   */
  item(const nlohmann::json &data, const std::string &key);
  ~item() = default;

  /**
   * @brief Read-in the data value.
   * @param[in] slot Thread index.
   * @param[in] entry Entry number.
   */
  virtual const T &read(unsigned int slot,
                        unsigned long long entry) const override;

protected:
  mutable T m_value;
  const nlohmann::json &m_data;
  const std::string m_key;
};

} // namespace queryosity

queryosity::json::json(nlohmann::json const &data) : m_data(data) {}

queryosity::json::json(std::ifstream &data)
    : m_data(nlohmann::json::parse(data)) {}

template <typename T>
queryosity::json::item<T>::item(nlohmann::json const &data,
                                const std::string &name)
    : m_data(data), m_key(name) {}

void queryosity::json::parallelize(unsigned int nslots) { m_nslots = nslots; }

std::vector<std::pair<unsigned long long, unsigned long long>>
queryosity::json::partition() {
  if (m_nslots == 1)
    return {{0, m_data.size()}};

  // take division & remainder
  const unsigned int nentries_per_slot = m_data.size() / m_nslots;
  const unsigned int nentries_remainder = m_data.size() % m_nslots;

  // divide entries evenly between parts
  std::vector<std::pair<unsigned long long, unsigned long long>> parts;
  for (unsigned int islot = 0; islot < m_nslots; ++islot) {
    parts.emplace_back(islot * nentries_per_slot,
                       (islot + 1) * nentries_per_slot);
  }
  // add remaining entries to last part
  parts.back().second += nentries_remainder;
  // sanity check
  assert(parts.back().second == m_data.size());

  return parts;
}

template <typename Val>
std::unique_ptr<queryosity::json::item<Val>>
queryosity::json::read(unsigned int, const std::string &name) const {
  return std::make_unique<item<Val>>(this->m_data, name);
}

template <typename T>
const T &queryosity::json::item<T>::read(unsigned int,
                                         unsigned long long item) const {
  m_value = this->m_data[item][m_key].template get<T>();
  return m_value;
}