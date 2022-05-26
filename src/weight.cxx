#include "ana/weight.h"

ana::selection::weight::weight(const std::string& name) :
	selection(name)
{}

bool ana::selection::weight::passed_cut() const
{
	return m_preselection ? m_preselection->passed_cut() : true;
}

double ana::selection::weight::get_weight() const
{
	return m_preselection ? m_preselection->get_weight() * m_variable.value() : m_variable.value();
}