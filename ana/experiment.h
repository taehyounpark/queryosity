#pragma once

#include <string>
#include <vector>
#include <memory>

#include "ana/counter.h"
#include "ana/cutflow.h"

namespace ana
{

template <typename Cnt> struct is_counter_booker: std::false_type {};
template <typename Cnt> struct is_counter_booker<counter::booker<Cnt>>: std::true_type {};
template <typename Cnt> constexpr bool is_counter_booker_v = is_counter_booker<Cnt>::value;

class counter::experiment : public selection::cutflow
{

public:
	experiment(double scale);
	~experiment() = default;

public:
	template <typename Cnt, typename... Args>
	std::shared_ptr<booker<Cnt>> book(Args&&... args);

	template <typename Cnt, typename Sel>
	std::shared_ptr<Cnt> count(booker<Cnt>& bkr, Sel const& sel);

	template <typename Cnt>
	auto repeat_counter(counter::booker<Cnt> const& bkr) const -> std::shared_ptr<counter::booker<Cnt>>;

	void clear_counters();

protected:
	void add_counter(counter& cnt);

protected:
	std::vector<counter*> m_counters;
	double m_norm;

};

}

template <typename Cnt, typename... Args>
std::shared_ptr<ana::counter::booker<Cnt>> ana::counter::experiment::book(Args&&... args)
{
	auto bkr = std::make_shared<booker<Cnt>>(std::forward<Args>(args)...);
	return bkr;
}

template <typename Cnt, typename Sel>
std::shared_ptr<Cnt> ana::counter::experiment::count(booker<Cnt>& bkr, Sel const& sel)
{
	auto cnt = bkr.book_counter_at(sel);
	cnt->set_scale(m_norm);
	this->add_counter(*cnt);
	return cnt;
}

template <typename Cnt>
auto ana::counter::experiment::repeat_counter(counter::booker<Cnt> const& bkr) const -> std::shared_ptr<counter::booker<Cnt>>
{
	// a single booker always requires a one-to-map between booked selection path and counter instance
	// so if a new variation is needed, the only way is to make a new copy of the booker
	// then varied counter bookers can each book a counter at given selection path
	// a trivial copy constructor (that copies the constructor and fill call functions) does the job
	return std::make_shared<counter::booker<Cnt>>(bkr);
}