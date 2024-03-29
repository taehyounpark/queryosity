#pragma once

#include "selection.h"

namespace queryosity {

class selection::cut : public selection::node {

public:
  cut(const selection::node *presel, column::variable<double> dec);
  virtual ~cut() = default;

public:
  virtual double calculate() const override;
  virtual bool passed_cut() const override;
  virtual double get_weight() const override;
};

} // namespace queryosity

inline queryosity::selection::cut::cut(const selection::node *presel,
                                       column::variable<double> dec)
    : selection::node(presel, std::move(dec)) {}

inline double queryosity::selection::cut::calculate() const {
  return this->passed_cut();
}

inline bool queryosity::selection::cut::passed_cut() const {
  return this->m_preselection
             ? this->m_preselection->passed_cut() && m_decision.value()
             : m_decision.value();
}

inline double queryosity::selection::cut::get_weight() const {
  return this->m_preselection ? this->m_preselection->get_weight() : 1.0;
}