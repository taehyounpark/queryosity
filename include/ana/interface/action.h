#pragma once

#include "systematic.h"

namespace ana {

/**
 * @class action
 * @brief abstract base class with initialization, execution, finalization steps
 */
class action {

public:
  action() = default;
  virtual ~action() = default;

  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) = 0;
  virtual void execute(unsigned int slot, unsigned long long entry) = 0;
  virtual void finalize(unsigned int slot) = 0;

  systematic::mode &systematic_mode();
  systematic::mode const &systematic_mode() const;

protected:
  systematic::mode m_syst_mode;
};

} // namespace ana

inline ana::systematic::mode &ana::action::systematic_mode() {
  return m_syst_mode;
}

inline ana::systematic::mode const &ana::action::systematic_mode() const {
  return m_syst_mode;
}