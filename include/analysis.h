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
#include "ana/selection.h"
#include "ana/cut.h"
#include "ana/weight.h"
#include "ana/counter.h"
#include "ana/concurrent.h"
#include "ana/looper.h"

namespace ana
{

template <typename T> struct is_column_evaluator : std::false_type {};
template <typename T> struct is_column_evaluator<column::evaluator<T>> : std::true_type {};
template <typename T> constexpr bool is_column_evaluator_v = is_column_evaluator<T>::value;

constexpr std::true_type check_column(const column&);
constexpr std::false_type check_column(...);
template <typename T> constexpr bool is_column_v = decltype(check_column(std::declval<T>()))::value;

template <typename T>
constexpr std::true_type check_column_reader(typename column::reader<T> const&);
constexpr std::false_type check_column_reader(...);
template <typename T> constexpr bool is_column_reader_v = decltype(check_column_reader(std::declval<T>()))::value;

template <typename T>
constexpr std::true_type check_column_constant(typename column::constant<T> const&);
constexpr std::false_type check_column_constant(...);
template <typename T> constexpr bool is_column_constant_v = decltype(check_column_constant(std::declval<T>()))::value;

template <typename T>
constexpr std::true_type check_column_equation(typename column::equation<T> const&);
constexpr std::false_type check_column_equation(...);
template <typename T> constexpr bool is_column_equation_v = decltype(check_column_equation(std::declval<T>()))::value;

template <typename T>
constexpr std::true_type check_column_definition(typename column::definition<T> const&);
constexpr std::false_type check_column_definition(...);
template <typename T> constexpr bool is_column_definition_v = decltype(check_column_definition(std::declval<T>()))::value;

template <typename T> struct is_selection_evaluator: std::false_type {};
template <typename T> struct is_selection_evaluator<selection::evaluator<T>>: std::true_type {};
// template <typename T> struct is_selection_evaluator<selection::weight::evaluator<T>>: std::true_type {};
template <typename T> constexpr bool is_selection_evaluator_v = is_selection_evaluator<T>::value;

template <typename Lmbd> using function_t = decltype(std::function(std::declval<Lmbd>()));
template <typename Lmbd> using equation_evaluator_t = typename column::template evaluator<ana::equation_t<Lmbd>>;
template <typename Lmbd> using custom_selection_evaluator_t = typename selection::template evaluator<ana::equation_t<Lmbd>>;
using simple_selection_evaluator_type = typename selection::template evaluator<ana::column::equation<double(double)>>;

template <typename T>
class analysis : public sample<T>
{

public:
  using dataset_reader_type = typename sample<T>::dataset_reader_type;

public:
	template <typename U>
	class node;

	template <typename U>
	class lazy;
	template <typename U>
	friend class lazy;

	template <typename U>
	class varied;
	template <typename U>
	friend class varied;

	template <typename U>
	static constexpr std::true_type check_nominal(typename analysis<T>::template lazy<U> const&);
	static constexpr std::false_type check_nominal(...);
	template <typename V> static constexpr bool is_nominal_v = decltype(check_nominal(std::declval<V>()))::value;

	template <typename U>
	static constexpr std::true_type check_varied(typename analysis<T>::template varied<U> const&);
	static constexpr std::false_type check_varied(...);
	template <typename V> static constexpr bool is_varied_v = decltype(check_varied(std::declval<V>()))::value;

	template <typename... Args> static constexpr bool has_no_variation_v = (is_nominal_v<Args>&&...);
	template <typename... Args> static constexpr bool has_variation_v = (is_varied_v<Args>||...);

public:
  analysis();
  virtual ~analysis() = default;

	// catch-all
  template <typename... Args>
  analysis(Args&&... args);
  // shortcuts for file paths provided with initializer braces
  template <typename U = T, typename = std::enable_if_t<std::is_constructible_v<U,std::string,std::vector<std::string>>>>
  analysis(const std::string& key, const std::vector<std::string>& file_paths);
  // shortcuts for file paths provided with initializer braces
  template <typename U = T, typename = std::enable_if_t<std::is_constructible_v<U,std::vector<std::string>,std::string>>>
  analysis(const std::vector<std::string>& file_paths, const std::string& key);

	analysis(analysis const&) = delete;
	analysis& operator=(analysis const&) = delete;

	analysis(analysis&&) = default;
	analysis& operator=(analysis&&) = default;

  template <typename Val>
  auto read(const std::string& name) -> lazy<read_column_t<read_dataset_t<T>,Val>>;
  template <typename Val>
  auto constant(const Val& value) -> lazy<column::constant<Val>>;

  template <typename Def, typename... Args, typename = std::enable_if_t<std::is_constructible_v<Def,Args&&...>>>
  auto define(Args&&... arguments) -> lazy<column::evaluator<Def>>;
  template <typename Lmbd, typename = std::enable_if_t<!is_column_definition_v<Lmbd>>>
  auto calculate(Lmbd lmbd) -> lazy<equation_evaluator_t<Lmbd>>;

	template <typename Sel, typename Lmbd>
  auto filter(const std::string& name, Lmbd lmbd) -> lazy<custom_selection_evaluator_t<Lmbd>>;
  template <typename Sel, typename Lmbd>
  auto channel(const std::string& name, Lmbd lmbd) -> lazy<custom_selection_evaluator_t<Lmbd>>;
	template <typename Sel>
  auto filter(const std::string& name) -> lazy<simple_selection_evaluator_type>;
  template <typename Sel>
  auto channel(const std::string& name) -> lazy<simple_selection_evaluator_type>;

	template <typename Cnt, typename... Args>
	auto book(Args&&... args) -> lazy<counter::booker<Cnt>>;

  template <typename Def, typename... Cols>
	auto evaluate_column(lazy<column::evaluator<Def>> const& calc, lazy<Cols> const&... columns) -> lazy<Def>;

	template <typename Sel, typename... Cols>
	auto evaluate_selection(lazy<selection::evaluator<Sel>> const& calc, lazy<Cols> const&... columns) -> lazy<selection>;

	template <typename Cnt>
	auto count_selection(lazy<counter::booker<Cnt>> const& bkr, lazy<selection> const& sel) -> lazy<Cnt>;

	template <typename Cnt, typename... Sels>
	auto count_selections(lazy<counter::booker<Cnt>> const& bkr, lazy<Sels> const&... sels) -> lazy<counter::booker<Cnt>>;

	void clear_counters();

	void analyze();
	void reset();

protected:
  void process_dataset();

	template <typename Sel, typename Lmbd>
  auto filter(lazy<selection> const& prev, const std::string& name, Lmbd lmbd) -> lazy<custom_selection_evaluator_t<Lmbd>>;
  template <typename Sel, typename Lmbd>
  auto channel(lazy<selection> const& prev, const std::string& name, Lmbd lmbd) -> lazy<custom_selection_evaluator_t<Lmbd>>;
	template <typename Sel>
  auto filter(lazy<selection> const& prev, const std::string& name) -> lazy<simple_selection_evaluator_type>;
  template <typename Sel>
  auto channel(lazy<selection> const& prev, const std::string& name) -> lazy<simple_selection_evaluator_type>;

	// recreate a lazy node as a variation under new arguments
	template <typename V, typename std::enable_if_t<ana::is_column_reader_v<V>, V>* = nullptr>
	auto vary_column(lazy<V> const& nom, const std::string& colname) -> lazy<V>;
	template <typename Val, typename V, typename std::enable_if_t<ana::is_column_constant_v<V>, V>* = nullptr>
	auto vary_column(lazy<V> const& nom, Val const& val) -> lazy<V>;
	template <typename... Args, typename V, typename std::enable_if_t<ana::is_column_definition_v<V> && !ana::is_column_equation_v<V>, V>* = nullptr>
	auto vary_definition(lazy<column::evaluator<V>> const& nom, Args&&... args) -> lazy<column::evaluator<V>>;
	template <typename Lmbd, typename V, typename std::enable_if_t<ana::is_column_equation_v<V>, V>* = nullptr>
	auto vary_equation(lazy<column::evaluator<V>> const& nom, Lmbd lmbd) -> lazy<column::evaluator<V>>;

  // template <typename Cnt>
  // auto repeat_booker(lazy<counter::booker<Cnt>> const& bkr) -> lazy<counter::booker<Cnt>>;

protected:
	void add_column(lazy<column> var);
	void add_selection(lazy<selection> sel);
	void add_counter(lazy<counter> cnt);

protected:
	bool m_analyzed;

	std::vector<lazy<column>>    m_column_list;
	std::vector<lazy<selection>> m_selection_list;
	std::vector<lazy<counter>>   m_counter_list;

};

template <typename T>
template <typename U>
class analysis<T>::node
{

public:
	using analysis_type = analysis<T>;
	using dataset_type = T;
	using action_type = U;

public:
	friend class analysis<T>;
	template <typename> friend class node;

public:
	node() :
		m_analysis(nullptr)
	{}
	node(analysis<T>& analysis) :
		m_analysis(&analysis)
	{}
	virtual ~node() = default;

public:

	virtual lazy<U> nominal() const = 0;
	virtual lazy<U> variation(const std::string& var_name) const = 0;

	virtual void set_nominal(lazy<U> const& nom) = 0;
	virtual void set_variation(const std::string& var_name, lazy<U> const& nom) = 0;

	virtual bool has_variation(const std::string& var_name) const = 0;
	virtual std::set<std::string> list_variation_names() const = 0;

protected:
	analysis<T>* m_analysis;

};

template <typename... Nodes>
auto list_all_variation_names(Nodes const&... nodes) -> std::set<std::string>;

}

#include "ana/lazy.h"
#include "ana/varied.h"

// ----------------------------------------------------------------------------
// analysis
// ----------------------------------------------------------------------------

template <typename T>
ana::analysis<T>::analysis() :
  sample<T>::sample(),
	m_analyzed(false)
{}

template <typename T>
template <typename... Args>
ana::analysis<T>::analysis(Args&&... args) :
  analysis<T>::analysis()
{
	this->open(std::forward<Args>(args)...);
}

template <typename T>
template <typename U, typename>
ana::analysis<T>::analysis(const std::string& key, const std::vector<std::string>& file_paths) :
  analysis<T>::analysis()
{
	this->open(key, file_paths);
}

template <typename T>
template <typename U, typename>
ana::analysis<T>::analysis(const std::vector<std::string>& file_paths, const std::string& key) :
  analysis<T>::analysis()
{
	this->open(file_paths, key);
}


template <typename T>
template <typename Val>
// typename ana::analysis<T>::template lazy<ana::term<Val>> ana::analysis<T>::read(const std::string& name)
auto ana::analysis<T>::read(const std::string& name) -> lazy<read_column_t<read_dataset_t<T>,Val>>
{
	this->prepare();
	auto nd = lazy<read_column_t<read_dataset_t<T>,Val>>(*this, this->m_loopers.from_slots( [=](looper<dataset_reader_type>& lpr) { return lpr.template read<Val>(name); } ));
	this->add_column(nd);
	return nd;
}

template <typename T>
template <typename Val>
auto ana::analysis<T>::constant(const Val& val) -> lazy<ana::column::constant<Val>>
{
	auto nd = lazy<column::constant<Val>>(*this, this->m_loopers.from_slots( [=](looper<dataset_reader_type>& lpr) { return lpr.template constant<Val>(val); } ));
	this->add_column(nd);
  return nd;
}

template <typename T>
template <typename Def, typename... Args, typename>
auto ana::analysis<T>::define(Args&&... arguments) -> typename analysis<T>::template lazy<column::evaluator<Def>>
{
	auto nd = lazy<column::evaluator<Def>>(*this, this->m_loopers.from_slots( [&](looper<dataset_reader_type>& lpr) { return lpr.template define<Def>(arguments...); } ));
	return nd;
}

template <typename T>
template <typename Lmbd, typename>
auto ana::analysis<T>::calculate(Lmbd lmbd) ->  typename analysis<T>::template lazy<equation_evaluator_t<Lmbd>>
{
	auto fn = std::function{lmbd};
	return lazy<equation_evaluator_t<Lmbd>>(*this, this->m_loopers.from_slots( [=](looper<dataset_reader_type>& lpr) { return lpr.template calculate(fn); } ));
}

template <typename T>
template <typename Def, typename... Cols>
auto ana::analysis<T>::evaluate_column(lazy<column::evaluator<Def>> const& calc, lazy<Cols> const&... columns) -> lazy<Def>
{
	auto col = lazy<Def>(*this, this->m_loopers.from_slots( [=](looper<dataset_reader_type>& lpr, column::evaluator<Def>& calc, Cols const&... cols) { return lpr.template evaluate_column(calc, cols...); }, calc.get_slots(), columns.get_slots()... ));
	this->add_column(col);
  return col;
}

template <typename T>
template <typename Sel, typename Lmbd>
auto ana::analysis<T>::filter(const std::string& name, Lmbd lmbd) -> lazy<custom_selection_evaluator_t<Lmbd>>
{
	auto fn = std::function{lmbd};
	return lazy<custom_selection_evaluator_t<Lmbd>>(*this, this->m_loopers.from_slots( [=](looper<dataset_reader_type>& lpr) { return lpr.template filter<Sel>(name,fn); } ));
}

template <typename T>
template <typename Sel, typename Lmbd>
auto ana::analysis<T>::channel(const std::string& name, Lmbd lmbd) -> lazy<custom_selection_evaluator_t<Lmbd>>
{
	auto fn = std::function{lmbd};
	auto sel = lazy<custom_selection_evaluator_t<Lmbd>>(*this, this->m_loopers.from_slots( [=](looper<dataset_reader_type>& lpr) { return lpr.template channel<Sel>(name,fn); } ));
	return sel;	
}

template <typename T>
template <typename Sel>
auto ana::analysis<T>::filter(const std::string& name) -> lazy<simple_selection_evaluator_type>
{
	auto fn = std::function([](double x){return x;});
	auto sel = lazy<simple_selection_evaluator_type>(*this, this->m_loopers.from_slots( [=](looper<dataset_reader_type>& lpr) { return lpr.template filter<Sel>(name,fn); } ));
	return sel;	
}

template <typename T>
template <typename Sel>
auto ana::analysis<T>::channel(const std::string& name) -> lazy<simple_selection_evaluator_type>
{
	auto fn = std::function([](double x){return x;});
	auto sel = lazy<simple_selection_evaluator_type>(*this, this->m_loopers.from_slots( [=](looper<dataset_reader_type>& lpr) { return lpr.template channel<Sel>(name,fn); } ));
	return sel;	
}

template <typename T>
template <typename Sel, typename Lmbd>
auto ana::analysis<T>::filter(lazy<selection> const& prev, const std::string& name, Lmbd lmbd) -> lazy<custom_selection_evaluator_t<Lmbd>>
{
	auto fn = std::function{lmbd};
	return lazy<custom_selection_evaluator_t<Lmbd>>(*this, this->m_loopers.from_slots( [=](looper<dataset_reader_type>& lpr, selection const& prev) { return lpr.template filter<Sel>(prev,name,fn); }, prev.get_slots() ));
}

template <typename T>
template <typename Sel, typename Lmbd>
auto ana::analysis<T>::channel(lazy<selection> const& prev, const std::string& name, Lmbd lmbd) -> lazy<custom_selection_evaluator_t<Lmbd>>
{
	auto fn = std::function{lmbd};
	return lazy<custom_selection_evaluator_t<Lmbd>>(*this, this->m_loopers.from_slots( [=](looper<dataset_reader_type>& lpr, selection const& prev) { return lpr.template channel<Sel>(prev,name,fn); }, prev.get_slots() ));
}

template <typename T>
template <typename Sel>
auto ana::analysis<T>::filter(lazy<selection> const& prev, const std::string& name) -> lazy<simple_selection_evaluator_type>
{
	auto fn = std::function([](double x){return x;});
	return lazy<simple_selection_evaluator_type>(*this, this->m_loopers.from_slots( [=](looper<dataset_reader_type>& lpr, selection const& prev) { return lpr.template filter<Sel>(prev,name,fn); }, prev.get_slots() ));
}

template <typename T>
template <typename Sel>
auto ana::analysis<T>::channel(lazy<selection> const& prev, const std::string& name) -> lazy<simple_selection_evaluator_type>
{
	auto fn = std::function([](double x){return x;});
	return lazy<simple_selection_evaluator_type>(*this, this->m_loopers.from_slots( [=](looper<dataset_reader_type>& lpr, selection const& prev) { return lpr.template channel<Sel>(prev,name,fn); }, prev.get_slots() ));
}

template <typename T>
template <typename Sel, typename... Cols>
auto ana::analysis<T>::evaluate_selection(lazy<selection::evaluator<Sel>> const& calc, lazy<Cols> const&... columns) -> typename ana::analysis<T>::template lazy<selection>
{
	auto sel = lazy<selection>(*this, this->m_loopers.from_slots( [=](looper<dataset_reader_type>& lpr, selection::evaluator<Sel>& calc, Cols&... cols) { return lpr.template evaluate_selection(calc, cols...); }, calc.get_slots(), columns.get_slots()... ));
	this->add_selection(sel);
  return sel;
}

template <typename T>
template <typename Cnt, typename... Args>
auto ana::analysis<T>::book(Args&&... args) -> lazy<ana::counter::booker<Cnt>>
{
	return lazy<counter::booker<Cnt>>(*this, this->m_loopers.from_slots( [&](looper<dataset_reader_type>& lpr) { return lpr.template book<Cnt>(std::forward<Args>(args)...); } ));
}

template <typename T>
template <typename Cnt>
auto ana::analysis<T>::count_selection(lazy<counter::booker<Cnt>> const& bkr, lazy<selection> const& sel) -> lazy<Cnt>
{
	// any time a new counter is booked, means the analysis must run: so reset its status
	this->reset();
	auto cnt = lazy<Cnt>(*this, this->m_loopers.from_slots( [=](looper<dataset_reader_type>& lpr, counter::booker<Cnt>& bkr, const selection& sel) { return lpr.count_selection(bkr,sel); }, bkr.get_slots(), sel.get_slots() ));
	this->add_counter(cnt);
  return cnt;
}

template <typename T>
template <typename Cnt, typename... Sels>
auto ana::analysis<T>::count_selections(lazy<counter::booker<Cnt>> const& bkr, lazy<Sels> const&... sels) -> lazy<counter::booker<Cnt>>
{
	// any time a new counter is booked, means the analysis must run: so reset its status
	this->reset();
	auto bkr2 = lazy<counter::booker<Cnt>>(*this, this->m_loopers.from_slots( [=](looper<dataset_reader_type>& lpr, counter::booker<Cnt>& bkr, Sels const&... sels) { return lpr.count_selections(bkr,sels...); }, bkr.get_slots(), sels.get_slots()... ));
	// add all counters that were booked
	for (auto const& sel_path : bkr2.list_selection_paths()) {
		this->add_counter(bkr2.get_counter_at(sel_path));
	}
  return bkr2;
}

template <typename T>
void ana::analysis<T>::analyze()
{ 
	if (m_analyzed) return;
	this->process_dataset();
	this->clear_counters();
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
	this->m_loopers.to_slots( [] (looper<dataset_reader_type>& lpr) { lpr.clear_counters(); } );
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
		for (size_t islot=0 ; islot<this->m_loopers.concurrency() ; ++islot) {
			pool.emplace_back(
				[] (looper<dataset_reader_type>& lpr) {
					lpr.loop();
				},
				std::ref(*this->m_loopers.get_slot(islot))
			);
		}
		// join threads
		for (auto&& thread : pool) {
			thread.join();
		}
	// single-threaded
	} else {
		for (size_t islot=0 ; islot<this->m_loopers.concurrency() ; ++islot) {
			this->m_loopers.get_slot(islot)->loop();
		}
	}

	// finish
	this->m_dataset->finish();

}

template <typename T>
void ana::analysis<T>::add_column(typename ana::analysis<T>::template lazy<column> lazy)
{
	m_column_list.push_back(lazy);
}

template <typename T>
void ana::analysis<T>::add_selection(typename ana::analysis<T>::template lazy<selection> lazy)
{
	m_selection_list.push_back(lazy);
}

template <typename T>
void ana::analysis<T>::add_counter(typename ana::analysis<T>::template lazy<counter> lazy)
{
	m_counter_list.push_back(lazy);
}

// template <typename T>
// template <typename Cnt>
// auto ana::analysis<T>::repeat_booker(lazy<counter::booker<Cnt>> const& bkr) -> lazy<counter::booker<Cnt>>
// {
// 	return lazy<counter::booker<Cnt>>(*this, this->m_loopers.from_slots( [](looper<dataset_reader_type>& lpr, counter::booker<Cnt> const& bkr){ return lpr.repeat_booker(bkr); }, bkr.get_slots() ));
// }

template <typename... Nodes>
auto ana::list_all_variation_names(Nodes const&... nodes) -> std::set<std::string> {
	std::set<std::string> variation_names;
	(variation_names.merge(nodes.list_variation_names()),...);
	return variation_names;
}

template <typename T>
template <typename V, typename std::enable_if_t<ana::is_column_reader_v<V>, V>* ptr>
auto ana::analysis<T>::vary_column(lazy<V> const&, const std::string& colname) -> lazy<V>
{
  return this->read<cell_value_t<std::decay_t<V>>>(colname);
}

template <typename T>
template <typename Val, typename V, typename std::enable_if_t<ana::is_column_constant_v<V>, V>* ptr>
auto ana::analysis<T>::vary_column(lazy<V> const& nom, Val const& val) -> lazy<V>
{
	return this->constant<Val>(val);
}

template <typename T>
template <typename... Args, typename V, typename std::enable_if_t<ana::is_column_definition_v<V> && !ana::is_column_equation_v<V>, V>* ptr>
auto ana::analysis<T>::vary_definition(lazy<column::evaluator<V>> const&, Args&&... args) -> lazy<column::evaluator<V>>
{
  return this->define<V>(std::forward<Args>(args)...);
}

template <typename T>
template <typename Lmbd, typename V, typename std::enable_if_t<ana::is_column_equation_v<V>, V>* ptr>
auto ana::analysis<T>::vary_equation(lazy<column::evaluator<V>> const& nom, Lmbd lmbd) -> lazy<column::evaluator<V>>
{
	// auto fn = std::function{lmbd};
	return this->calculate<Lmbd>(lmbd);
}