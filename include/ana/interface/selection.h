#pragma once

#include <memory>

#include "action.h"
#include "column.h"

namespace ana {

class selection;

template <typename T>
constexpr bool is_selection_v = std::is_base_of_v<ana::selection, T>;

class selection : public action {

public:
  class cutflow;

  class cut;
  class weight;

  template <typename T> class applicator;

public:
  static std::string concatenate_names(std::vector<std::string> const &names,
                                       std::string delimiter = "/");

public:
  selection(const std::string &name, bool channel);
  virtual ~selection() = default;

public:
  std::string get_name() const;
  std::string get_path() const;
  std::string get_full_path() const;

  void set_initial();
  void set_previous(const selection &preselection);

  bool is_initial() const;
  const selection *get_previous() const;

  bool is_channel() const noexcept;

  template <typename Val> void set_decision(std::shared_ptr<term<Val>> dec);

  virtual bool passed_cut() const = 0;
  virtual double get_weight() const = 0;

public:
  virtual void initialize() override;
  virtual void execute() override;
  virtual void finalize() override;

private:
  const std::string m_name;
  const bool m_channel;

  const selection *m_preselection;
  std::shared_ptr<column> m_decision;
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

template <typename T> class ana::selection::applicator {

public:
  applicator(std::shared_ptr<T> eqn);
  ~applicator() = default;

  template <typename Sel>
  void set_selection(const std::string &name, bool channel);
  void set_previous(selection const &prev);

  template <typename... Vals>
  std::shared_ptr<selection>
  apply_selection(cell<Vals> const &...columns) const;

protected:
  std::function<std::shared_ptr<selection>()> m_make_shared;
  std::shared_ptr<T> m_equation;
  std::function<void(selection &)> m_set_previous;
};

} // namespace ana

#include "counter.h"
#include "equation.h"

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

inline ana::selection::selection(const std::string &name, bool channel)
    : m_name(name), m_channel(channel), m_preselection(nullptr) {}

inline void ana::selection::set_initial() { m_preselection = nullptr; }

inline void ana::selection::set_previous(const ana::selection &preselection) {
  m_preselection = &preselection;
}

inline bool ana::selection::is_initial() const {
  return m_preselection ? false : true;
}

inline const ana::selection *ana::selection::get_previous() const {
  return m_preselection;
}

inline bool ana::selection::is_channel() const noexcept { return m_channel; }

inline std::string ana::selection::get_name() const { return m_name; }

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

inline void ana::selection::initialize() { m_decision->initialize(); }

inline void ana::selection::execute() { m_decision->execute(); }

inline void ana::selection::finalize() { m_decision->finalize(); }

template <typename T>
void ana::selection::set_decision(std::shared_ptr<term<T>> decision) {
  // keep decision as term
  m_decision = decision;
  // link value to variable<double>
  m_variable = variable<double>(*decision);
}

template <typename T>
ana::selection::applicator<T>::applicator(std::shared_ptr<T> eqn)
    : m_make_shared([]() -> std::shared_ptr<selection> { return nullptr; }),
      m_equation(eqn), m_set_previous([](selection &) { return; }) {}

template <typename T>
template <typename Sel>
void ana::selection::applicator<T>::set_selection(const std::string &name,
                                                  bool channel) {
  m_make_shared = std::bind(
      [](const std::string &name, bool ch) -> std::shared_ptr<selection> {
        return std::make_shared<Sel>(name, ch);
      },
      name, channel);
}

template <typename T>
void ana::selection::applicator<T>::set_previous(
    ana::selection const &previous) {
  m_set_previous = std::bind(
      [](selection &curr, selection const &prev) { curr.set_previous(prev); },
      std::placeholders::_1, std::cref(previous));
}

template <typename T>
template <typename... Vals>
std::shared_ptr<ana::selection> ana::selection::applicator<T>::apply_selection(
    cell<Vals> const &...columns) const {
  // make this selection
  auto sel = m_make_shared();

  // set the previous selection
  m_set_previous(*sel);
  // set the decision
  m_equation->set_arguments(columns...);
  sel->set_decision(
      std::static_pointer_cast<term<cell_value_t<T>>>(m_equation));

  return sel;
}