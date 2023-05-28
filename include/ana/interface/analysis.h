#pragma once

#include <set>
#include <vector>
#include <memory>
#include <string>
#include <type_traits>
#include <functional>
#include <thread>

#include "input.h"
#include "sample.h"
#include "column.h"
#include "selection.h"
#include "counter.h"
#include "concurrent.h"
#include "processor.h"

namespace ana
{

template <typename T> struct is_column_evaluator : std::false_type {};
template <typename T> struct is_column_evaluator<column::evaluator<T>> : std::true_type {};
template <typename T> constexpr bool is_column_evaluator_v = is_column_evaluator<T>::value;

template <typename T> struct is_selection_applicator: std::false_type {};
template <typename T> struct is_selection_applicator<selection::applicator<T>>: std::true_type {};
template <typename T> constexpr bool is_selection_applicator_v = is_selection_applicator<T>::value;

template <typename F> using equation_evaluator_t = typename column::template evaluator<ana::equation_t<F>>;
template <typename F> using custom_selection_applicator_t = typename selection::template applicator<ana::equation_t<F>>;
using simple_selection_applicator_type = typename selection::template applicator<ana::column::equation<double(double)>>;

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
  template <typename Def, typename... Args>
  auto define(Args&&... args) -> lazy<column_evaluator_t<Def>>;
  template <typename F>
  auto define(F callable) -> lazy<column_evaluator_t<F>>;

	template <typename Sel, typename F>
  auto filter(const std::string& name, F callable) -> lazy<custom_selection_applicator_t<F>>;
  template <typename Sel, typename F>
  auto channel(const std::string& name, F callable) -> lazy<custom_selection_applicator_t<F>>;
	template <typename Sel>
  auto filter(const std::string& name) -> lazy<simple_selection_applicator_type>;
  template <typename Sel>
  auto channel(const std::string& name) -> lazy<simple_selection_applicator_type>;

	template <typename Cnt, typename... Args>
	auto book(Args&&... args) -> lazy<counter::booker<Cnt>>;

  template <typename Def, typename... Cols>
	auto evaluate_column(lazy<column::evaluator<Def>> const& calc, lazy<Cols> const&... columns) -> lazy<Def>;
	template <typename Eqn, typename... Cols>
	auto apply_selection(lazy<selection::applicator<Eqn>> const& calc, lazy<Cols> const&... columns) -> lazy<selection>;
	template <typename Cnt>
	auto book_selection(lazy<counter::booker<Cnt>> const& bkr, lazy<selection> const& sel) -> lazy<Cnt>;
	template <typename Cnt, typename... Sels>
	auto book_selections(lazy<counter::booker<Cnt>> const& bkr, lazy<Sels> const&... sels) -> lazy<counter::booker<Cnt>>;

protected:
	void analyze();
	void reset();

	/**
	 * @brief Default constructor for initial flags and values.
	 * @details The dataset pointer remains as an `nullptr`, need to call `prepare`.
	*/
  analysis();

  void process_dataset();

	template <typename Sel, typename F>
  auto filter(lazy<selection> const& prev, const std::string& name, F callable) -> lazy<custom_selection_applicator_t<F>>;
  template <typename Sel, typename F>
  auto channel(lazy<selection> const& prev, const std::string& name, F callable) -> lazy<custom_selection_applicator_t<F>>;
	template <typename Sel>
  auto filter(lazy<selection> const& prev, const std::string& name) -> lazy<simple_selection_applicator_type>;
  template <typename Sel>
  auto channel(lazy<selection> const& prev, const std::string& name) -> lazy<simple_selection_applicator_type>;
	template <typename Sel>
  auto join(lazy<selection> const& a, lazy<selection> const& b) -> lazy<selection>;

	// recreate a lazy node as a variation under new arguments
	template <typename V, typename std::enable_if_t<ana::is_column_reader_v<V>, V>* = nullptr>
	auto vary_column(lazy<V> const& nom, const std::string& colname) -> lazy<V>;
	template <typename Val, typename V, typename std::enable_if_t<ana::is_column_constant_v<V>, V>* = nullptr>
	auto vary_column(lazy<V> const& nom, Val const& val) -> lazy<V>;
	template <typename... Args, typename V, typename std::enable_if_t<ana::is_column_definition_v<V> && !ana::is_column_equation_v<V>, V>* = nullptr>
	auto vary_definition(lazy<column::evaluator<V>> const& nom, Args&&... args) -> lazy<column::evaluator<V>>;
	template <typename F, typename V, typename std::enable_if_t<ana::is_column_equation_v<V>, V>* = nullptr>
	auto vary_equation(lazy<column::evaluator<V>> const& nom, F callable) -> lazy<column::evaluator<V>>;

protected:
	void add_action(lazy<action> const& act);

protected:
	bool m_analyzed;

	std::vector<concurrent<action>> m_actions;

	// std::vector<concurrent<column>>    m_column_list;
	// std::vector<concurrent<selection>> m_selection_list;
	// std::vector<concurrent<counter>>   m_counter_list;

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

	virtual void set_variation(const std::string& var_name, lazy<U> const& nom) = 0;

	virtual lazy<U> get_nominal() const = 0;
	virtual lazy<U> get_variation(const std::string& var_name) const = 0;

	virtual bool has_variation(const std::string& var_name) const = 0;
	virtual std::set<std::string> list_variation_names() const = 0;

protected:
	analysis<T>* m_analysis;

};

template <typename... Nodes>
auto list_all_variation_names(Nodes const&... nodes) -> std::set<std::string>;

}

#include "lazy.h"
#include "varied.h"

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
// typename ana::analysis<T>::template lazy<ana::term<Val>> ana::analysis<T>::read(const std::string& name)
auto ana::analysis<T>::read(const std::string& name) -> lazy<read_column_t<read_dataset_t<T>,Val>>
{
	this->initialize();
	auto nd = lazy<read_column_t<read_dataset_t<T>,Val>>(*this, this->m_processors.get_concurrent_result( [name=name](processor<dataset_reader_type>& proc) { return proc.template read<Val>(name); }));
	this->add_action(nd);
	return nd;
}

template <typename T>
template <typename Val>
auto ana::analysis<T>::constant(const Val& val) -> lazy<ana::column::constant<Val>>
{
	auto nd = lazy<column::constant<Val>>(*this, this->m_processors.get_concurrent_result( [val=val](processor<dataset_reader_type>& proc) { return proc.template constant<Val>(val); } ));
	this->add_action(nd);
  return nd;
}

template <typename T>
template <typename Def, typename... Args>
auto ana::analysis<T>::define(Args&&... args) -> lazy<ana::column_evaluator_t<Def>>
{
	return lazy<column::evaluator<Def>>(*this, this->m_processors.get_concurrent_result( [&args...](processor<dataset_reader_type>& proc) { return proc.template define<Def>(std::forward<Args>(args)...); } ));
}

template <typename T>
template <typename F>
auto ana::analysis<T>::define(F callable) -> lazy<column_evaluator_t<F>>
{
	return lazy<equation_evaluator_t<F>>(*this, this->m_processors.get_concurrent_result( [callable=callable](processor<dataset_reader_type>& proc) { return proc.template define(callable); } ));
}

template <typename T>
template <typename Def, typename... Cols>
auto ana::analysis<T>::evaluate_column(lazy<column::evaluator<Def>> const& calc, lazy<Cols> const&... columns) -> lazy<Def>
{
	auto col = lazy<Def>(*this, this->m_processors.get_concurrent_result( [](processor<dataset_reader_type>& proc, column::evaluator<Def>& calc, Cols const&... cols) { return proc.template evaluate_column(calc, cols...); }, calc, columns... ));
	this->add_action(col);
  return col;
}

template <typename T>
template <typename Sel, typename F>
auto ana::analysis<T>::filter(const std::string& name, F callable) -> lazy<custom_selection_applicator_t<F>>
{
	return lazy<custom_selection_applicator_t<F>>(*this, this->m_processors.get_concurrent_result( [name=name,callable=callable](processor<dataset_reader_type>& proc) { return proc.template filter<Sel>(name,callable); } ));
}

template <typename T>
template <typename Sel, typename F>
auto ana::analysis<T>::channel(const std::string& name, F callable) -> lazy<custom_selection_applicator_t<F>>
{
	return lazy<custom_selection_applicator_t<F>>(*this, this->m_processors.get_concurrent_result( [name=name,callable=callable](processor<dataset_reader_type>& proc) { return proc.template channel<Sel>(name,callable); } ));
}

template <typename T>
template <typename Sel>
auto ana::analysis<T>::filter(const std::string& name) -> lazy<simple_selection_applicator_type>
{
	auto callable = [](double x){return x;};
	auto sel = lazy<simple_selection_applicator_type>(*this, this->m_processors.get_concurrent_result( [name=name,callable=callable](processor<dataset_reader_type>& proc) { return proc.template filter<Sel>(name,callable); } ));
	return sel;	
}

template <typename T>
template <typename Sel>
auto ana::analysis<T>::channel(const std::string& name) -> lazy<simple_selection_applicator_type>
{
	auto callable = [](double x){return x;};
	auto sel = lazy<simple_selection_applicator_type>(*this, this->m_processors.get_concurrent_result( [name=name,callable=callable](processor<dataset_reader_type>& proc) { return proc.template channel<Sel>(name,callable); } ));
	return sel;	
}

template <typename T>
template <typename Sel, typename F>
auto ana::analysis<T>::filter(lazy<selection> const& prev, const std::string& name, F callable) -> lazy<custom_selection_applicator_t<F>>
{
	return lazy<custom_selection_applicator_t<F>>(*this, this->m_processors.get_concurrent_result( [name=name,callable=callable](processor<dataset_reader_type>& proc, selection const& prev) { return proc.template filter<Sel>(prev,name,callable); }, prev ));
}

template <typename T>
template <typename Sel, typename F>
auto ana::analysis<T>::channel(lazy<selection> const& prev, const std::string& name, F callable) -> lazy<custom_selection_applicator_t<F>>
{
	return lazy<custom_selection_applicator_t<F>>(*this, this->m_processors.get_concurrent_result( [name=name,callable=callable](processor<dataset_reader_type>& proc, selection const& prev) { return proc.template channel<Sel>(prev,name,callable); }, prev ));
}

template <typename T>
template <typename Sel>
auto ana::analysis<T>::filter(lazy<selection> const& prev, const std::string& name) -> lazy<simple_selection_applicator_type>
{
	auto callable = [](double x){return x;};
	return lazy<simple_selection_applicator_type>(*this, this->m_processors.get_concurrent_result( [name=name,callable=callable](processor<dataset_reader_type>& proc, selection const& prev) { return proc.template filter<Sel>(prev,name,callable); }, prev ));
}

template <typename T>
template <typename Sel>
auto ana::analysis<T>::channel(lazy<selection> const& prev, const std::string& name) -> lazy<simple_selection_applicator_type>
{
	auto callable = [](double x){return x;};
	return lazy<simple_selection_applicator_type>(*this, this->m_processors.get_concurrent_result( [name=name,callable=callable](processor<dataset_reader_type>& proc, selection const& prev) { return proc.template channel<Sel>(prev,name,callable); }, prev ));
}

template <typename T>
template <typename Eqn, typename... Cols>
auto ana::analysis<T>::apply_selection(lazy<selection::applicator<Eqn>> const& calc, lazy<Cols> const&... columns) -> lazy<selection>
{
	auto sel = lazy<selection>(*this, this->m_processors.get_concurrent_result( [](processor<dataset_reader_type>& proc, selection::applicator<Eqn>& calc, Cols&... cols) { return proc.template apply_selection(calc, cols...); }, calc, columns... ));
	this->add_action(sel);
  return sel;
}

template <typename T>
template <typename Sel>
auto ana::analysis<T>::join(lazy<selection> const& a, lazy<selection> const& b) -> lazy<selection>
{
	auto sel = lazy<selection>(*this, this->m_processors.get_concurrent_result( [](processor<dataset_reader_type>& proc, selection const& a, selection const& b) { return proc.template join<Sel>(a,b); }, a, b ));
	this->add_action(sel);
	return sel;
}

template <typename T>
template <typename Cnt, typename... Args>
auto ana::analysis<T>::book(Args&&... args) -> lazy<counter::booker<Cnt>>
{
	return lazy<counter::booker<Cnt>>(*this, this->m_processors.get_concurrent_result( [&args...](processor<dataset_reader_type>& proc) { return proc.template book<Cnt>(std::forward<Args>(args)...); } ));
}

template <typename T>
template <typename Cnt>
auto ana::analysis<T>::book_selection(lazy<counter::booker<Cnt>> const& bkr, lazy<selection> const& sel) -> lazy<Cnt>
{
	// any time a new counter is booked, means the analysis must run: so reset its status
	this->reset();
	auto cnt = lazy<Cnt>(*this, this->m_processors.get_concurrent_result( [](processor<dataset_reader_type>& proc, counter::booker<Cnt>& bkr, const selection& sel) { return proc.book_selection(bkr,sel); }, bkr, sel ));
	this->add_action(cnt);
  return cnt;
}

template <typename T>
template <typename Cnt, typename... Sels>
auto ana::analysis<T>::book_selections(lazy<counter::booker<Cnt>> const& bkr, lazy<Sels> const&... sels) -> lazy<counter::booker<Cnt>>
{
	// any time a new counter is booked, means the analysis must run: so reset its status
	this->reset();
	auto bkr2 = lazy<counter::booker<Cnt>>(*this, this->m_processors.get_concurrent_result( [](processor<dataset_reader_type>& proc, counter::booker<Cnt>& bkr, Sels const&... sels) { return proc.book_selections(bkr,sels...); }, bkr, sels... ));
	// add all counters that were booked
	for (auto const& sel_path : bkr2.list_selection_paths()) {
		this->add_action(bkr2.get_counter(sel_path));
	}
  return bkr2;
}

template <typename T>
void ana::analysis<T>::analyze()
{ 
	// do not analyze if already done
	if (m_analyzed) return;

	this->m_dataset->start_dataset();

	// multithreaded (if enabled)
	this->m_processors.run_slots(
		[](processor<dataset_reader_type>& proc) {
			proc.process();
		}
	);

	this->m_dataset->finish_dataset();

	// clear counters in counter::experiment
	// if they are present, they will be repeated in future runs
	this->m_processors.call_all( [](processor<dataset_reader_type>& proc){proc.clear_counters();} );

	// ignore future analyze() requests,
	// until reset() is called
	m_analyzed = true;
}

template <typename T>
void ana::analysis<T>::reset()
{ 
	m_analyzed = false;
}

template <typename T>
void ana::analysis<T>::add_action(typename ana::analysis<T>::template lazy<action> const& action)
{
	m_actions.emplace_back(action);
}

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
template <typename F, typename V, typename std::enable_if_t<ana::is_column_equation_v<V>, V>* ptr>
auto ana::analysis<T>::vary_equation(lazy<column::evaluator<V>> const&, F callable) -> lazy<column::evaluator<V>>
{
	return this->define(typename V::function_type(callable));
}