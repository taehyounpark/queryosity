#include "ana/experiment.h"

#include <exception>

ana::counter::experiment::experiment(double norm) :
	m_norm(norm)
{}

void ana::counter::experiment::add_counter(ana::counter& cnt)
{
	m_counters.push_back(&cnt);
}

void ana::counter::experiment::clear_counters()
{
	m_counters.clear();
}