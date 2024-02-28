#pragma once

#include "selection.h"

namespace ana {

class selection::weight : public selection::node {

public:
  class a_times_b;

public:
  weight(const selection::node *presel);
  virtual ~weight() = default;

public:
  virtual double calculate() const override;
  virtual bool passed_cut() const override;
  virtual double get_weight() const override;
};

} // namespace ana

inline ana::selection::weight::weight(const selection::node *presel)
    : selection::node(presel) {}

inline double ana::selection::weight::calculate() const {
  return this->m_preselection
             ? this->m_preselection->get_weight() * m_variable.value()
             : m_variable.value();
}

inline bool ana::selection::weight::passed_cut() const {
  return this->m_preselection ? this->m_preselection->passed_cut() : true;
}

inline double ana::selection::weight::get_weight() const {
  return this->value();
}