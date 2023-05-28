#pragma once

#include "selection.h"

namespace ana {

class selection::cut : public selection {

public:
  class a_or_b;
  class a_and_b;

public:
  cut(const std::string &name, bool channel);
  virtual ~cut() = default;

public:
  virtual bool passed_cut() const override;
  virtual double get_weight() const override;
};

class selection::cut::a_or_b : public selection::cut {

public:
  a_or_b(const selection &a, const selection &b);
  virtual ~a_or_b() = default;

  virtual bool passed_cut() const override;
  virtual double get_weight() const override;

protected:
  const selection &m_a;
  const selection &m_b;
};

class selection::cut::a_and_b : public selection::cut {

public:
  a_and_b(const selection &a, const selection &b);
  virtual ~a_and_b() = default;

  virtual bool passed_cut() const override;
  virtual double get_weight() const override;

protected:
  const selection &m_a;
  const selection &m_b;
};

} // namespace ana

inline ana::selection::cut::cut(const std::string &name, bool channel)
    : selection(name, channel) {}

inline bool ana::selection::cut::passed_cut() const {
  return m_preselection ? m_preselection->passed_cut() && m_variable.value()
                        : m_variable.value();
}

inline double ana::selection::cut::get_weight() const {
  return m_preselection ? m_preselection->get_weight() : 1.0;
}

inline ana::selection::cut::a_or_b::a_or_b(const selection &a,
                                           const selection &b)
    : cut("(" + a.get_path() + ")||(" + b.get_path() + ")", false), m_a(a),
      m_b(b) {}

inline bool ana::selection::cut::a_or_b::passed_cut() const {
  return m_a.passed_cut() || m_b.passed_cut();
}

inline double ana::selection::cut::a_or_b::get_weight() const { return 1.0; }

inline ana::selection::cut::a_and_b::a_and_b(const selection &a,
                                             const selection &b)
    : cut("(" + a.get_path() + ")&&(" + b.get_path() + ")", false), m_a(a),
      m_b(b) {}

inline bool ana::selection::cut::a_and_b::passed_cut() const {
  return m_a.passed_cut() && m_b.passed_cut();
}

inline double ana::selection::cut::a_and_b::get_weight() const { return 1.0; }
