
#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "aggregation.h"

namespace ana {

template <typename T> class aggregation::booker {

public:
  using aggregation_type = T;

  using bkpr_and_cnts_type =
      typename std::pair<std::unique_ptr<bookkeeper<T>>,
                         std::vector<std::unique_ptr<T>>>;

public:
  booker() = default;
  template <typename... Args> booker(Args... args);
  ~booker() = default;

  // copyable
  booker(const booker &) = default;
  booker &operator=(const booker &) = default;

  template <typename... Vals>
  auto book_fill(term<Vals> const &...cols) const -> std::unique_ptr<booker<T>>;

  auto select_aggregation(const selection &sel) const -> std::unique_ptr<T>;

  template <typename... Sels>
  auto select_aggregations(Sels const &...selections) const
      -> bkpr_and_cnts_type;

protected:
  std::unique_ptr<T> make_aggregation();
  template <typename... Vals> void fill_aggregation(term<Vals> const &...cols);

protected:
  std::function<std::unique_ptr<T>()> m_make_unique_aggregation;
  std::vector<std::function<void(T &)>> m_fill_columns;
};

} // namespace ana

template <typename T>
template <typename... Args>
ana::aggregation::booker<T>::booker(Args... args)
    : m_make_unique_aggregation(std::bind(
          [](Args... args) { return std::make_unique<T>(args...); }, args...)) {
}

template <typename T>
template <typename... Vals>
auto ana::aggregation::booker<T>::book_fill(term<Vals> const &...columns) const
    -> std::unique_ptr<booker<T>> {
  // use a fresh one with its current fills
  auto filled = std::make_unique<booker<T>>(*this);
  // add fills
  filled->fill_aggregation(columns...);
  // return new booker
  return filled;
}

template <typename T>
template <typename... Vals>
void ana::aggregation::booker<T>::fill_aggregation(
    term<Vals> const &...columns) {
  // use a snapshot of its current calls
  m_fill_columns.push_back(std::bind(
      [](T &cnt, term<Vals> const &...cols) { cnt.enter_columns(cols...); },
      std::placeholders::_1, std::ref(columns)...));
}

template <typename T>
auto ana::aggregation::booker<T>::select_aggregation(const selection &sel) const
    -> std::unique_ptr<T> {
  // call constructor
  auto cnt = m_make_unique_aggregation();
  // fill columns (if set)
  for (const auto &fill_aggregation : m_fill_columns) {
    fill_aggregation(*cnt);
  }
  // book cnt at the selection
  cnt->set_selection(sel);
  // return
  return cnt;
}

template <typename T>
template <typename... Sels>
auto ana::aggregation::booker<T>::select_aggregations(
    const Sels &...selections) const -> bkpr_and_cnts_type {

  // make bookkeeper remember selections and gather aggregations into vector.
  auto aggregation_bookkeeper = std::make_unique<bookkeeper<T>>();
  std::vector<std::unique_ptr<T>> booked_aggregations;
  (std::invoke(
       [this, bkpr = aggregation_bookkeeper.get(),
        &cntr_list = booked_aggregations](const selection &sel) {
         auto cntr = this->select_aggregation(sel);
         bkpr->bookkeep(*cntr, sel);
         cntr_list.emplace_back(std::move(cntr));
       },
       selections),
   ...);

  // return a new booker with the selections added
  return std::make_pair(std::move(aggregation_bookkeeper),
                        std::move(booked_aggregations));
}