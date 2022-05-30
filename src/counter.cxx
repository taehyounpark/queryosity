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

std::string ana::counter::full_path() const
{
	return str::ensure_trailing(m_selection->full_path(),"/")+this->name();
}

void ana::counter::set_scale(double scale)
{
	m_scale *= scale;
}

void ana::counter::use_weight(bool use)
{
	m_raw = !use;
}

void ana::counter::initialize()
{
	if (!m_selection) throw std::runtime_error("no booked selection");
}

void ana::counter::execute()
{
	if (m_selection->passed_cut()) this->count(m_raw ? 1.0 : m_scale * m_selection->get_weight());
}


void ana::counter::finalize()
{}