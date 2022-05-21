
#pragma once

#include <memory>
#include <tuple>
#include <type_traits>

#include "ana/cell.h"
#include "ana/column.h"
#include "ana/table.h"

namespace ana
{

//------------------------------------------------------------------------------
// Tree
//------------------------------------------------------------------------------

// template <typename Val>
// template <typename Dat>
// class column<Val>::Reader : public column<Val>, public Dat::Reader

// template <typename Val>
// template <typename Rdr>
// class column<Val>::reader : public column<Val>
// {
// public:
//   template <typename... Args>
//   Reader(const std::string& name, std::unique_ptr<Rdr<Val>>) : 
//     column<Val>(name)
//   {}
//   virtual ~Reader() = default;

// protected:
//   std::unique_ptr<Rdr<Val>> m_reader;

// };

template <typename Val>
template <typename Dat>
class column<Val>::Reader : public column<Val>, public table::Reader<Dat, Val>
{

public:
  template <typename... Args>
  Reader(const std::string& name, Args... args) : 
    column<Val>(name),
    table::Reader<Dat, Val>(args...)
  {}
  virtual ~Reader() = default;

  const Val& value() const override
  {
    return this->get_entry();
  }

  void initialize() override
  {
    this->open_column();
  }

  void execute() override
  {
    this->next_entry();
  }

  void finalize() override
  {}

protected:

};

}