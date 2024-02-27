
#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "counter.h"

namespace ana {

template <typename T> class counter::booker {

public:
  using counter_type = T;

public:
  template <typename... Args> booker(Args... args);
  ~booker() = default;

  // copyable
  booker(const booker &) = default;
  booker &operator=(const booker &) = default;

  template <typename... Vals>
  auto book_fill(term<Vals> const &...cols) const -> std::unique_ptr<booker<T>>;

  auto set_selection(const selection &sel) const -> std::unique_ptr<T>;

protected:
  std::unique_ptr<T> make_counter();
  template <typename... Vals> void fill_counter(term<Vals> const &...cols);

protected:
  std::function<std::unique_ptr<T>()> m_make_unique_counter;
  std::vector<std::function<void(T &)>> m_fill_columns;
};

} // namespace ana

template <typename T>
template <typename... Args>
ana::counter::booker<T>::booker(Args... args)
    : m_make_unique_counter(std::bind(
          [](Args... args) { return std::make_unique<T>(args...); }, args...)) {
}

template <typename T>
template <typename... Vals>
auto ana::counter::booker<T>::book_fill(term<Vals> const &...columns) const
    -> std::unique_ptr<booker<T>> {
  // use a fresh one with its current fills
  auto filled = std::make_unique<booker<T>>(*this);
  // add fills
  filled->fill_counter(columns...);
  // return new booker
  return filled;
}

template <typename T>
template <typename... Vals>
void ana::counter::booker<T>::fill_counter(term<Vals> const &...columns) {
  // use a snapshot of its current calls
  m_fill_columns.push_back(std::bind(
      [](T &cnt, term<Vals> const &...cols) { cnt.enter_columns(cols...); },
      std::placeholders::_1, std::ref(columns)...));
}

template <typename T>
auto ana::counter::booker<T>::set_selection(const selection &sel) const
    -> std::unique_ptr<T> {
  // call constructor
  auto cnt = m_make_unique_counter();
  // fill columns (if set)
  for (const auto &fill_counter : m_fill_columns) {
    fill_counter(*cnt);
  }
  // book cnt at the selection
  cnt->set_selection(sel);
  return cnt;
}