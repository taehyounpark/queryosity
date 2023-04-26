#include "ana/cutflow.h"

#include "ana/strutils.h"

ana::selection::cutflow::cutflow() //:
	// m_latest(nullptr)
{}

ana::selection::cutflow& ana::selection::cutflow::rebase(const ana::selection& sel) noexcept
{
	// m_latest = &sel;
	return *this;
}

void ana::selection::cutflow::add_selection(ana::selection& sel)
{
	m_selections.push_back(&sel);
	// m_latest = &sel;
}