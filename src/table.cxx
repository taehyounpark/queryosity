#include "ana/table.h"


#include "ana/strutils.h"
#include "ana/vecutils.h"

ana::table::range::range() :
	slot(0),
	start(0),
	end(-1)
{}

ana::table::range::range(size_t slot, long long start, long long end) :
	slot(slot),
	start(start),
	end(end)
{}

long long ana::table::range::entries() const
{
	return end-start;
}

ana::table::range ana::table::range::operator+(const range& next)
{
	assert(this->end==next.start);
	return range(this->slot,this->start,next.end);
}

ana::table::range& ana::table::range::operator+=(const range& next)
{
	assert(this->end==next.start);
	this->end=next.end;
	return *this;
}

void ana::table::partition::add(size_t islot, long long start, long long end)
{
	this->parts.push_back(range(islot,start,end));
}

void ana::table::partition::add(const range& part)
{
	this->parts.push_back(part);
}

ana::table::range ana::table::partition::part(size_t islot) const
{
	return this->parts[islot];
}

ana::table::range ana::table::partition::total() const
{
	return vec::sum(this->parts);
}

size_t ana::table::partition::size() const
{
	return this->parts.size();
}

ana::table::partition ana::table::partition::merge(size_t max_parts) const
{
	if (fixed) return *this;
	partition merged;
	auto groups = vec::group(this->parts,max_parts);
	for (const auto& group : groups) {
		merged.parts.push_back(vec::sum(group));
	}
	return merged;
}

ana::table::partition ana::table::partition::truncate(long long max_entries) const
{
	if (fixed) return *this;
	if (max_entries<0) return *this;
	partition trunced;
	for (const auto& part : this->parts) {
		auto part_end = max_entries >= 0 ? std::min(part.start+max_entries,part.end) : part.end;
		trunced.parts.push_back(range(part.slot, part.start, part_end));
		max_entries -= part_end;
		if (!max_entries) break;
	}
	return trunced;
}

ana::table::progress::progress(long long tot) : 
	tot(tot)
{
	prog.store(0);
}

void ana::table::progress::start()
{
	prog.store(0);
}

ana::table::progress& ana::table::progress::operator++()
{
	prog++;
	return *this;
}

double ana::table::progress::percent() const
{ 
	return double(prog.load()) / double(tot) * 100.0; 
}

bool ana::table::progress::done() const 
{ 
	return prog.load() == tot; 
}