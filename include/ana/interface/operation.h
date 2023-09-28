#pragma once

namespace ana {

namespace dataset {
struct range;
}

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
};

} // namespace ana