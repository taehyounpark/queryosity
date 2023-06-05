#pragma once

#include "aggregation.h"

namespace ana {

class selection;

template <typename Cnt> class aggregation::bookkeeper {

public:
  using aggregation_type = Cnt;

public:
  bookkeeper() = default;
  ~bookkeeper() = default;

  bookkeeper(const bookkeeper &) = default;
  bookkeeper &operator=(const bookkeeper &) = default;

  void bookkeep(Cnt &cnt, const selection &sel);

  std::set<std::string> list_selection_paths() const;
  Cnt *get_aggregation(const std::string &path) const;

protected:
  std::set<std::string> m_booked_selection_paths;
  std::unordered_map<std::string, Cnt *> m_booked_aggregation_map;
};

} // namespace ana

#include "selection.h"

template <typename Cnt>
void ana::aggregation::bookkeeper<Cnt>::bookkeep(Cnt &cnt,
                                                 const selection &sel) {
  // check if booking makes sense
  if (m_booked_aggregation_map.find(sel.get_path()) !=
      m_booked_aggregation_map.end()) {
    throw std::logic_error("aggregation already booked at selection");
  }
  m_booked_selection_paths.insert(sel.get_path());
  m_booked_aggregation_map.insert(std::make_pair(sel.get_path(), &cnt));
}

template <typename T>
T *ana::aggregation::bookkeeper<T>::get_aggregation(
    const std::string &selection_path) const {
  if (m_booked_aggregation_map.find(selection_path) ==
      m_booked_aggregation_map.end()) {
    throw std::out_of_range("aggregation not booked at selection path");
  }
  return m_booked_aggregation_map.at(selection_path);
}
