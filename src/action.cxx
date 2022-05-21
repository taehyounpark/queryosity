#include "ana/action.h"

ana::action::action(const std::string& name) :
	m_name(name)
{}

const std::string& ana::action::name() const
{
	return m_name;
}