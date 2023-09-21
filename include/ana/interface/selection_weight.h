#pragma once

#include "selection.h"

namespace ana {

class selection::weight : public selection {

public:
  class a_times_b;

public:
  weight(const selection *presel, bool ch, const std::string &name);
  virtual ~weight() = default;

public:
  virtual bool passed_cut() const override;
  virtual double get_weight() const override;
};

class selection::weight::a_times_b : public selection {

public:
  a_times_b(const selection &a, const selection &b);
  virtual ~a_times_b() = default;

  virtual bool passed_cut() const override;
  virtual double get_weight() const override;

protected:
  const selection &m_a;
  const selection &m_b;
};

} // namespace ana

inline ana::selection::weight::weight(const selection *presel, bool ch,
                                      const std::string &name)
    : selection(presel, ch, name) {}

inline bool ana::selection::weight::passed_cut() const {
  return m_preselection ? m_preselection->passed_cut() : true;
}

inline double ana::selection::weight::get_weight() const {
  return m_preselection ? m_preselection->passed_cut() * this->value()
                        : this->value();
}

inline ana::selection::weight::a_times_b::a_times_b(const selection &a,
                                                    const selection &b)
    : selection(nullptr, false,
                "(" + a.get_path() + ")*(" + b.get_path() + ")"),
      m_a(a), m_b(b) {}

inline bool ana::selection::weight::a_times_b::passed_cut() const {
  return m_a.passed_cut() && m_b.passed_cut();
}

inline double ana::selection::weight::a_times_b::get_weight() const {
  return m_a.get_weight() * m_b.get_weight();
}