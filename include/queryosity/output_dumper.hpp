#pragma once

namespace queryosity
{

namespace output 
{

template <typename Dumper>
class dumper : public Dumper 
{

public:
  dumper() = default;
  ~dumper() = default;

public:
  template <typename Res>
  void save(Res&& result);

};

}

}