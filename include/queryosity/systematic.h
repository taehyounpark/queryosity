#pragma once

#include <set>
#include <string>
#include <type_traits>

namespace queryosity {

template <typename T> class lazy;

namespace systematic {

template <typename... Nodes>
auto get_variation_names(Nodes const &...nodes) -> std::set<std::string>;

template <typename Node> class resolver;

template <typename... Args> class variation;

template <typename Lzy> class nominal;

class mode {
public:
  mode();
  ~mode() = default;

  void set_variation_name(const std::string &var_name);

  bool is_nominal() const;
  std::string variation_name() const;

protected:
  bool m_is_nominal;
  std::string m_variation_name;
};

} // namespace systematic

} // namespace queryosity

template <typename... Nodes>
auto queryosity::systematic::get_variation_names(Nodes const &...nodes)
    -> std::set<std::string> {
  std::set<std::string> variation_names;
  (variation_names.merge(nodes.get_variation_names()), ...);
  return variation_names;
}

inline queryosity::systematic::mode::mode()
    : m_is_nominal(true), m_variation_name("") {}

inline void
queryosity::systematic::mode::set_variation_name(const std::string &var_name) {
  m_is_nominal = false;
  ;
  m_variation_name = var_name;
}

inline bool queryosity::systematic::mode::is_nominal() const {
  return m_variation_name.empty();
}
inline std::string queryosity::systematic::mode::variation_name() const {
  return m_variation_name;
}