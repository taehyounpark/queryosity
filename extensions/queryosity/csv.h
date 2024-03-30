#include "rapidcsv.h"

#include "queryosity.h"

namespace queryosity {

class csv : public dataset::reader<csv> {

public:
  template <typename T>
  class cell;

public:
  csv(std::ifstream& data);
  virtual ~csv() = default;

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
  std::unique_ptr<cell<T>> read(unsigned int slot,
                                const std::string &column_name) const;

protected:
  rapidcsv::Document m_document;
  unsigned int m_nslots;
};

template <typename T> class csv::cell : public column::reader<T> {

public:
  cell(rapidcsv::Document const &document, const std::string &column_name)
      : m_value(), m_document(document), m_column_name(column_name) {};
  virtual ~cell() = default;

  virtual T const &read(unsigned int, unsigned long long) const override;

protected:
  mutable T m_value;
  rapidcsv::Document const &m_document;
  std::string m_column_name;
};

} // namespace queryosity

inline queryosity::csv::csv(std::ifstream& data)
    : m_document(data), m_nslots(1) {}

inline void queryosity::csv::parallelize(unsigned int nslots) { m_nslots = nslots; }

inline std::vector<std::pair<unsigned long long, unsigned long long>>
queryosity::csv::partition() {
  unsigned int nrows = m_document.GetRowCount();

  // take division & remainder
  const unsigned int nentries_per_slot = nrows / m_nslots;
  const unsigned int nentries_remainder = nrows % m_nslots;

  // divide entries evenly between parts
  std::vector<std::pair<unsigned long long, unsigned long long>> parts;
  for (unsigned int islot = 0; islot < m_nslots; ++islot) {
    parts.emplace_back(islot * nentries_per_slot,
                       (islot + 1) * nentries_per_slot);
  }
  // add remaining entries to last part
  parts.back().second += nentries_remainder;
  // sanity check
  assert(parts.back().second == nrows);

  return parts;
}

template <typename T>
std::unique_ptr<queryosity::csv::cell<T>>
queryosity::csv::read(unsigned int, const std::string &column_name) const {
  return std::make_unique<cell<T>>(m_document, column_name);
}

template <typename T>
T const &queryosity::csv::cell<T>::read(unsigned int, unsigned long long entry) const {
  this->m_value = this->m_document.template GetCell<T>(this->m_column_name, entry);
  return this->m_value;
}