#pragma once

#include <memory>
#include <tuple>

#include "column.h"
#include "column_definition.h"

namespace queryosity {

/**
 * @brief Column definition that converts from one type to another.
 */
template <typename To, typename From>
class column::conversion : public column::definition<To(From)> {
public:
  conversion() = default;
  virtual ~conversion() = default;

  virtual To evaluate(observable<From> from) const override;
};

} // namespace queryosity

template <typename To, typename From>
To queryosity::column::conversion<To, From>::evaluate(
    observable<From> from) const {
  return from.value();
}