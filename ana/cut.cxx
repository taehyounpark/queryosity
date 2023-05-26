#include "ana/cut.h"

ana::selection::cut::cut(const std::string& name, bool channel) :
	selection(name,channel)
{}

bool ana::selection::cut::passed_cut() const
{
	return m_preselection ? m_preselection->passed_cut() && m_variable.value() : m_variable.value();
}

double ana::selection::cut::get_weight() const
{
	return m_preselection ? m_preselection->get_weight() : 1.0;
}

ana::selection::a_or_b::a_or_b(const selection& a, const selection& b) :
	selection("("+a.get_path()+")||("+b.get_path()+")",false),
	m_a(a),
	m_b(b)
{}

bool ana::selection::a_or_b::passed_cut() const
{
	return m_a.passed_cut() || m_b.passed_cut();
}

double ana::selection::a_or_b::get_weight() const
{
	return 1.0;	
}

ana::selection::a_and_b::a_and_b(const selection& a, const selection& b) :
	selection("("+a.get_path()+")&&("+b.get_path()+")",false),
	m_a(a),
	m_b(b)
{}

bool ana::selection::a_and_b::passed_cut() const
{
	return m_a.passed_cut() && m_b.passed_cut();
}

double ana::selection::a_and_b::get_weight() const
{
	return 1.0;	
}
