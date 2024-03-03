
#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "query.h"

namespace queryosity {

template <typename T> class query::book {

public:
  using query_type = T;

public:
  template <typename... Args> book(Args... args);
  ~book() = default;

  // copyable
  book(const book &) = default;
  book &operator=(const book &) = default;

  template <typename... Vals>
  auto book_fill(column::cell<Vals> const &...cols) const
      -> std::unique_ptr<book<T>>;

  auto set_selection(const selection::node &sel) const -> std::unique_ptr<T>;

protected:
  std::unique_ptr<T> make_query();
  template <typename... Vals>
  void fill_query(column::cell<Vals> const &...cols);

protected:
  std::function<std::unique_ptr<T>()> m_make_unique_query;
  std::vector<std::function<void(T &)>> m_fill_columns;
};

} // namespace queryosity

template <typename T>
template <typename... Args>
queryosity::query::book<T>::book(Args... args)
    : m_make_unique_query(std::bind(
          [](Args... args) { return std::make_unique<T>(args...); }, args...)) {
}

template <typename T>
template <typename... Vals>
auto queryosity::query::book<T>::book_fill(
    column::cell<Vals> const &...columns) const -> std::unique_ptr<book<T>> {
  // use a fresh one with its current fills
  auto filled = std::make_unique<book<T>>(*this);
  // add fills
  filled->fill_query(columns...);
  // return new book
  return filled;
}

template <typename T>
template <typename... Vals>
void queryosity::query::book<T>::fill_query(
    column::cell<Vals> const &...columns) {
  // use a snapshot of its current calls
  m_fill_columns.push_back(std::bind(
      [](T &cnt, column::cell<Vals> const &...cols) {
        cnt.enter_columns(cols...);
      },
      std::placeholders::_1, std::ref(columns)...));
}

template <typename T>
auto queryosity::query::book<T>::set_selection(const selection::node &sel) const
    -> std::unique_ptr<T> {
  // call constructor
  auto cnt = m_make_unique_query();
  // fill columns (if set)
  for (const auto &fill_query : m_fill_columns) {
    fill_query(*cnt);
  }
  // book cnt at the selection
  cnt->set_selection(sel);
  return cnt;
}