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
	std::shared_ptr<bookkeeper<Cnt>> count(const std::string& name, const Args&... args);

	template <typename Cnt>
	std::shared_ptr<Cnt> book(const bookkeeper<Cnt>& bookkeeper);

	void clear_counters();

protected:
	void add(counter& cnt);

protected:
	std::vector<counter*> m_counters;
	double m_norm;

};

}

template <typename Cnt, typename... Args>
std::shared_ptr<ana::counter::bookkeeper<Cnt>> ana::counter::experiment::count(const std::string& name, const Args&... args)
{
	auto bkr = std::make_shared<bookkeeper<Cnt>>(name,args...);
	return bkr;
}

template <typename Cnt>
std::shared_ptr<Cnt> ana::counter::experiment::book(const bookkeeper<Cnt>& bookkeeper)
{
	auto cnt = bookkeeper.book_selection(*m_latest);
	cnt->set_scale(m_norm);
	this->add(*cnt);
	return cnt;
}