#include "ana/column.h"

ana::column::column(const std::string& name) :
	ana::action(name),
	m_required(false)
{}

void ana::column::mark_required(bool required)
{
	m_required = required;
}

bool ana::column::is_required() const
{
	return m_required;
}