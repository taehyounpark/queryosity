#include "ana/input.h"


#include "ana/strutils.h"
#include "ana/vecutils.h"

ana::input::range::range() :
	slot(0),
	begin(0),
	end(-1)
{}

ana::input::range::range(size_t slot, long long begin, long long end) :
	slot(slot),
	begin(begin),
	end(end)
{}

long long ana::input::range::entries() const
{
	return end-begin;
}

ana::input::range ana::input::range::operator+(const range& next)
{
	assert(this->end==next.begin);
	return range(this->slot,this->begin,next.end);
}

ana::input::range& ana::input::range::operator+=(const range& next)
{
	assert(this->end==next.begin);
	this->end=next.end;
	return *this;
}

void ana::input::partition::add(size_t islot, long long begin, long long end)
{
	this->parts.push_back(range(islot,begin,end));
}

void ana::input::partition::add(const range& part)
{
	this->parts.push_back(part);
}

ana::input::range ana::input::partition::part(size_t islot) const
{
	return this->parts[islot];
}

ana::input::range ana::input::partition::total() const
{
	return vec::sum(this->parts);
}

size_t ana::input::partition::size() const
{
	return this->parts.size();
}

ana::input::partition ana::input::partition::merge(size_t max_parts) const
{
	if (fixed) return *this;
	partition merged;
	auto groups = vec::group(this->parts,max_parts);
	for (const auto& group : groups) {
		merged.parts.push_back(vec::sum(group));
	}
	return merged;
}

ana::input::partition ana::input::partition::truncate(long long max_entries) const
{
	if (fixed) return *this;
	if (max_entries<0) return *this;
	partition trunced;
	for (const auto& part : this->parts) {
		auto part_end = max_entries >= 0 ? std::min(part.begin+max_entries,part.end) : part.end;
		trunced.parts.push_back(range(part.slot, part.begin, part_end));
		max_entries -= part_end;
		if (!max_entries) break;
	}
	return trunced;
}

ana::input::progress::progress(long long tot) : 
	tot(tot)
{
	prog.store(0);
}

void ana::input::progress::reset()
{
	prog.store(0);
}

ana::input::progress& ana::input::progress::operator++()
{
	prog++;
	return *this;
}

double ana::input::progress::percent() const
{ 
	return double(prog.load()) / double(tot) * 100.0; 
}

bool ana::input::progress::done() const 
{ 
	return prog.load() == tot; 
}