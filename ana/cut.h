#pragma once


#include "ana/selection.h"

namespace ana
{

class selection::cut : public selection
{

public:
	template <typename T>
	class evaluator;

public:
	cut(const std::string& name);
  virtual ~cut() = default;

public:
	virtual bool   passed_cut() const override;
	virtual double get_weight() const override;

};

}
