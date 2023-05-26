#include "ana/input.h"

#include <numeric>
#include <iterator>

ana::input::range::range(size_t slot, long long begin, long long end) :
	slot(slot),
	begin(begin),
	end(end)
{}

long long ana::input::range::entries() const
{
	assert(this->end > this->begin);
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

std::vector<std::vector<ana::input::range>> ana::input::partition::group_parts(const std::vector<range>& parts, size_t n)
{
	std::vector<std::vector<range>> grouped_parts;
	size_t length = parts.size() / n;
	size_t remain = parts.size() % n;
	size_t begin = 0;
	size_t end = 0;
	for (size_t i = 0; i < std::min(n, parts.size()); ++i)
	{
		end += (remain > 0) ? (length + !!(remain--)) : length;
		grouped_parts.push_back(std::vector<range>(parts.begin()+begin, parts.begin()+end));
		begin = end;
	}
	return grouped_parts;	
}

ana::input::range ana::input::partition::sum_parts(const std::vector<range>& parts)
{
	return std::accumulate(std::next(parts.begin()), parts.end(), parts.front());
}

void ana::input::partition::add_part(size_t islot, long long begin, long long end)
{
	this->parts.push_back(range(islot,begin,end));
}

ana::input::range ana::input::partition::get_part(size_t islot) const
{
	return this->parts[islot];
}

ana::input::range ana::input::partition::total() const
{
	return sum_parts(this->parts);
}

size_t ana::input::partition::size() const
{
	return this->parts.size();
}

void ana::input::partition::merge(size_t max_parts)
{
	if (fixed) return;
	partition merged;
	auto groups = group_parts(this->parts,max_parts);
	for (const auto& group : groups) {
		merged.parts.push_back(sum_parts(group));
	}
	this->parts.clear();
	for (const auto& group : groups) {
		this->parts.push_back(sum_parts(group));
	}
}

void ana::input::partition::truncate(long long max_entries)
{
	if (fixed) return;
	if (max_entries<0) return;
	// remember the full parts
	auto full_parts = this->parts;
	// clear the parts to be added anew
	this->parts.clear();
	for (const auto& part : full_parts) {
		auto part_end = max_entries >= 0 ? std::min(part.begin+max_entries,part.end) : part.end;
		this->parts.push_back(range(part.slot, part.begin, part_end));
		max_entries -= part_end;
		if (!max_entries) break;
	}
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