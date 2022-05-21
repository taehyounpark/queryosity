#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <tuple>
#include <memory>
#include <type_traits>

#include "ana/cell.h"

namespace ana
{

// -----------------------------------------------------------------------------
// column<T>: column whose value of type <T> can be observed
// -----------------------------------------------------------------------------
template <typename T>
class column : public variable, public cell<T>
{

public:
  using value_type = typename cell<T>::value_type;

public:
  column(const std::string& name);
  virtual ~column() = default;

  virtual void initialize() override;
  virtual void execute() override;
  virtual void finalize() override;

public:
  class constant;

  template <typename Dat>
  class Reader;

  class calculation;

  template <typename... Args>
  class definition;

  template <typename... Args>
  class equation;

};

}

template <typename T>
ana::column<T>::column(const std::string& name) : 
  variable(name),
  cell<T>()
{}

template <typename T>
void ana::column<T>::initialize()
{}

template <typename T>
void ana::column<T>::execute()
{}

template <typename T>
void ana::column<T>::finalize()
{}