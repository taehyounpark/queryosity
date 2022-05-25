#include "ana/cut.h"

ana::selection::cut::cut(const std::string& name) :
	selection(name)
{}

bool ana::selection::cut::passed_cut() const
{
	return m_preselection ? m_preselection->passed_cut() && m_value->value() : m_value->value();
}

double ana::selection::cut::get_weight() const
{
	return m_preselection ? m_preselection->get_weight() : 1.0;
}
