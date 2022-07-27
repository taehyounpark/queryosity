#include "ana/action.h"

ana::action::action(const std::string& name) :
	m_name(name)
{}

const std::string& ana::action::get_name() const
{
	return m_name;
}