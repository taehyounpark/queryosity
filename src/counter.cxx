#include "ana/counter.h"

#include "ana/selection.h"
#include "ana/strutils.h"

ana::counter::counter(const std::string& name) :
	ana::action(name),
	m_selection(nullptr),
	m_scale(1.0),
	m_raw(false)
{}

void ana::counter::set_selection(const selection& selection)
{
	m_selection = &selection;
}

std::string ana::counter::path() const
{
	return str::ensure_trailing(m_selection->path(),"/")+this->name();
}

void ana::counter::applyScale(double scale)
{
	m_scale *= scale;
}

void ana::counter::use_weight(bool use)
{
	m_raw = !use;
}

void ana::counter::initialize()
{}

void ana::counter::execute()
{}

void ana::counter::finalize()
{}