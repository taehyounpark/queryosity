#pragma once

#include <unordered_map>
#include <variant>
#include <numeric>

#include "ana/interface.h"

using trivial_data_t = std::vector<std::unordered_map<std::string,std::variant<int, double, std::string>>>;

class trivial_input;
class trivial_reader;
template <typename T> class trivial_column;

class trivial_input : public ana::dataset::input<trivial_input> {

public:
  trivial_input(trivial_data_t);

  ana::dataset::partition allocate();

  std::unique_ptr<trivial_reader> read(const ana::dataset::range &part) const;

protected:
  const trivial_data_t m_data;
};

class trivial_reader : public ana::dataset::reader<trivial_reader> {

public:
  trivial_reader(const trivial_data_t &data);

  template <typename T>
  std::unique_ptr<trivial_column<T>> read(const ana::dataset::range &part,
                                      const std::string &name) const;

 	void start(const ana::dataset::range& part);
  void next(const ana::dataset::range &part, unsigned long long entry);
 	void finish(const ana::dataset::range& part);

protected:
  const trivial_data_t &m_data;
};

template <typename T>
class trivial_column : public ana::column::reader<T> {

public:
  trivial_column(const trivial_data_t &data,
                const std::string &column_name);

  virtual const T& read(const ana::dataset::range &part, unsigned long long entry) const override;

protected:
  const trivial_data_t &m_data;
  const std::string m_column_name;
};

trivial_input::trivial_input(trivial_data_t data)
    : m_data(data) {}

ana::dataset::partition trivial_input::allocate() {
  ana::dataset::partition parts;
  auto nentries = m_data.size();
  for (int i=0 ; i<nentries ; ++i) {
    parts.add_part(i, i, i+1);
  }
  return parts;
}

std::unique_ptr<trivial_reader>
trivial_input::read(const ana::dataset::range &) const {
  return std::make_unique<trivial_reader>(this->m_data);
}

trivial_reader::trivial_reader(
    const trivial_data_t &data)
    : m_data(data) {}

template <typename T>
std::unique_ptr<trivial_column<T>>
trivial_reader::read(const ana::dataset::range &,
                            const std::string &column_name) const {
  return std::make_unique<trivial_column<T>>(this->m_data, column_name);
}

void trivial_reader::start(const ana::dataset::range &part) {}

void trivial_reader::next(const ana::dataset::range &part,
                                 unsigned long long entry) {}

void trivial_reader::finish(const ana::dataset::range &part) {}

template <typename T>
trivial_column<T>::trivial_column(
    const trivial_data_t &data,
    const std::string &column_name)
    : m_data(data), m_column_name(column_name) {}

template <typename T>
const T &trivial_column<T>::read(const ana::dataset::range &part,
                                          unsigned long long entry) const {
  return std::get<T>(m_data.at(entry).at(m_column_name));
}