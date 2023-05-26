#pragma once


#include "ana/selection.h"

namespace ana
{

class selection::cut : public selection
{

public:
	cut(const std::string& name, bool channel);
  virtual ~cut() = default;

public:
	virtual bool   passed_cut() const override;
	virtual double get_weight() const override;

};

class selection::a_or_b : public selection
{

public:
	a_or_b(const selection& a, const selection& b);
	virtual ~a_or_b() = default;
  
	virtual bool   passed_cut() const override;
	virtual double get_weight() const override;

protected:
	const selection& m_a;
	const selection& m_b;

};

class selection::a_and_b : public selection
{

public:
	a_and_b(const selection& a, const selection& b);
	virtual ~a_and_b() = default;

	virtual bool   passed_cut() const override;
	virtual double get_weight() const override;

protected:
	const selection& m_a;
	const selection& m_b;

};

}
