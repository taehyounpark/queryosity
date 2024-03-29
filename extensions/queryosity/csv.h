#include "rapidcsv.h"

#include "queryosity.h"

namespace queryosity {

class csv : public dataset::reader<csv> {

public:
class cell;

public:
csv(const std::string& file_path);
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
std::unique_ptr<item<T>> read(unsigned int slot,
                              const std::string &column_name) const;

protected:
  CSVReader m_reader;
  unsigned int m_nslots;

};

}

inline queryosity::csv::csv(const std::string& file_path) : m_reader(file_path), m_nslots(1) {}

inline std::vector<std::pair<unsigned long long, unsigned long long>> partition() {
  unsigned int nrows = 0;
  for(auto it = m_reader.begin() ; it != m_reader.end() ; ++it) {
    nrows++;
  }

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