#include "ana/selection.h"

#include "ana/counter.h"
#include "ana/strutils.h"

ana::selection::selection(const std::string& name) :
	ana::action(name),
	m_preselection(nullptr),
	m_channel(false)
{}

void ana::selection::set_initial()
{
	m_preselection = nullptr;
}

void ana::selection::set_previous(const ana::selection& preselection)
{
	m_preselection = &preselection;
}

bool ana::selection::is_initial() const
{
	return m_preselection ? false : true;
}

const ana::selection* ana::selection::get_previous() const
{
	return m_preselection;
}

void ana::selection::set_channel(bool channel)
{
	m_channel = channel;
}

bool ana::selection::is_channel() const noexcept
{
	return m_channel;
}

std::string ana::selection::path() const
{
	std::vector<std::string> channels;
	const selection* presel = this->get_previous();
	while (presel) {
		if (presel->is_channel()) channels.push_back(presel->name());
		presel = presel->get_previous();
	}
	std::reverse(channels.begin(),channels.end());
	return ( channels.size() ? str::join(channels,"/")+"/"+this->name() : this->name() );
}

void ana::selection::initialize()
{
	m_selectionDecision->initialize();
}

void ana::selection::execute()
{
	m_selectionDecision->execute();
}

void ana::selection::finalize()
{
	m_selectionDecision->finalize();
}