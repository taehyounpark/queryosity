#pragma once

#include <numeric>
#include <unordered_map>
#include <variant>

#include "ana/analogical.h"

namespace {

using table_row_t = std::unordered_map<
    std::string, std::variant<unsigned int, int, float, double, std::string>>;
using table_data_t = std::vector<table_row_t>;

class table;
class table_reader;
template <typename T> class table_column;

class table : public ana::dataset::input<table> {

public:
  table(table_data_t);

  ana::dataset::partition allocate();

  std::unique_ptr<table_reader> read(const ana::dataset::range &part) const;

protected:
  const table_data_t m_data;
};

class table_reader : public ana::dataset::reader<table_reader> {

public:
  table_reader(const table_data_t &data);

  template <typename T>
  std::unique_ptr<table_column<T>> read(const ana::dataset::range &part,
                                        const std::string &name) const;

  void start(const ana::dataset::range &part);
  void next(const ana::dataset::range &part, unsigned long long entry);
  void finish(const ana::dataset::range &part);

protected:
  const table_data_t &m_data;
};

template <typename T> class table_column : public ana::column::reader<T> {

public:
  table_column(const table_data_t &data, const std::string &column_name);

  virtual const T &read(const ana::dataset::range &part,
                        unsigned long long entry) const override;

protected:
  const table_data_t &m_data;
  const std::string m_column_name;
};

} // namespace

table::table(table_data_t data) : m_data(data) {}

ana::dataset::partition table::allocate() {
  ana::dataset::partition parts;
  auto nentries = m_data.size();
  for (unsigned int i = 0; i < nentries; ++i) {
    parts.add_part(i, i, i + 1);
  }
  return parts;
}

std::unique_ptr<table_reader> table::read(const ana::dataset::range &) const {
  return std::make_unique<table_reader>(this->m_data);
}

table_reader::table_reader(const table_data_t &data) : m_data(data) {}

template <typename T>
std::unique_ptr<table_column<T>>
table_reader::read(const ana::dataset::range &,
                   const std::string &column_name) const {
  return std::make_unique<table_column<T>>(this->m_data, column_name);
}

void table_reader::start(const ana::dataset::range &part) {}

void table_reader::next(const ana::dataset::range &part,
                        unsigned long long entry) {}

void table_reader::finish(const ana::dataset::range &part) {}

template <typename T>
table_column<T>::table_column(const table_data_t &data,
                              const std::string &column_name)
    : m_data(data), m_column_name(column_name) {}

template <typename T>
const T &table_column<T>::read(const ana::dataset::range &part,
                               unsigned long long entry) const {
  return std::get<T>(m_data.at(entry).at(m_column_name));
}