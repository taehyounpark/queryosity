
#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "query.h"

namespace queryosity {

template <typename T> class query::booker {

public:
  using query_type = T;

public:
  template <typename... Args> booker(Args... args);
  ~booker() = default;

  // copyable
  booker(const booker &) = default;
  booker &operator=(const booker &) = default;

  template <typename... Vals>
  auto add_columns(column::valued<Vals> const &...cols) const
      -> std::unique_ptr<booker<T>>;

  auto set_selection(const selection::node &sel) const -> std::unique_ptr<T>;

protected:
  std::unique_ptr<T> make_query();
  template <typename... Vals>
  void fill_query(column::valued<Vals> const &...cols);

protected:
  std::function<std::unique_ptr<T>()> m_make_unique_query;
  std::vector<std::function<void(T &)>> m_add_columns;
};

} // namespace queryosity

template <typename T>
template <typename... Args>
queryosity::query::booker<T>::booker(Args... args)
    : m_make_unique_query(std::bind(
          [](Args... args) { return std::make_unique<T>(args...); }, args...)) {
}

template <typename T>
template <typename... Vals>
auto queryosity::query::booker<T>::add_columns(
    column::valued<Vals> const &...columns) const -> std::unique_ptr<booker<T>> {
  // use a fresh one with its current fills
  auto filled = std::make_unique<booker<T>>(*this);
  // add fills
  filled->fill_query(columns...);
  // return new book
  return filled;
}

template <typename T>
template <typename... Vals>
void queryosity::query::booker<T>::fill_query(
    column::valued<Vals> const &...columns) {
  // use a snapshot of its current calls
  m_add_columns.push_back(std::bind(
      [](T &cnt, column::valued<Vals> const &...cols) {
        cnt.enter_columns(cols...);
      },
      std::placeholders::_1, std::cref(columns)...));
}

template <typename T>
auto queryosity::query::booker<T>::set_selection(const selection::node &sel) const
    -> std::unique_ptr<T> {
  // call constructor
  auto cnt = m_make_unique_query();
  // fill columns (if set)
  for (const auto &fill_query : m_add_columns) {
    fill_query(*cnt);
  }
  // book cnt at the selection
  cnt->set_selection(sel);
  return cnt;
}