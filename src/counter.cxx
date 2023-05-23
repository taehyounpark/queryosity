#include "ana/counter.h"

#include "ana/selection.h"

ana::counter::counter() :
	action(),
	m_selection(nullptr),
	m_scale(1.0),
	m_raw(false)
{}

void ana::counter::set_selection(const selection& selection)
{
	m_selection = &selection;
}

const ana::selection* ana::counter::get_selection() const
{
	return m_selection;
}

void ana::counter::apply_scale(double scale)
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