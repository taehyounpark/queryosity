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
class column::conversion : public column::node,
                           column::valued<To>::template converted_from<From> {
public:
  conversion(view<From> const &from);
  virtual ~conversion() = default;
};

} // namespace queryosity

template <typename To, typename From>
queryosity::column::conversion<To, From>::conversion(view<From> const &from)
    : valued<To>::template converted_from<From>(from) {}
