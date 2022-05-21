#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include "ana/counter.h"
#include "ana/cutflow.h"

namespace ana
{

class counter::experiment : public selection::cutflow
{

public:
	experiment(double scale);
	~experiment() = default;

public:
	template <typename Cnt, typename... Args>
	std::shared_ptr<booker<Cnt>> count(const std::string& name, const Args&... args);

	template <typename Cnt>
	std::shared_ptr<Cnt> book(const booker<Cnt>& booker);

protected:
	void addcounter(counter& cnt);
	void clearcounters();

protected:
	std::vector<counter*> m_counters;
	double m_norm;

};

}

template <typename Cnt, typename... Args>
std::shared_ptr<ana::counter::booker<Cnt>> ana::counter::experiment::count(const std::string& name, const Args&... args)
{
	auto bkr = std::make_shared<booker<Cnt>>(name,args...);
	return bkr;
}

template <typename Cnt>
std::shared_ptr<Cnt> ana::counter::experiment::book(const booker<Cnt>& booker)
{
	auto cnt = booker.book_selection(*m_latest);
	cnt->applyScale(m_norm);
	this->addcounter(*cnt);
	return cnt;
}