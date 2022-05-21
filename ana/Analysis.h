#pragma once

#include <vector>
#include <memory>
#include <string>
#include <type_traits>
#include <functional>
#include <thread>

#include "ana/table.h"
#include "ana/sample.h"
#include "ana/column.h"
#include "ana/cell.h"
#include "ana/selection.h"
#include "ana/cut.h"
#include "ana/weight.h"
#include "ana/counter.h"
#include "ana/concurrent.h"
#include "ana/processor.h"

namespace ana
{

template <typename T>
class Analysis : public sample<T>
{

public:
	template <typename U>
	class node;

public:
  Analysis(long long maxEntries=-1);
  ~Analysis() = default;

  template <typename Val, typename... Args>
  node<column<Val>> read(const std::string& name, const Args&... args);

  template <typename Val>
  node<column<Val>> constant(const std::string& name, const Val& value);

  template <typename Def, typename... Args>
  node<Def> define(const std::string& name, const Args&... arguments);

  template <typename F, typename... Vars>
  auto evaluate(const std::string& name, F callable, const node<Vars>&... columns) -> node<column<std::decay_t<typename decltype(std::function(std::declval<F>()))::result_type>>>;

  Analysis& at(const node<selection>& rebase);

  template <typename Sel, typename F, typename... Vars>
  node<selection> filter(const std::string& name, F callable, const node<Vars>&... columns);

  template <typename Sel, typename F, typename... Vars>
  node<selection> channel(const std::string& name, F callable, const node<Vars>&... columns);

	template <typename Cntr, typename... Args>
	node<counter::booker<Cntr>> count(const std::string& name, const Args&... arguments);

	template <typename Cntr>
	node<Cntr> book(const node<counter::booker<Cntr>>& booker);

	bool has_variable(const std::string& name);
	node<variable> get_variable(const std::string& name);

	bool has_selection(const std::string& path);
	node<selection> get_selection(const std::string& path);

	bool hascounter(const std::string& name);
	node<counter> getcounter(const std::string& name);

	std::vector<std::string> list_column_names() const;
	std::vector<std::string> list_selection_paths() const;
	std::vector<std::string> listcounterPaths() const;

public:
	void analyze();
	void reset();

protected:
  void run();

protected:
	void add_variable(node<variable> var);
	void add_selection(node<selection> sel);
	void add_counter(node<counter> cnt);

protected:
	bool m_analyzed;

	std::vector<std::string>                       m_columnNames;
	std::unordered_map<std::string,node<variable>> m_columnMap;

	std::vector<std::string>                        m_selectionPaths;
	std::unordered_map<std::string,node<selection>> m_selectionMap;

	std::vector<std::string>                       m_counterPaths;
	std::unordered_map<std::string,node<counter>>  m_counterMap;

};

template <typename T>
template <typename U>
class Analysis<T>::node : public concurrent<U>
{

public:
	using node_type = typename concurrent<U>::node_type;

public:
	friend class Analysis<T>;
	template <typename> friend class node;

public:
	node() :
		m_analysis(nullptr)
	{}
	node(Analysis<T>& analysis, const concurrent<U>& action) :
		concurrent<U>(action),
		m_analysis(&analysis)
	{}
	~node() = default;

	template <typename V>
	node(const node<V>& other) :
		concurrent<U>(other),
		m_analysis(other.m_analysis)
	{}

	template <typename V>
	node& operator=(const node<V>& other)
	{
		m_analysis = other.m_analysis;
		this->m_islots.clear();
		for (size_t i=0 ; i<other.concurrency() ; ++i) {
			this->m_islots.push_back(other.slot(i));
		}
		return *this;
	}

	std::string name() const
	{
		return this->check( [] (const action& act) { return act.name(); } );
	}

	std::string path() const
	{
		if constexpr(std::is_base_of_v<selection,U>) {
			return this->check( [] (const selection& fltr) { return fltr.path(); } );
		} else if constexpr(std::is_base_of_v<counter,U>) {
			return this->check( [] (const counter& cnt) { return cnt.path(); } );
		} else {
			static_assert((std::is_base_of_v<selection,U> || std::is_base_of_v<counter,U>), "non-selection node has no path defined");
		}
	}

  template <typename V = U, typename... Vars>
	typename std::enable_if<std::tuple_size<decltype(std::declval<V>().get_arguments())>::value!=0, void>::type evaluate(const node<Vars>&... arguments)
	{
		this->in_seq( [] (U& defn, Vars&... args) { defn.input_arguments(args...); }, arguments... );
	}

  template <typename Sel, typename F, typename... Vars>
  node<selection> filter(const std::string& name, F callable, const node<Vars>&... columns)
	{
		if constexpr(std::is_base_of_v<selection,U>) {
			return m_analysis->at(*this).template filter<Sel>(name, callable, columns...);
		} else {
			static_assert(std::is_base_of_v<selection,U>, "non-selection cannot chain filters");
		}
	}

  template <typename Sel, typename F, typename... Vars>
  node<selection> channel(const std::string& name, F callable, const node<Vars>&... columns)
	{
		if constexpr(std::is_base_of_v<selection,U>) {
			return m_analysis->at(*this).template channel<Sel>(name, callable, columns...);
		} else {
			static_assert(std::is_base_of_v<selection,U>, "non-selection cannot chain filters");
		}
	}

	void weighted(bool weighted=true)
	{
		if constexpr(std::is_base_of_v<counter,U>) {
			this->in_seq( [=] (counter& cnt) { cnt.use_weight(weighted); } );
		} else {
			static_assert(std::is_base_of_v<counter,U>, "non-counter cannot be set to be raw/weighted");
		}
	}

	void scale(double scale)
	{
		if constexpr(std::is_base_of_v<counter,U>) {
			this->in_seq( [=] (counter& cnt) { cnt.applyScale(scale); } );
		} else {
			static_assert(std::is_base_of_v<counter,U>, "non-counter cannot be scaled");
		}
	}

	template <typename... Cols>
	void fill(const node<Cols>&... columns)
	{
		if constexpr( is_counter_booker<U>::value ) {
			this->in_seq( [] (U& booker, Cols&... cols) { booker.fill_columns(cols...); }, columns... );
		} else if constexpr( std::is_base_of_v<counter::implementation<U>, U> ) {
			this->in_seq( [] (U& cnt, Cols&... cols) { cnt.fill_columns(cols...); }, columns... );
		} else {
			static_assert( (is_counter_booker<U>::value ||  std::is_base_of_v<counter::implementation<U>, U>), "non-counter(booker) cannot be filled with columns" );
		}
	}

	template <typename Cntr, typename V = U, std::enable_if_t<std::is_base_of_v<selection,V>>* = nullptr>
	node<Cntr> book(const node<counter::booker<Cntr>>& booker)
	{
		return m_analysis->at(*this).template book(booker);
	}

	template <typename V = U, typename std::enable_if<is_counter_booker<V>::value,void>::type* = nullptr>
	node<typename V::counterType> book(const node<selection>& filter)
	{
		return m_analysis->at(filter).template book(*this);
	}

	template <typename V = U, std::enable_if_t<std::is_base_of_v<counter::implementation<V>,V>>* = nullptr>
	decltype(auto) result()
	{
		m_analysis->analyze();
		return this->model()->getResult();
	}

	template <typename Out, typename V = U, std::enable_if_t<std::is_base_of_v<counter::implementation<V>,V>>* = nullptr>
	void output(Out& out)
	{
		m_analysis->analyze();
		this->model()->outputResult(out);
	}

protected:
	void merge()
	{
		if constexpr(std::is_base_of_v<counter,U>) {
			for (size_t iislot=1 ; iislot<this->concurrency() ; ++iislot) {
				this->model()->merge_counter(*this->slot(iislot));
			}
		} else {
			static_assert(std::is_base_of_v<counter,U>,"non-counter cannot be merged");
		}
	}

protected:
	Analysis<T>*  m_analysis;

};

}

// ----------------------------------------------------------------------------
// Analysis
// ----------------------------------------------------------------------------

template <typename T>
ana::Analysis<T>::Analysis(long long maxEntries) :
	sample<T>(maxEntries),
	m_analyzed(false)
{}

template <typename T>
template <typename Val, typename... Args>
typename ana::Analysis<T>::template node<ana::column<Val>> ana::Analysis<T>::read(const std::string& name, const Args&... args)
{
	auto nd = node<column<Val>>(*this, this->m_dataprocessors.invoke( [=](table::processor<T>& processor) { return processor.template read<Val>(name,args...); } ));
	this->add_variable(nd);
	return nd;
}

template <typename T>
template <typename Val>
typename ana::Analysis<T>::template node<ana::column<Val>> ana::Analysis<T>::constant(const std::string& name, const Val& val)
{
	auto nd = node<column<Val>>(*this, this->m_dataprocessors.invoke( [=](table::processor<T>& processor) { return processor.template constant<Val>(name,val); } ));
	this->add_variable(nd);
  return nd;
}

template <typename T>
template <typename Def, typename... Args>
typename ana::Analysis<T>::template node<Def> ana::Analysis<T>::define(const std::string& name, const Args&... arguments)
{
	auto nd = node<Def>(*this, this->m_dataprocessors.invoke( [&](table::processor<T>& processor) { return processor.template define<Def>(name,arguments...); } ));
	this->add_variable(nd);
	return nd;
}

template <typename T>
template <typename F, typename... Vars>
auto ana::Analysis<T>::evaluate(const std::string& name, F callable, const node<Vars>&... columns) ->  typename Analysis<T>::template node<column<std::decay_t<typename decltype(std::function(std::declval<F>()))::result_type>>>
{
	auto nd = node<column<std::decay_t<typename decltype(std::function(std::declval<F>()))::result_type>>>(*this, this->m_dataprocessors.invoke( [=](table::processor<T>& processor, Vars&... vars) { return processor.template evaluate(name,callable,vars...); }, columns... ));
	this->add_variable(nd);
  return nd;
}

template <typename T>
template <typename Sel, typename F, typename... Vars>
typename ana::Analysis<T>::template node<ana::selection> ana::Analysis<T>::filter(const std::string& name, F callable, const node<Vars>&... columns)
{
	auto nd = node<selection>(*this, this->m_dataprocessors.invoke( [=](table::processor<T>& processor, Vars&... vars) { return processor.template filter<Sel>(name,callable,vars...); }, columns... ));
	this->add_selection(nd);
  return nd;
}

template <typename T>
template <typename Sel, typename F, typename... Vars>
typename ana::Analysis<T>::template node<ana::selection> ana::Analysis<T>::channel(const std::string& name, F callable, const node<Vars>&... columns)
{
	auto nd = node<selection>(*this, this->m_dataprocessors.invoke( [=](table::processor<T>& processor, Vars&... vars) { return processor.template channel<Sel>(name,callable,vars...); }, columns... ));
	this->add_selection(nd);
  return nd;
}

template <typename T>
template <typename Cntr, typename... Args>
typename ana::Analysis<T>::template node<ana::counter::booker<Cntr>> ana::Analysis<T>::count(const std::string& name, const Args&... arguments)
{
	auto nd = node<counter::booker<Cntr>>(*this, this->m_dataprocessors.invoke( [=](table::processor<T>& processor) { return processor.template count<Cntr>(name,arguments...); } ));
  return nd;
}

template <typename T>
template <typename Cntr>
typename ana::Analysis<T>::template node<Cntr> ana::Analysis<T>::book(const node<counter::booker<Cntr>>& booker)
{
	auto nd = node<Cntr>(*this, this->m_dataprocessors.invoke( [=](table::processor<T>& processor, const counter::booker<Cntr>& dlyd) { return processor.template book<Cntr>(dlyd); }, booker ));
	this->add_counter(nd);
  return nd;
}

template <typename T>
typename ana::Analysis<T>::template node<ana::variable> ana::Analysis<T>::get_variable(const std::string& name)
{
	if (!this->has_variable(name)) {
		throw std::logic_error("column does not exist");
	};
	return m_columnMap[name];
}

template <typename T>
typename ana::Analysis<T>::template node<ana::selection> ana::Analysis<T>::get_selection(const std::string& path)
{
	if (!this->has_selection(path)) {
		throw std::logic_error("selection does not exist");
	};
	return m_selectionMap[path];
}

template <typename T>
typename ana::Analysis<T>::template node<ana::counter> ana::Analysis<T>::getcounter(const std::string& name)
{
	if (!this->hascounter(name)) {
		throw std::logic_error("counter does not exist");
	}
	return m_counterMap[name];
}

template <typename T>
void ana::Analysis<T>::analyze()
{ 
	if (!m_analyzed) this->run();
	m_analyzed = true;
}

template <typename T>
void ana::Analysis<T>::reset()
{ 
	m_analyzed = false;
}

template <typename T>
void ana::Analysis<T>::run()
{
	// start
	this->m_dataset->start_run();

	// multithread (if set)
	if (multithread::status()) {
		// start parts
		for (size_t islot=0 ; islot<this->m_dataprocessors.concurrency() ; ++islot) {
			this->m_dataRanges.slot(islot)->start_range(this->m_partition.part(islot));
		}
		// start threads
		std::vector<std::thread> pool;
		for (size_t islot=0 ; islot<this->m_dataprocessors.concurrency() ; ++islot) {
			pool.emplace_back(
				[] (table::processor<T>& processor) {
					processor.process();
				},
				std::ref(*this->m_dataprocessors.slot(islot))
			);
		}
		// join threads
		for (auto&& thread : pool) {
			thread.join();
		}
		// end parts
		for (size_t islot=0 ; islot<this->m_dataprocessors.concurrency() ; ++islot) {
			this->m_dataRanges.slot(islot)->end_range();
		}
	} else {
		for (size_t islot=0 ; islot<this->m_dataprocessors.concurrency() ; ++islot) {
			this->m_dataRanges.slot(islot)->start_range(this->m_partition.part(islot));
			this->m_dataprocessors.slot(islot)->process();
			this->m_dataRanges.slot(islot)->end_range();
		}
	}

	// end
	this->m_dataset->end_run();

	// merge counters
	for (auto& cnt : m_counterMap) {
		cnt.second.merge();
	}
	// clear counters (for future runs)
	m_counterMap.clear();

}

template <typename T>
ana::Analysis<T>& ana::Analysis<T>::at(const node<selection>& rebase)
{
	this->m_dataprocessors.in_seq( [] (table::processor<T>& processor, selection& fltr) { processor.at(fltr); }, rebase );	
  return *this;
}

template <typename T>
bool ana::Analysis<T>::has_variable(const std::string& name)
{
	return m_columnMap.find(name)!=m_columnMap.end();
}

template <typename T>
bool ana::Analysis<T>::has_selection(const std::string& path)
{
	return m_selectionMap.find(path)!=m_selectionMap.end();
}

template <typename T>
bool ana::Analysis<T>::hascounter(const std::string& name)
{
	return m_counterMap.find(name)!=m_counterMap.end();
}

template <typename T>
void ana::Analysis<T>::add_variable(typename ana::Analysis<T>::template node<variable> node)
{
	auto name = node.name();
	if (this->has_variable(name)) {
		throw std::logic_error("column already exists");
	}
	m_columnMap[name] = node;
	m_columnNames.push_back(name);
}

template <typename T>
void ana::Analysis<T>::add_selection(typename ana::Analysis<T>::template node<selection> node)
{
	auto path = node.path();
	if (this->has_selection(path)) {
		throw std::logic_error("selection already exists");
	}
	m_selectionMap[path] = node;
	m_selectionPaths.push_back(path);
}

template <typename T>
void ana::Analysis<T>::add_counter(typename ana::Analysis<T>::template node<counter> node)
{
	auto path = node.path();
	if (this->hascounter(path)) {
		throw std::logic_error("counter already exists");
	}
	m_counterMap[path] = node;
	m_counterPaths.push_back(path);
}

template <typename T>
std::vector<std::string> ana::Analysis<T>::list_column_names() const
{
	return m_columnNames;
}

template <typename T>
std::vector<std::string> ana::Analysis<T>::list_selection_paths() const
{
	return m_selectionPaths;
}

template <typename T>
std::vector<std::string> ana::Analysis<T>::listcounterPaths() const
{
	return m_counterPaths;
}