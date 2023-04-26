#pragma once

#include <set>
#include <vector>
#include <memory>
#include <string>
#include <type_traits>
#include <functional>
#include <thread>

#include "ana/input.h"
#include "ana/sample.h"
#include "ana/column.h"
#include "ana/term.h"
#include "ana/selection.h"
#include "ana/cut.h"
#include "ana/weight.h"
#include "ana/counter.h"
#include "ana/concurrent.h"
#include "ana/processor.h"

namespace ana
{

template <typename T>
class analysis : public sample<T>
{

public:
  using dataset_reader_type = typename sample<T>::dataset_reader_type;

public:
	template <typename U>
	class node;

	template <typename U>
	class varied;

	template <typename U>
	static constexpr std::true_type check_varied(const typename analysis<T>::template varied<U>&);
	static constexpr std::false_type check_varied(...);
	template <typename V>
	static constexpr bool is_varied_v = decltype(check_varied(std::declval<V>()))::value;

public:
  analysis(long long max_entries=-1);
  virtual ~analysis() = default;

	analysis(analysis const&) = delete;
	analysis& operator=(analysis const&) = delete;

  template <typename Val>
  node<term<Val>> read(const std::string& name);

  template <typename Val>
  node<term<Val>> constant(const Val& value);

  template <typename Def, typename... Args>
  node<Def> define(const Args&... arguments);

  template <typename F, typename... Vars>
  auto evaluate(F callable, const node<Vars>&... columns) -> node<equation_t<F>>;

	// nominal nodes only
  // template <typename Sel, typename Syst, typename Var>
  // auto filter(const std::string& name, const Syst<Var>& column) -> Syst<selection>;

  template <typename Sel, typename F, typename... Systs , std::enable_if_t<(is_varied_v<Systs> || ...), int> = 0>
  auto filter(const std::string& name, F callable, Systs const&... columns) -> varied<selection>
	{
		auto syst = varied<selection>();
		// get applied variation names
		auto varnames = lists_applied_variation_names(columns...);
		// nominal
		auto nd = node<selection>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc, Systs&... vars) { return proc.template filter<Sel>(name,callable,vars...); }, columns.get_nominal().get_action()... ));
		this->add_selection(nd);
		syst->set_nominal(nd);
		// variations
		for (auto const& varname : varnames) {
			auto nd = node<selection>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc, Systs&... vars) { return proc.template filter<Sel>(name,callable,vars...); }, columns.get_variation(varname).get_action()... ));
			this->add_selection(nd);
			syst->set_variation(varname,nd);
		}
		return syst;	
	}

  template <typename Sel, typename F, typename... Systs , std::enable_if_t<(!is_varied_v<Systs> && ...), int> = 0>
  auto filter(const std::string& name, F callable, Systs const&... columns) -> node<selection>
	{
		auto nd = node<selection>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc, Systs&... vars) { return proc.template filter<Sel>(name,callable,vars...); }, columns.get_action()... ));
		this->add_selection(nd);
	  return nd;
	}

  template <typename Sel, typename Var>
  node<selection> channel(const std::string& name, const node<Var>& column);
  template <typename Sel, typename F, typename... Vars>
  node<selection> channel(const std::string& name, F callable, const node<Vars>&... columns);

  analysis& rebase(const node<selection>& sel);
	node<selection> operator[](const std::string& path) const;

	template <typename Cnt, typename... Args>
	node<counter::booker<Cnt>> count(Args&&... arguments);

	// template <typename Cnt>
	// node<Cnt> book(const node<counter::booker<Cnt>>& booker);

	std::vector<std::string> list_selection_paths() const;

	bool has_selection(const std::string& path) const;
	void clear_counters();

	void analyze();
	void reset();

protected:
  void process_dataset();

protected:
	void add_column(node<column> var);
	void add_selection(node<selection> sel);
	void add_counter(node<counter> cnt);

protected:
	bool m_analyzed;

	std::vector<node<column>> m_column_list;

	std::vector<std::string>                        m_selection_paths;
	std::unordered_map<std::string,node<selection>> m_selection_map;

	std::vector<node<counter>> m_counter_list;

};

}

#include "ana/node.h"
#include "ana/varied.h"

// ----------------------------------------------------------------------------
// analysis
// ----------------------------------------------------------------------------

template <typename T>
ana::analysis<T>::analysis(long long max_entries) :
	sample<T>(max_entries),
	m_analyzed(false)
{}

// template <typename T>
// template <typename... Args>
// ana::analysis<T>::analysis(Args&&... args) :
// 	sample<T>(std::forward<Args>(args)...),
// 	m_analyzed(false)
// {}

template <typename T>
template <typename Val>
typename ana::analysis<T>::template node<ana::term<Val>> ana::analysis<T>::read(const std::string& name)
{
	using column_reader_t = typename decltype(this->m_processors.model()->template read<Val>(name))::element_type;
	auto nd = node<column_reader_t>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc) { return proc.template read<Val>(name); } ));
	this->add_column(nd);
	return nd;
}

template <typename T>
template <typename Val>
typename ana::analysis<T>::template node<ana::term<Val>> ana::analysis<T>::constant(const Val& val)
{
	auto nd = node<term<Val>>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc) { return proc.template constant<Val>(val); } ));
	this->add_column(nd);
  return nd;
}

template <typename T>
template <typename Def, typename... Args>
typename ana::analysis<T>::template node<Def> ana::analysis<T>::define(const Args&... arguments)
{
	auto nd = node<Def>(*this, this->m_processors.from_slots( [&](processor<dataset_reader_type>& proc) { return proc.template define<Def>(arguments...); } ));
	this->add_column(nd);
	return nd;
}

template <typename T>
template <typename F, typename... Vars>
auto ana::analysis<T>::evaluate(F callable, const node<Vars>&... columns) ->  typename analysis<T>::template node<equation_t<F>>
{
	auto nd = node<equation_t<F>>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc, Vars&... vars) { return proc.template evaluate(callable,vars...); }, columns.get_action()... ));
	this->add_column(nd);
  return nd;
}

// template <typename T>
// template <typename Sel, typename F, typename... Systs, typename Enable>
// auto ana::analysis<T>::filter(const std::string& name, F callable, Systs const&... columns) -> varied<selection>
// {
// 	auto syst = varied<selection>();
// 	// get applied variation names
// 	auto varnames = lists_applied_variation_names(columns...);
// 	// nominal
// 	auto nd = node<selection>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc, Systs&... vars) { return proc.template filter<Sel>(name,callable,vars...); }, columns.get_nominal().get_action()... ));
// 	this->add_selection(nd);
// 	syst->set_nominal(nd);
// 	// variations
// 	for (auto const& varname : varnames) {
// 		auto nd = node<selection>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc, Systs&... vars) { return proc.template filter<Sel>(name,callable,vars...); }, columns.get_variation(varname).get_action()... ));
// 		this->add_selection(nd);
// 		syst->set_variation(varname,nd);
// 	}
//   return syst;	
// }

// template <typename T>
// template <typename Sel, typename F, typename Syst, typename Var>
// auto node<ana::selection> ana::analysis<T>::filter(const std::string& name, const node<Var>& column) -> Syst<selection>
// {
// 	auto nd = node<selection>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc, Var& var) { return proc.template filter<Sel>(name,[](typename std::decay_t<decltype(std::declval<Var>().value())> val){return val;},var); }, column.get_action() ));
// 	this->add_selection(nd);
//   return nd;
// }

// template <typename T>
// template <typename Sel, typename F, typename... Systs>
// typename ana::analysis<T>::template varied<ana::selection> ana::analysis<T>::filter(const std::string& name, F callable, Systs const&... columns)
// {
// 	auto syst = varied<selection>();
// 	// get applied variation names
// 	auto varnames = lists_applied_variation_names(columns...);
// 	// nominal
// 	auto nd = node<selection>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc, Systs&... vars) { return proc.template filter<Sel>(name,callable,vars...); }, columns.get_nominal().get_action()... ));
// 	this->add_selection(nd);
// 	syst->set_nominal(nd);
// 	// variations
// 	for (auto const& varname : varnames) {
// 		auto nd = node<selection>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc, Systs&... vars) { return proc.template filter<Sel>(name,callable,vars...); }, columns.get_variation(varname).get_action()... ));
// 		this->add_selection(nd);
// 		syst->set_variation(varname,nd);
// 	}
//   return syst;
// }

// template <typename T>
// template <typename Sel, typename Var>
// typename ana::analysis<T>::template varied<ana::selection> ana::analysis<T>::filter(const std::string& name, varied<Var> const& column)
// {

// 	auto syst = varied<selection>();
// 	// get applied variation names
// 	auto varnames = lists_applied_variation_names(column);
// 	// nominal
// 	auto nd = node<selection>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc, Var& var) { return proc.template filter<Sel>(name,[](typename std::decay_t<decltype(std::declval<Var>().value())> val){return val;},var); }, column.get_nominal().get_action() ));
// 	this->add_selection(nd);
// 	syst->set_nominal(nd);
// 	// variations
// 	for (auto const& varname : varnames) {
// 		auto nd = node<selection>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc, Var& var) { return proc.template filter<Sel>(name,[](typename std::decay_t<decltype(std::declval<Var>().value())> val){return val;},var); }, column.get_variation(varname).get_action() ));
// 		this->add_selection(nd);
// 		syst->set_variation(varname,nd);
// 	}
//   return syst;
// }

// template <typename T>
// template <typename Sel, typename F, typename... Vars>
// typename ana::analysis<T>::template node<ana::selection> ana::analysis<T>::channel(const std::string& name, F callable, const node<Vars>&... columns)
// {
// 	auto nd = node<selection>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc, Vars&... vars) { return proc.template channel<Sel>(name,callable,vars...); }, columns.get_action()... ));
// 	this->add_selection(nd);
//   return nd;
// }

// template <typename T>
// template <typename Sel, typename Var>
// typename ana::analysis<T>::template node<ana::selection> ana::analysis<T>::channel(const std::string& name, const node<Var>& column)
// {
// 	auto nd = node<selection>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc, Var& var) { return proc.template channel<Sel>(name,[](typename std::decay_t<decltype(std::declval<Var>().value())> val){return val;},var); }, column.get_action() ));
// 	this->add_selection(nd);
//   return nd;
// }

template <typename T>
template <typename Cnt, typename... Args>
typename ana::analysis<T>::template node<ana::counter::booker<Cnt>> ana::analysis<T>::count(Args&&... args)
{
	auto nd = node<counter::booker<Cnt>>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc) { return proc.template count<Cnt>(args...); } ));
  return nd;
}

// template <typename T>
// template <typename Cnt>
// typename ana::analysis<T>::template node<Cnt> ana::analysis<T>::book(const node<counter::booker<Cnt>>& booker)
// {
// 	// any time a new counter is booked, means the analysis must run: so reset its status
// 	this->reset();
// 	auto nd = node<Cnt>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc, counter::booker<Cnt>& bkr) { return proc.template book<Cnt>(bkr); }, booker.get_action() ));
// 	this->add_counter(nd);
//   return nd;
// }

template <typename T>
typename ana::analysis<T>::template node<ana::selection> ana::analysis<T>::operator[](const std::string& path) const
{
	if (!this->has_selection(path)) {
		throw std::logic_error("selection does not exist");
	};
	return m_selection_map.at(path);
}

template <typename T>
void ana::analysis<T>::analyze()
{ 
	if (!m_analyzed) {
		this->process_dataset();
		this->clear_counters();
	}
	m_analyzed = true;
}

template <typename T>
void ana::analysis<T>::reset()
{ 
	m_analyzed = false;
}

template <typename T>
void ana::analysis<T>::clear_counters()
{ 
	m_counter_list.clear();
	this->m_processors.to_slots( [] (processor<dataset_reader_type>& proc) { proc.clear_counters(); } );
}

template <typename T>
void ana::analysis<T>::process_dataset()
{
	// start
	this->m_dataset->start();

	// multi-threaded
	if (multithread::status()) {
		// start threads
		std::vector<std::thread> pool;
		for (size_t islot=0 ; islot<this->m_processors.concurrency() ; ++islot) {
			pool.emplace_back(
				[] (processor<dataset_reader_type>& proc) {
					proc.process();
				},
				std::ref(*this->m_processors.get_slot(islot))
			);
		}
		// join threads
		for (auto&& thread : pool) {
			thread.join();
		}
	// single-threaded
	} else {
		for (size_t islot=0 ; islot<this->m_processors.concurrency() ; ++islot) {
			this->m_processors.get_slot(islot)->process();
		}
	}

	// finish
	this->m_dataset->finish();

}

template <typename T>
ana::analysis<T>& ana::analysis<T>::rebase(const node<selection>& sel)
{
	this->m_processors.to_slots( [] (processor<dataset_reader_type>& proc, selection& sel) { proc.rebase(sel); }, sel.get_action() );	
  return *this;
}

template <typename T>
bool ana::analysis<T>::has_selection(const std::string& path) const
{
	return m_selection_map.find(path)!=m_selection_map.end();
}

template <typename T>
void ana::analysis<T>::add_column(typename ana::analysis<T>::template node<column> node)
{
	m_column_list.push_back(node);
}

template <typename T>
void ana::analysis<T>::add_selection(typename ana::analysis<T>::template node<selection> node)
{
	auto path = node.get_path();
	if (this->has_selection(path)) {
		throw std::logic_error("selection already exists");
	}
	m_selection_map[path] = node;
	m_selection_paths.push_back(path);
}

template <typename T>
void ana::analysis<T>::add_counter(typename ana::analysis<T>::template node<counter> node)
{
	m_counter_list.push_back(node);
}

template <typename T>
std::vector<std::string> ana::analysis<T>::list_selection_paths() const
{
	return m_selection_paths;
}