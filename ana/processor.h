#pragma once

#include <string>
#include <vector>
#include <memory>

#include "ana/routine.h"
#include "ana/computation.h"
#include "ana/cutflow.h"
#include "ana/experiment.h"

namespace ana
{

template <typename T>
class table::processor : public routine, public term::computation<T>, public counter::experiment
{

public:
	processor(table::reader<T>& reader, double scale);
	virtual ~processor() = default;

public:
	virtual void initialize() override;
	virtual void execute() override;
	virtual void finalize() override;

	void process();
	void process(table::progress& progress);

};

}

template <typename T>
ana::table::processor<T>::processor(table::reader<T>& reader, double scale) :
  routine(),
	term::computation<T>(reader),
	counter::experiment(scale)
{}

template <typename T>
void ana::table::processor<T>::initialize()
{
	for (const auto& col : this->m_columns) {
		col->initialize();
	}
	for (const auto& sel : this->m_selections) {
		sel->initialize();
	}
	for (const auto& cnt : this->m_counters) {
		cnt->initialize();
	}
}

template <typename T>
void ana::table::processor<T>::execute()
{
	for (const auto& col : this->m_columns) {
		col->execute();
	}
	for (const auto& sel : this->m_selections) {
		sel->execute();
	}
	for (const auto& cnt : this->m_counters) {
		cnt->execute();
	}
}

template <typename T>
void ana::table::processor<T>::finalize()
{
	for (const auto& col : this->m_columns) {
		col->finalize();
	}
	for (const auto& sel : this->m_selections) {
		sel->finalize();
	}
	for (const auto& cnt : this->m_counters) {
		cnt->finalize();
	}
}

template <typename T>
void ana::table::processor<T>::process()
{
	// start
	this->m_reader->begin();
	this->initialize();

	// per-entry
	while (this->m_reader->next()) {
		this->execute();
	}

	// finish
	this->finalize();
	this->m_reader->end();
}

template <typename T>
void ana::table::processor<T>::process(table::progress& progress)
{
	// start
	this->m_reader->begin();
	this->initialize();

	// per-entry
	while (this->m_reader->next()) {
		this->execute();
		++progress;
	}

	// finish
	this->finalize();
	this->m_reader->end();
}