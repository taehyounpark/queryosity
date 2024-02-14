#pragma once

#include "systematic.h"

namespace ana {

/**
 * @class operation
 * @brief abstract base class with initialization, execution, finalization steps
 */
class operation {

public:
  operation() = default;
  virtual ~operation() = default;

  virtual void initialize(const ana::dataset::range &part) = 0;
  virtual void execute(const ana::dataset::range &part,
                       unsigned long long entry) = 0;
  virtual void finalize(const ana::dataset::range &part) = 0;

  systematic::mode &systematic_mode();
  systematic::mode const &systematic_mode() const;

protected:
  systematic::mode m_syst_mode;
};

} // namespace ana

inline ana::systematic::mode &ana::operation::systematic_mode() {
  return m_syst_mode;
}

inline ana::systematic::mode const &ana::operation::systematic_mode() const {
  return m_syst_mode;
}