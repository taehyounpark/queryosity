#pragma once

#include "column.hpp"
#include "dataflow.hpp"
#include "query_output.hpp"
#include "selection.hpp"

#include <cmath>

namespace queryosity {

namespace selection {

/**
 * @brief cutbookkeeper (sum of weights and squared error) at a selection.
 */
struct count_t {
  unsigned long long entries;
  double value;
  double error;
};

class counter : public query::aggregation<count_t> {

public:
  counter() = default;
  virtual ~counter() = default;

  virtual void count(double w) final override;
  virtual count_t result() const final override;
  virtual void finalize(unsigned int) final override;
  virtual count_t
  merge(std::vector<count_t> const &results) const final override;

protected:
  count_t m_cnt;
};

/**
 * @brief Argument for column cutbookkeeper.
 * @tparam Sel (Varied) lazy column node.
 * @todo C++20: Use concept to require lazy<column<Val>(::varied)>.
 */
template <typename... Sels> struct cutbookkeeper {

public:
  cutbookkeeper(Sels const &...sels);
  ~cutbookkeeper() = default;

  auto make(dataflow &df) const;

protected:
  std::tuple<Sels...> m_selections;
};

} // namespace selection

} // namespace queryosity

inline void queryosity::selection::counter::count(double w) {
  m_cnt.entries++;
  m_cnt.value += w;
  m_cnt.error += w * w;
}

inline void queryosity::selection::counter::finalize(unsigned int) {
  m_cnt.error = std::sqrt(m_cnt.error);
}

inline queryosity::selection::count_t
queryosity::selection::counter::result() const {
  return m_cnt;
}

inline queryosity::selection::count_t
queryosity::selection::counter::merge(std::vector<count_t> const &cnts) const {
  count_t sum;
  for (auto const &cnt : cnts) {
    sum.entries += cnt.entries;
    sum.value += cnt.value;
    sum.error += cnt.error * cnt.error;
  }
  sum.error = std::sqrt(sum.error);
  return sum;
}

template <typename... Sels>
queryosity::selection::cutbookkeeper<Sels...>::cutbookkeeper(
    Sels const &...sels)
    : m_selections(sels...) {}

template <typename... Sels>
auto queryosity::selection::cutbookkeeper<Sels...>::make(dataflow &df) const {
  return std::apply(
      [&df](Sels const &...sels) {
        return df.get(query::output<counter>()).at(sels...);
      },
      m_selections);
}