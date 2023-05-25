#include "ana/cutflow.h"

void ana::selection::cutflow::add_selection(ana::selection& sel)
{
	m_selections.push_back(&sel);
}