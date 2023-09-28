#include <nlohmann/json.hpp>

#include "ana/analogical.h"

namespace ana {

class json : public ana::dataset::input<json> {

public:
  template <typename T> class column;

public:
  json(const nlohmann::json &data);
  // template <typename Arg> json(Arg &&arg);
  ~json() = default;

  ana::dataset::partition allocate();

  // std::unique_ptr<ana::dataset::reader>
  // read(const ana::dataset::range &part) const;

  template <typename T>
  std::unique_ptr<column<T>> read(const ana::dataset::range &part,
                                  const std::string &name) const;

protected:
  nlohmann::json m_data;
};

template <typename T> class json::column : public ana::dataset::column<T> {

public:
  column(const nlohmann::json &data, const std::string &name);
  ~column() = default;

  virtual const T &read(const ana::dataset::range &part,
                        unsigned long long entry) const override;

protected:
  mutable T m_value;
  const nlohmann::json &m_data;
  const std::string m_name;
};

} // namespace ana

ana::json::json(nlohmann::json const &data) : m_data(data) {}

template <typename T>
ana::json::column<T>::column(nlohmann::json const &data,
                             const std::string &name)
    : m_data(data), m_name(name) {}

ana::dataset::partition ana::json::allocate() {
  ana::dataset::partition parts;
  auto nentries = m_data.size();
  for (unsigned int i = 0; i < nentries; ++i) {
    parts.add_part(i, i, i + 1);
  }
  return parts;
}

// std::unique_ptr<ana::dataset::reader>
// ana::json::read(const ana::dataset::range &part) const {
//   return std::make_unique<ana::dataset::reader>();
// }

template <typename Val>
std::unique_ptr<ana::json::column<Val>>
ana::json::read(const ana::dataset::range &, const std::string &name) const {
  return std::make_unique<column<Val>>(this->m_data, name);
}

template <typename T>
const T &ana::json::column<T>::read(const ana::dataset::range &,
                                    unsigned long long entry) const {
  m_value = this->m_data[entry][m_name].template get<T>();
  return m_value;
}