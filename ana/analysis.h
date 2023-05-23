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
#include "ana/processor.h"

namespace ana
{

template <typename T> struct is_column_evaluator : std::false_type {};
template <typename T> struct is_column_evaluator<column::evaluator<T>> : std::true_type {};
template <typename T> constexpr bool is_column_evaluator_v = is_column_evaluator<T>::value;

template <typename T> struct is_selection_evaluator: std::false_type {};
template <typename T> struct is_selection_evaluator<selection::evaluator<T>>: std::true_type {};
// template <typename T> struct is_selection_evaluator<selection::weight::evaluator<T>>: std::true_type {};
template <typename T> constexpr bool is_selection_evaluator_v = is_selection_evaluator<T>::value;

// template <typename Lmbd> using function_t = decltype(std::function(std::declval<Lmbd>()));
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
	class delayed;
	template <typename U>
	friend class delayed;

	template <typename U>
	class varied;
	template <typename U>
	friend class varied;

	template <typename U>
	static constexpr std::true_type check_nominal(typename analysis<T>::template delayed<U> const&);
	static constexpr std::false_type check_nominal(...);
	template <typename V> static constexpr bool is_nominal_v = decltype(check_nominal(std::declval<V>()))::value;

	template <typename U>
	static constexpr std::true_type check_varied(typename analysis<T>::template varied<U> const&);
	static constexpr std::false_type check_varied(...);
	template <typename V> static constexpr bool is_varied_v = decltype(check_varied(std::declval<V>()))::value;

	template <typename... Args> static constexpr bool has_no_variation_v = (is_nominal_v<Args>&&...);
	template <typename... Args> static constexpr bool has_variation_v = (is_varied_v<Args>||...);

public:
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
  auto read(const std::string& name) -> delayed<read_column_t<read_dataset_t<T>,Val>>;
  template <typename Val>
  auto constant(const Val& value) -> delayed<column::constant<Val>>;

  template <typename Def, typename... Args, typename = std::enable_if_t<std::is_constructible_v<Def,Args&&...>>>
  auto define(Args&&... arguments) -> delayed<column::evaluator<Def>>;
  template <typename Lmbd, typename = std::enable_if_t<!is_column_definition_v<Lmbd>>>
  auto calculate(Lmbd lmbd) -> delayed<equation_evaluator_t<Lmbd>>;

	template <typename Sel, typename Lmbd>
  auto filter(const std::string& name, Lmbd lmbd) -> delayed<custom_selection_evaluator_t<Lmbd>>;
  template <typename Sel, typename Lmbd>
  auto channel(const std::string& name, Lmbd lmbd) -> delayed<custom_selection_evaluator_t<Lmbd>>;
	template <typename Sel>
  auto filter(const std::string& name) -> delayed<simple_selection_evaluator_type>;
  template <typename Sel>
  auto channel(const std::string& name) -> delayed<simple_selection_evaluator_type>;

	template <typename Cnt, typename... Args>
	auto book(Args&&... args) -> delayed<counter::booker<Cnt>>;

  template <typename Def, typename... Cols>
	auto evaluate_column(delayed<column::evaluator<Def>> const& calc, delayed<Cols> const&... columns) -> delayed<Def>;

	template <typename Sel, typename... Cols>
	auto evaluate_selection(delayed<selection::evaluator<Sel>> const& calc, delayed<Cols> const&... columns) -> delayed<selection>;

	template <typename Cnt>
	auto book_selection(delayed<counter::booker<Cnt>> const& bkr, delayed<selection> const& sel) -> delayed<Cnt>;

	template <typename Cnt, typename... Sels>
	auto book_selections(delayed<counter::booker<Cnt>> const& bkr, delayed<Sels> const&... sels) -> delayed<counter::booker<Cnt>>;

	void clear_counters();

	void analyze();
	void reset();

protected:
	/**
	 * @brief Default constructor for initial flags and values.
	 * @details The dataset pointer remains as an `nullptr`, need to call `prepare`.
	*/
  analysis();

  void process_dataset();

	template <typename Sel, typename Lmbd>
  auto filter(delayed<selection> const& prev, const std::string& name, Lmbd lmbd) -> delayed<custom_selection_evaluator_t<Lmbd>>;
  template <typename Sel, typename Lmbd>
  auto channel(delayed<selection> const& prev, const std::string& name, Lmbd lmbd) -> delayed<custom_selection_evaluator_t<Lmbd>>;
	template <typename Sel>
  auto filter(delayed<selection> const& prev, const std::string& name) -> delayed<simple_selection_evaluator_type>;
  template <typename Sel>
  auto channel(delayed<selection> const& prev, const std::string& name) -> delayed<simple_selection_evaluator_type>;

	// recreate a delayed node as a variation under new arguments
	template <typename V, typename std::enable_if_t<ana::is_column_reader_v<V>, V>* = nullptr>
	auto vary_column(delayed<V> const& nom, const std::string& colname) -> delayed<V>;
	template <typename Val, typename V, typename std::enable_if_t<ana::is_column_constant_v<V>, V>* = nullptr>
	auto vary_column(delayed<V> const& nom, Val const& val) -> delayed<V>;
	template <typename... Args, typename V, typename std::enable_if_t<ana::is_column_definition_v<V> && !ana::is_column_equation_v<V>, V>* = nullptr>
	auto vary_definition(delayed<column::evaluator<V>> const& nom, Args&&... args) -> delayed<column::evaluator<V>>;
	template <typename Lmbd, typename V, typename std::enable_if_t<ana::is_column_equation_v<V>, V>* = nullptr>
	auto vary_equation(delayed<column::evaluator<V>> const& nom, Lmbd lmbd) -> delayed<column::evaluator<V>>;

protected:
	void add_column(delayed<column> const& var);
	void add_selection(delayed<selection> const& sel);
	void add_counter(delayed<counter> const& cnt);

protected:
	bool m_analyzed;

	std::vector<concurrent<column>>    m_column_list;
	std::vector<concurrent<selection>> m_selection_list;
	std::vector<concurrent<counter>>   m_counter_list;

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
	node(analysis<T>& analysis);

	virtual ~node() = default;

public:

	virtual delayed<U> get_nominal() const = 0;
	virtual delayed<U> get_variation(const std::string& var_name) const = 0;

	virtual void set_nominal(delayed<U> const& nom) = 0;
	virtual void set_variation(const std::string& var_name, delayed<U> const& nom) = 0;

	virtual bool has_variation(const std::string& var_name) const = 0;
	virtual std::set<std::string> list_variation_names() const = 0;

protected:
	analysis<T>* m_analysis;

};

template <typename... Nodes>
auto list_all_variation_names(Nodes const&... nodes) -> std::set<std::string>;

}

#include "ana/delayed.h"
#include "ana/varied.h"

// ----------------------------------------------------------------------------
// node
// ----------------------------------------------------------------------------

template <typename T>
template <typename U>
ana::analysis<T>::node<U>::node(analysis<T>& analysis) :
	m_analysis(&analysis)
{}

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
	this->prepare(std::forward<Args>(args)...);
}

template <typename T>
template <typename U, typename>
ana::analysis<T>::analysis(const std::string& key, const std::vector<std::string>& file_paths) :
  analysis<T>::analysis()
{
	this->prepare(key, file_paths);
}

template <typename T>
template <typename U, typename>
ana::analysis<T>::analysis(const std::vector<std::string>& file_paths, const std::string& key) :
  analysis<T>::analysis()
{
	this->prepare(file_paths, key);
}


template <typename T>
template <typename Val>
// typename ana::analysis<T>::template delayed<ana::term<Val>> ana::analysis<T>::read(const std::string& name)
auto ana::analysis<T>::read(const std::string& name) -> delayed<read_column_t<read_dataset_t<T>,Val>>
{
	this->initialize();
	auto nd = delayed<read_column_t<read_dataset_t<T>,Val>>(*this, this->m_processors.get_concurrent_result( [=](processor<dataset_reader_type>& proc) { return proc.template read<Val>(name); }));
	this->add_column(nd);
	return nd;
}

template <typename T>
template <typename Val>
auto ana::analysis<T>::constant(const Val& val) -> delayed<ana::column::constant<Val>>
{
	auto nd = delayed<column::constant<Val>>(*this, this->m_processors.get_concurrent_result( [=](processor<dataset_reader_type>& proc) { return proc.template constant<Val>(val); } ));
	this->add_column(nd);
  return nd;
}

template <typename T>
template <typename Def, typename... Args, typename>
auto ana::analysis<T>::define(Args&&... arguments) -> typename analysis<T>::template delayed<column::evaluator<Def>>
{
	auto nd = delayed<column::evaluator<Def>>(*this, this->m_processors.get_concurrent_result( [&](processor<dataset_reader_type>& proc) { return proc.template define<Def>(arguments...); } ));
	return nd;
}

template <typename T>
template <typename Lmbd, typename>
auto ana::analysis<T>::calculate(Lmbd lmbd) ->  typename analysis<T>::template delayed<equation_evaluator_t<Lmbd>>
{
	auto fn = std::function{lmbd};
	return delayed<equation_evaluator_t<Lmbd>>(*this, this->m_processors.get_concurrent_result( [=](processor<dataset_reader_type>& proc) { return proc.template calculate(fn); } ));
}

template <typename T>
template <typename Def, typename... Cols>
auto ana::analysis<T>::evaluate_column(delayed<column::evaluator<Def>> const& calc, delayed<Cols> const&... columns) -> delayed<Def>
{
	auto col = delayed<Def>(*this, this->m_processors.get_concurrent_result( [=](processor<dataset_reader_type>& proc, column::evaluator<Def>& calc, Cols const&... cols) { return proc.template evaluate_column(calc, cols...); }, calc.get_concurrent(), columns.get_concurrent()... ));
	this->add_column(col);
  return col;
}

template <typename T>
template <typename Sel, typename Lmbd>
auto ana::analysis<T>::filter(const std::string& name, Lmbd lmbd) -> delayed<custom_selection_evaluator_t<Lmbd>>
{
	auto fn = std::function{lmbd};
	return delayed<custom_selection_evaluator_t<Lmbd>>(*this, this->m_processors.get_concurrent_result( [=](processor<dataset_reader_type>& proc) { return proc.template filter<Sel>(name,fn); } ));
}

template <typename T>
template <typename Sel, typename Lmbd>
auto ana::analysis<T>::channel(const std::string& name, Lmbd lmbd) -> delayed<custom_selection_evaluator_t<Lmbd>>
{
	auto fn = std::function{lmbd};
	auto sel = delayed<custom_selection_evaluator_t<Lmbd>>(*this, this->m_processors.get_concurrent_result( [=](processor<dataset_reader_type>& proc) { return proc.template channel<Sel>(name,fn); } ));
	return sel;	
}

template <typename T>
template <typename Sel>
auto ana::analysis<T>::filter(const std::string& name) -> delayed<simple_selection_evaluator_type>
{
	auto fn = std::function([](double x){return x;});
	auto sel = delayed<simple_selection_evaluator_type>(*this, this->m_processors.get_concurrent_result( [=](processor<dataset_reader_type>& proc) { return proc.template filter<Sel>(name,fn); } ));
	return sel;	
}

template <typename T>
template <typename Sel>
auto ana::analysis<T>::channel(const std::string& name) -> delayed<simple_selection_evaluator_type>
{
	auto fn = std::function([](double x){return x;});
	auto sel = delayed<simple_selection_evaluator_type>(*this, this->m_processors.get_concurrent_result( [=](processor<dataset_reader_type>& proc) { return proc.template channel<Sel>(name,fn); } ));
	return sel;	
}

template <typename T>
template <typename Sel, typename Lmbd>
auto ana::analysis<T>::filter(delayed<selection> const& prev, const std::string& name, Lmbd lmbd) -> delayed<custom_selection_evaluator_t<Lmbd>>
{
	auto fn = std::function{lmbd};
	return delayed<custom_selection_evaluator_t<Lmbd>>(*this, this->m_processors.get_concurrent_result( [=](processor<dataset_reader_type>& proc, selection const& prev) { return proc.template filter<Sel>(prev,name,fn); }, prev.get_concurrent() ));
}

template <typename T>
template <typename Sel, typename Lmbd>
auto ana::analysis<T>::channel(delayed<selection> const& prev, const std::string& name, Lmbd lmbd) -> delayed<custom_selection_evaluator_t<Lmbd>>
{
	auto fn = std::function{lmbd};
	return delayed<custom_selection_evaluator_t<Lmbd>>(*this, this->m_processors.get_concurrent_result( [=](processor<dataset_reader_type>& proc, selection const& prev) { return proc.template channel<Sel>(prev,name,fn); }, prev.get_concurrent() ));
}

template <typename T>
template <typename Sel>
auto ana::analysis<T>::filter(delayed<selection> const& prev, const std::string& name) -> delayed<simple_selection_evaluator_type>
{
	auto fn = std::function([](double x){return x;});
	return delayed<simple_selection_evaluator_type>(*this, this->m_processors.get_concurrent_result( [=](processor<dataset_reader_type>& proc, selection const& prev) { return proc.template filter<Sel>(prev,name,fn); }, prev.get_concurrent() ));
}

template <typename T>
template <typename Sel>
auto ana::analysis<T>::channel(delayed<selection> const& prev, const std::string& name) -> delayed<simple_selection_evaluator_type>
{
	auto fn = std::function([](double x){return x;});
	return delayed<simple_selection_evaluator_type>(*this, this->m_processors.get_concurrent_result( [=](processor<dataset_reader_type>& proc, selection const& prev) { return proc.template channel<Sel>(prev,name,fn); }, prev.get_concurrent() ));
}

template <typename T>
template <typename Sel, typename... Cols>
auto ana::analysis<T>::evaluate_selection(delayed<selection::evaluator<Sel>> const& calc, delayed<Cols> const&... columns) -> typename ana::analysis<T>::template delayed<selection>
{
	auto sel = delayed<selection>(*this, this->m_processors.get_concurrent_result( [=](processor<dataset_reader_type>& proc, selection::evaluator<Sel>& calc, Cols&... cols) { return proc.template evaluate_selection(calc, cols...); }, calc.get_concurrent(), columns.get_concurrent()... ));
	this->add_selection(sel);
  return sel;
}

template <typename T>
template <typename Cnt, typename... Args>
auto ana::analysis<T>::book(Args&&... args) -> delayed<ana::counter::booker<Cnt>>
{
	return delayed<counter::booker<Cnt>>(*this, this->m_processors.get_concurrent_result( [&](processor<dataset_reader_type>& proc) { return proc.template book<Cnt>(std::forward<Args>(args)...); } ));
}

template <typename T>
template <typename Cnt>
auto ana::analysis<T>::book_selection(delayed<counter::booker<Cnt>> const& bkr, delayed<selection> const& sel) -> delayed<Cnt>
{
	// any time a new counter is booked, means the analysis must run: so reset its status
	this->reset();
	auto cnt = delayed<Cnt>(*this, this->m_processors.get_concurrent_result( [=](processor<dataset_reader_type>& proc, counter::booker<Cnt>& bkr, const selection& sel) { return proc.book_selection(bkr,sel); }, bkr.get_concurrent(), sel.get_concurrent() ));
	this->add_counter(cnt);
  return cnt;
}

template <typename T>
template <typename Cnt, typename... Sels>
auto ana::analysis<T>::book_selections(delayed<counter::booker<Cnt>> const& bkr, delayed<Sels> const&... sels) -> delayed<counter::booker<Cnt>>
{
	// any time a new counter is booked, means the analysis must run: so reset its status
	this->reset();
	auto bkr2 = delayed<counter::booker<Cnt>>(*this, this->m_processors.get_concurrent_result( [=](processor<dataset_reader_type>& proc, counter::booker<Cnt>& bkr, Sels const&... sels) { return proc.book_selections(bkr,sels...); }, bkr.get_concurrent(), sels.get_concurrent()... ));
	// add all counters that were booked
	for (auto const& sel_path : bkr2.list_selection_paths()) {
		this->add_counter(bkr2.get_counter(sel_path));
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
	this->m_processors.broadcast_all( [] (processor<dataset_reader_type>& proc) { proc.clear_counters(); } );
}

template <typename T>
void ana::analysis<T>::process_dataset()
{
	this->m_dataset->start_dataset();
	this->m_processors.run_slots(
		[](processor<dataset_reader_type>& proc) {
			proc.process();
		}
	);
	this->m_dataset->finish_dataset();
}

template <typename T>
void ana::analysis<T>::add_column(typename ana::analysis<T>::template delayed<column> const& delayed)
{
	m_column_list.emplace_back(delayed.get_concurrent());
}

template <typename T>
void ana::analysis<T>::add_selection(typename ana::analysis<T>::template delayed<selection> const& delayed)
{
	m_selection_list.emplace_back(delayed.get_concurrent());
}

template <typename T>
void ana::analysis<T>::add_counter(typename ana::analysis<T>::template delayed<counter> const& delayed)
{
	m_counter_list.emplace_back(delayed.get_concurrent());
}

template <typename... Nodes>
auto ana::list_all_variation_names(Nodes const&... nodes) -> std::set<std::string> {
	std::set<std::string> variation_names;
	(variation_names.merge(nodes.list_variation_names()),...);
	return variation_names;
}

template <typename T>
template <typename V, typename std::enable_if_t<ana::is_column_reader_v<V>, V>* ptr>
auto ana::analysis<T>::vary_column(delayed<V> const&, const std::string& colname) -> delayed<V>
{
  return this->read<cell_value_t<std::decay_t<V>>>(colname);
}

template <typename T>
template <typename Val, typename V, typename std::enable_if_t<ana::is_column_constant_v<V>, V>* ptr>
auto ana::analysis<T>::vary_column(delayed<V> const& nom, Val const& val) -> delayed<V>
{
	return this->constant<Val>(val);
}

template <typename T>
template <typename... Args, typename V, typename std::enable_if_t<ana::is_column_definition_v<V> && !ana::is_column_equation_v<V>, V>* ptr>
auto ana::analysis<T>::vary_definition(delayed<column::evaluator<V>> const&, Args&&... args) -> delayed<column::evaluator<V>>
{
  return this->define<V>(std::forward<Args>(args)...);
}

template <typename T>
template <typename Lmbd, typename V, typename std::enable_if_t<ana::is_column_equation_v<V>, V>* ptr>
auto ana::analysis<T>::vary_equation(delayed<column::evaluator<V>> const&, Lmbd lmbd) -> delayed<column::evaluator<V>>
{
	// return this->calculate(lmbd);
	typename V::evalfunc_type fn(lmbd);
	return this->calculate(fn);
	// auto fn = std::function<function_t<V>>(lmbd);
	// return this->calculate<decltype(fn)>(fn);
}