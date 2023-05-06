#pragma once


#include "ana/selection.h"

namespace ana
{

class selection::weight : public selection
{

public:
	weight(const std::string& name);
  virtual ~weight() = default;

public:
	virtual bool   passed_cut() const override;
	virtual double get_weight() const override;

};

}