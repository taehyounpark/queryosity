#pragma once

#include "counter.h"

namespace ana {

class selection;

template <typename Cnt> class counter::bookkeeper {

public:
  using counter_type = Cnt;

public:
  bookkeeper() = default;
  ~bookkeeper() = default;

  bookkeeper(const bookkeeper &) = default;
  bookkeeper &operator=(const bookkeeper &) = default;

  void bookkeep(Cnt &cnt, const selection &sel);

  std::set<std::string> list_selection_paths() const;
  Cnt *get_counter(const std::string &path) const;

protected:
  std::set<std::string> m_booked_selection_paths;
  std::unordered_map<std::string, Cnt *> m_booked_counter_map;
};

} // namespace ana

#include "selection.h"

template <typename Cnt>
void ana::counter::bookkeeper<Cnt>::bookkeep(Cnt &cnt, const selection &sel) {
  // check if booking makes sense
  if (m_booked_counter_map.find(sel.get_path()) != m_booked_counter_map.end()) {
    throw std::logic_error("counter already booked at selection");
  }
  m_booked_selection_paths.insert(sel.get_path());
  m_booked_counter_map.insert(std::make_pair(sel.get_path(), &cnt));
}

template <typename T>
T *ana::counter::bookkeeper<T>::get_counter(
    const std::string &selection_path) const {
  if (m_booked_counter_map.find(selection_path) == m_booked_counter_map.end()) {
    throw std::out_of_range("counter not booked at selection path");
  }
  return m_booked_counter_map.at(selection_path);
}
