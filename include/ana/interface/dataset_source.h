#pragma once

#include "action.h"
#include "column.h"

namespace ana {

namespace dataset {

class source : public action {
public:
  source() = default;
  virtual ~source() = default;
  virtual void parallelize(unsigned int concurrency) = 0;
  virtual double normalize();
  virtual void initialize();
  virtual std::vector<std::pair<unsigned long long, unsigned long long>>
  partition() = 0;
  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) override;
  virtual void execute(unsigned int slot, unsigned long long entry) override;
  virtual void finalize(unsigned int slot) override;
  virtual void finalize();
};

template <typename DS> class reader : public source {

public:
  virtual ~reader() = default;

  template <typename Val>
  std::unique_ptr<ana::column::reader<Val>>
  read_column(unsigned int slot, const std::string &name);

  template <typename Val>
  std::unique_ptr<ana::column::reader<Val>> read(unsigned int slot,
                                                 const std::string &name);
};

} // namespace dataset

} // namespace ana

template <typename T, typename Val>
using read_column_t =
    typename decltype(std::declval<T>().template read_column<Val>(
        std::declval<unsigned int>(),
        std::declval<std::string const &>()))::element_type;

#include "column_reader.h"

inline double ana::dataset::source::normalize() { return 1.0; }

inline void ana::dataset::source::initialize() {}

inline void ana::dataset::source::initialize(unsigned int, unsigned long long,
                                             unsigned long long) {}

inline void ana::dataset::source::execute(unsigned int, unsigned long long) {}

inline void ana::dataset::source::finalize(unsigned int) {}

inline void ana::dataset::source::finalize() {}

template <typename DS>
template <typename Val>
std::unique_ptr<ana::column::reader<Val>>
ana::dataset::reader<DS>::read_column(unsigned int slot,
                                      const std::string &name) {
  auto col_rdr = static_cast<DS *>(this)->template read<Val>(slot, name);
  if (!col_rdr)
    throw std::runtime_error("dataset column cannot be read");
  return col_rdr;
}

template <typename DS>
template <typename Val>
std::unique_ptr<ana::column::reader<Val>>
ana::dataset::reader<DS>::read(unsigned int, const std::string &) {
  return nullptr;
}