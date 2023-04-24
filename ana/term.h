#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <tuple>
#include <memory>
#include <type_traits>

#include "ana/column.h"

namespace ana
{

// -----------------------------------------------------------------------------
// term<T>: column whose value of type <T> can be observed
// -----------------------------------------------------------------------------
template <typename T>
class term : public column, public cell<T>
{

public:
  using value_type = typename cell<T>::value_type;

public:
  term(const std::string& name);
  virtual ~term() = default;

  virtual void initialize() override;
  virtual void execute() override;
  virtual void finalize() override;

public:
  class constant;

  class reader;

  class calculation;

  template <typename... Args>
  class defined_from;

  template <typename... Args>
  class evaluated_from;

};

}

template <typename T>
ana::term<T>::term(const std::string& name) : 
  column(name),
  cell<T>()
{}

template <typename T>
void ana::term<T>::initialize()
{}

template <typename T>
void ana::term<T>::execute()
{}

template <typename T>
void ana::term<T>::finalize()
{}