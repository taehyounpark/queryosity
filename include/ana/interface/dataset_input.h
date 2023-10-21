#pragma once

#include "dataset.h"

namespace ana {
namespace dataset {

class source {
public:
  virtual ~source() = default;
  virtual void initialize();
  virtual void finalize();
};

template <typename DS> class input : public source {

public:
  virtual ~input() = default;

  decltype(auto) open_player(const range &part);
  std::unique_ptr<ana::dataset::player> open(const ana::dataset::range &part);

  template <typename Val>
  decltype(auto) read_column(const ana::dataset::range &part,
                             const std::string &name);

  virtual partition allocate() = 0;
  virtual double normalize();
};

} // namespace dataset

} // namespace ana

#include "dataset_player.h"

inline void ana::dataset::source::initialize() {}

inline void ana::dataset::source::finalize() {}

template <typename DS> inline double ana::dataset::input<DS>::normalize() {
  return 1.0;
}

template <typename DS>
decltype(auto)
ana::dataset::input<DS>::open_player(const ana::dataset::range &part) {

  using result_type = decltype(static_cast<DS *>(this)->open(part));
  static_assert(is_unique_ptr_v<result_type>,
                "method must return a std::unique_ptr");

  using player_type = typename result_type::element_type;
  static_assert(std::is_base_of_v<player, player_type>,
                "must be an implementation of dataest player");

  return static_cast<DS *>(this)->open(part);
}

template <typename DS>
template <typename Val>
decltype(auto)
ana::dataset::input<DS>::read_column(const ana::dataset::range &part,
                                     const std::string &name) {

  // using result_type =
  //     decltype(static_cast<DS *>(this)->template read<Val>(part, name));
  // static_assert(is_unique_ptr_v<result_type>,
  //               "method must return a std::unique_ptr");

  // using column_type = typename result_type::element_type;
  // static_assert(std::is_base_of_v<column<Val>, column_type>,
  //               "must be an implementation of dataset column");

  return static_cast<DS *>(this)->template read<Val>(part, name);
}

template <typename DS>
std::unique_ptr<ana::dataset::player>
ana::dataset::input<DS>::open(const ana::dataset::range &) {
  return std::make_unique<ana::dataset::player>();
}