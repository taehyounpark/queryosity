#pragma once

#include "selection.h"

namespace ana {

class selection::cut : public selection {

public:
  cut(const selection *presel);
  virtual ~cut() = default;

public:
  virtual double calculate() const override;
  virtual bool passed_cut() const override;
  virtual double get_weight() const override;
};

} // namespace ana

inline ana::selection::cut::cut(const selection *presel) : selection(presel) {}

inline double ana::selection::cut::calculate() const {
  return this->m_preselection
             ? this->m_preselection->passed_cut() && m_variable.value()
             : m_variable.value();
}

inline bool ana::selection::cut::passed_cut() const { return this->value(); }

inline double ana::selection::cut::get_weight() const {
  return this->m_preselection ? this->m_preselection->get_weight() : 1.0;
}