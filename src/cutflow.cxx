#include "ana/cutflow.h"

#include "ana/strutils.h"

ana::selection::cutflow::cutflow() //:
{}

void ana::selection::cutflow::add_selection(ana::selection& sel)
{
	m_selections.push_back(&sel);
	// m_latest = &sel;
}