#include <nlohmann/json.hpp>

#include "ana/analogical.h"

class Json : public ana::dataset::input<Json> {

public:
  template <typename T> class Entry;

public:
  Json(const nlohmann::json &data);
  ~Json() = default;

  virtual ana::dataset::partition allocate() override;

  template <typename T>
  std::unique_ptr<Entry<T>> read(const ana::dataset::range &part,
                                 const std::string &name) const;

protected:
  nlohmann::json const m_data;
};

template <typename T> class Json::Entry : public ana::dataset::column<T> {

public:
  Entry(const nlohmann::json &data, const std::string &name);
  ~Entry() = default;

  virtual const T &read(const ana::dataset::range &part,
                        unsigned long long entry) const override;

protected:
  mutable T m_value;
  const nlohmann::json &m_data;
  const std::string m_key;
};

Json::Json(nlohmann::json const &data) : m_data(data) {}

template <typename T>
Json::Entry<T>::Entry(nlohmann::json const &data, const std::string &name)
    : m_data(data), m_key(name) {}

ana::dataset::partition Json::allocate() {
  return ana::dataset::partition(m_data.size());
}

template <typename Val>
std::unique_ptr<Json::Entry<Val>> Json::read(const ana::dataset::range &,
                                             const std::string &name) const {
  return std::make_unique<Entry<Val>>(this->m_data, name);
}

template <typename T>
const T &Json::Entry<T>::read(const ana::dataset::range &,
                              unsigned long long entry) const {
  m_value = this->m_data[entry][m_key].template get<T>();
  return m_value;
}