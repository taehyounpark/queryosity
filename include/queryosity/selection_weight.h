#pragma once

#include "selection.h"

namespace queryosity {

class selection::weight : public selection::node {

public:
  class a_times_b;

public:
  weight(const selection::node *presel, column::variable<double> dec);
  virtual ~weight() = default;

public:
  virtual double calculate() const override;
  virtual bool passed_cut() const override;
  virtual double get_weight() const override;
};

} // namespace queryosity

inline queryosity::selection::weight::weight(const selection::node *presel,
                                             column::variable<double> dec)
    : selection::node(presel, std::move(dec)) {}

inline double queryosity::selection::weight::calculate() const {
  return this->get_weight();
}

inline bool queryosity::selection::weight::passed_cut() const {
  return this->m_preselection ? this->m_preselection->passed_cut() : true;
}

inline double queryosity::selection::weight::get_weight() const {
  return this->m_preselection
             ? this->m_preselection->get_weight() * m_decision.value()
             : m_decision.value();
}