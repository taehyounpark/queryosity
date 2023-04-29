#include "ana/column.h"

ana::column::column() :
	ana::routine(),
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