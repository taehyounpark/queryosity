#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "ana/analogical.h"

namespace ana {

class json : public ana::dataset::source<json> {

public:
  template <typename T> class entry;

public:
  json(const nlohmann::json &data);
  json(std::ifstream &data);
  ~json() = default;

  virtual ana::dataset::partition parallelize() override;

  template <typename T>
  std::unique_ptr<entry<T>> read(const ana::dataset::range &part,
                                 const std::string &name) const;

protected:
  nlohmann::json const m_data;
};

template <typename T> class json::entry : public ana::dataset::reader<T> {

public:
  entry(const nlohmann::json &data, const std::string &name);
  ~entry() = default;

  virtual const T &read(const ana::dataset::range &part,
                        unsigned long long entry) const override;

protected:
  mutable T m_value;
  const nlohmann::json &m_data;
  const std::string m_key;
};

} // namespace ana

ana::json::json(std::ifstream &data) : m_data(nlohmann::json::parse(data)) {}

ana::json::json(nlohmann::json const &data) : m_data(data) {}

template <typename T>
ana::json::entry<T>::entry(nlohmann::json const &data, const std::string &name)
    : m_data(data), m_key(name) {}

ana::dataset::partition ana::json::parallelize() {
  return ana::dataset::partition(m_data.size());
}

template <typename Val>
std::unique_ptr<ana::json::entry<Val>>
ana::json::read(const ana::dataset::range &, const std::string &name) const {
  return std::make_unique<entry<Val>>(this->m_data, name);
}

template <typename T>
const T &ana::json::entry<T>::read(const ana::dataset::range &,
                                   unsigned long long entry) const {
  m_value = this->m_data[entry][m_key].template get<T>();
  return m_value;
}