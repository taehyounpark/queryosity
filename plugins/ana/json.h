#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "queryosity/queryosity.h"

namespace queryosity {

class json : public queryosity::dataset::reader<json> {

public:
  template <typename T> class entry;

public:
  json(const nlohmann::json &data);
  json(std::ifstream &data);
  ~json() = default;

  virtual void parallelize(unsigned int nslots) override;
  virtual std::vector<std::pair<unsigned long long, unsigned long long>>
  partition() override;

  template <typename T>
  std::unique_ptr<entry<T>> read(unsigned int, const std::string &name) const;

protected:
  nlohmann::json const m_data;
  unsigned int m_nslots;
};

template <typename T> class json::entry : public queryosity::column::reader<T> {

public:
  entry(const nlohmann::json &data, const std::string &name);
  ~entry() = default;

  virtual const T &read(unsigned int, unsigned long long entry) const override;

protected:
  mutable T m_value;
  const nlohmann::json &m_data;
  const std::string m_key;
};

} // namespace queryosity

queryosity::json::json(std::ifstream &data)
    : m_data(nlohmann::json::parse(data)) {}

queryosity::json::json(nlohmann::json const &data) : m_data(data) {}

template <typename T>
queryosity::json::entry<T>::entry(nlohmann::json const &data,
                                  const std::string &name)
    : m_data(data), m_key(name) {}

void queryosity::json::parallelize(unsigned int nslots) { m_nslots = nslots; }

std::vector<std::pair<unsigned long long, unsigned long long>>
queryosity::json::partition() {
  const unsigned int nentries_per_slot = m_data.size() / m_nslots;
  if (!nentries_per_slot)
    return {{0, m_data.size()}};
  const unsigned int nentries_remainder = m_data.size() % m_nslots;
  std::vector<std::pair<unsigned long long, unsigned long long>> parts;
  for (unsigned int islot = 0; islot < m_nslots; ++islot) {
    parts.emplace_back(islot * nentries_per_slot,
                       (islot + 1) * nentries_per_slot);
    std::cout << parts.back().second << std::endl;
  }
  parts.back().second += nentries_remainder;
  assert(parts.back().second == m_data.size());
  return parts;
}

template <typename Val>
std::unique_ptr<queryosity::json::entry<Val>>
queryosity::json::read(unsigned int, const std::string &name) const {
  return std::make_unique<entry<Val>>(this->m_data, name);
}

template <typename T>
const T &queryosity::json::entry<T>::read(unsigned int,
                                          unsigned long long entry) const {
  m_value = this->m_data[entry][m_key].template get<T>();
  return m_value;
}