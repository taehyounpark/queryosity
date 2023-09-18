#pragma once

#include <memory>
#include <string>
#include <vector>

#include "column.h"
#include "operation.h"

namespace ana {

class selection;

template <typename T>
constexpr bool is_selection_v = std::is_base_of_v<ana::selection, T>;

// class selection : public operation {
class selection : public column::calculation<double> {

public:
  class cutflow;

  class cut;
  class weight;

  template <typename T> class applicator;

public:
  static std::string concatenate_names(std::vector<std::string> const &names,
                                       std::string delimiter = "/");

public:
  selection(const selection *presel, bool ch, const std::string &name);
  virtual ~selection() = default;

public:
  std::string get_name() const noexcept;
  std::string get_path() const;
  std::string get_full_path() const;

  bool is_initial() const noexcept;
  bool is_channel() const noexcept;
  const selection *get_previous() const noexcept;

  template <typename T> void set_decision(std::unique_ptr<T> dec);

  virtual bool passed_cut() const = 0;
  virtual double get_weight() const = 0;

public:
  virtual double calculate() const override;

  virtual void initialize(const dataset::range &part) override;
  virtual void execute(const dataset::range &part,
                       unsigned long long entry) override;
  virtual void finalize(const dataset::range &part) override;

private:
  const selection *const m_preselection;
  const bool m_channel;
  const std::string m_name;

  std::unique_ptr<column> m_decision;
  ana::variable<double> m_variable;

public:
  template <typename T> struct is_applicator : std::false_type {};
  template <typename T>
  struct is_applicator<selection::applicator<T>> : std::true_type {};
  template <typename T>
  static constexpr bool is_applicator_v = is_applicator<T>::value;

  template <typename F>
  using custom_applicator_t = typename selection::template applicator<
      ana::column::template equation_t<F>>;
  using trivial_applicator_type = typename selection::template applicator<
      ana::column::equation<double(double)>>;
};

} // namespace ana

#include "aggregation.h"
#include "column_equation.h"

inline std::string
ana::selection::concatenate_names(std::vector<std::string> const &names,
                                  std::string delimiter) {
  std::string joined;
  for (auto const &name : names) {
    joined += name;
    joined += delimiter;
  }
  return joined;
}

inline ana::selection::selection(const selection *presel, bool ch,
                                 const std::string &name)
    : m_preselection(presel), m_channel(ch), m_name(name) {}

inline bool ana::selection::is_initial() const noexcept {
  return m_preselection ? false : true;
}

inline const ana::selection *ana::selection::get_previous() const noexcept {
  return m_preselection;
}

inline bool ana::selection::is_channel() const noexcept { return m_channel; }

inline std::string ana::selection::get_name() const noexcept { return m_name; }

inline std::string ana::selection::get_path() const {
  std::vector<std::string> channels;
  const selection *presel = this->get_previous();
  while (presel) {
    if (presel->is_channel())
      channels.push_back(presel->get_name());
    presel = presel->get_previous();
  }
  std::reverse(channels.begin(), channels.end());
  return concatenate_names(channels, "/") + this->get_name();
}

inline std::string ana::selection::get_full_path() const {
  std::vector<std::string> presels;
  const selection *presel = this->get_previous();
  while (presel) {
    presels.push_back(presel->get_name());
    presel = presel->get_previous();
  }
  std::reverse(presels.begin(), presels.end());
  return concatenate_names(presels, "/") + this->get_name();
}

inline double ana::selection::calculate() const {
  return m_variable.value();
}

inline void ana::selection::initialize(const ana::dataset::range &part) {
  m_decision->initialize(part);
}

inline void ana::selection::execute(const ana::dataset::range &part,
                                    unsigned long long entry) {
  ana::column::calculation<double>::execute(part,entry);
  m_decision->execute(part, entry);
}

inline void ana::selection::finalize(const ana::dataset::range &part) {
  m_decision->finalize(part);
}

template <typename T>
void ana::selection::set_decision(std::unique_ptr<T> decision) {
  // link value to variable<double>
  m_variable = variable<double>((term<cell_value_t<T>> &)(*decision));
  // keep decision as term
  m_decision = std::move(decision);
}