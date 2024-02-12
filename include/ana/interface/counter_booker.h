
#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "counter.h"

namespace ana {

template <typename T> class counter::booker {

public:
  using counter_type = T;

  using bkpr_and_cnts_type =
      typename std::pair<std::unique_ptr<bookkeeper<T>>,
                         std::vector<std::unique_ptr<T>>>;

public:
  template <typename... Args> booker(Args... args);
  ~booker() = default;

  // copyable
  booker(const booker &) = default;
  booker &operator=(const booker &) = default;

  template <typename... Vals>
  auto book_fill(term<Vals> const &...cols) const -> std::unique_ptr<booker<T>>;

  auto select_counter(const selection &sel) const -> std::unique_ptr<T>;

  template <typename... Sels>
  auto select_counters(Sels const &...selections) const
      -> std::array<std::unique_ptr<T>, sizeof...(Sels)>;
  // -> bkpr_and_cnts_type;

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
auto ana::counter::booker<T>::select_counter(const selection &sel) const
    -> std::unique_ptr<T> {
  // call constructor
  auto cnt = m_make_unique_counter();
  // fill columns (if set)
  for (const auto &fill_counter : m_fill_columns) {
    fill_counter(*cnt);
  }
  // book cnt at the selection
  cnt->set_selection(sel);
  // return
  return cnt;
}

// template <typename T>
// template <typename... Sels>
// auto ana::counter::booker<T>::select_counters(const Sels
// &...selections)
//     const -> std::array<std::unique_ptr<T>, sizeof...(Sels)> {

//   return std::array<std::unique_ptr<T>, sizeof...(Sels)>{
//       this->select_counter(sels)...};

/*
// make bookkeeper remember selections and gather counters into vector.
auto counter_bookkeeper = std::make_unique<bookkeeper<T>>();
std::vector<std::unique_ptr<T>> booked_counters;
(std::invoke(
   [this, bkpr = counter_bookkeeper.get(),
    &cntr_list = booked_counters](const selection &sel) {
     auto cntr = this->select_counter(sel);
     bkpr->bookkeep(*cntr, sel);
     cntr_list.emplace_back(std::move(cntr));
   },
   selections),
...);

// return a new booker with the selections added
return std::make_pair(std::move(counter_bookkeeper),
                    std::move(booked_counters));
                    */
// }