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
	template <typename V>
	static constexpr bool is_nominal_v = decltype(check_nominal(std::declval<V>()))::value;

	template <typename U>
	static constexpr std::true_type check_varied(typename analysis<T>::template varied<U> const&);
	static constexpr std::false_type check_varied(...);
	template <typename V>
	static constexpr bool is_varied_v = decltype(check_varied(std::declval<V>()))::value;

	template <typename... Args>
	static constexpr bool has_no_variation_v = (is_nominal_v<Args>&&...);
	template <typename... Args>
	static constexpr bool has_variation_v = (is_varied_v<Args>||...);

	template <typename Sel, typename F>
	using custom_selection_calculator_t = typename Sel::template calculator<equation_t<F>>;
	template <typename Sel>
	using simple_selection_calculator_t = typename Sel::template calculator<equation_t<std::function<double(double)>>>;

public:
  analysis(long long max_entries=-1);
  virtual ~analysis() = default;

	analysis(analysis const&) = delete;
	analysis& operator=(analysis const&) = delete;

  template <typename Val>
  auto read(const std::string& name) -> delayed<input::read_column_t<input::read_dataset_t<T>,Val>>;
  template <typename Val>
  auto constant(const Val& value) -> delayed<column::constant<Val>>;

  template <typename Def, typename... Args>
  auto define(const Args&... arguments) -> delayed<column::calculator<Def>>;
  template <typename F>
  auto define(F callable) -> delayed<column::calculator<equation_t<F>>>;
  template <typename Def, typename... Cols>
	auto compute(delayed<column::calculator<Def>> const& calc, delayed<Cols> const&... columns) -> delayed<Def>;

	template <typename Sel, typename F>
  auto filter(const std::string& name, F lmbd) -> delayed<custom_selection_calculator_t<Sel,F>>;
  template <typename Sel, typename F>
  auto channel(const std::string& name, F lmbd) -> delayed<custom_selection_calculator_t<Sel,F>>;

	template <typename Sel>
  auto filter(const std::string& name) -> delayed<simple_selection_calculator_t<Sel>>;
  template <typename Sel>
  auto channel(const std::string& name) -> delayed<simple_selection_calculator_t<Sel>>;

	template <typename Calc, typename... Cols>
	auto apply(delayed<Calc> const& calc, delayed<Cols> const&... columns) -> delayed<typename Calc::selection_type>;

	template <typename Cnt, typename... Args>
	delayed<counter::booker<Cnt>> book(Args&&... args);

	template <typename Cnt>
	delayed<counter::booker<Cnt>> book(delayed<counter::booker<Cnt>> const& bkr);

	template <typename Cnt, typename Sel>
	delayed<Cnt> count(delayed<counter::booker<Cnt>> const& bkr, delayed<Sel> const& sel);

	void clear_counters();

	void analyze();
	void reset();

protected:
  void process_dataset();

  template <typename Def, typename... Args>
  auto vary_column(delayed<column::calculator<Def>> const& calc, Args&&... args) -> delayed<column::calculator<Def>>;
  template <typename Sel, typename F>
  auto repeat_selection(delayed<custom_selection_calculator_t<Sel,F>> const& calc) -> delayed<custom_selection_calculator_t<Sel,F>>;
  template <typename Cnt>
  auto repeat_counter(delayed<counter::booker<Cnt>> const& bkr) -> delayed<counter::booker<Cnt>>;

protected:
	void add_column(delayed<column> var);
	void add_selection(delayed<selection> sel);
	void add_counter(delayed<counter> cnt);

protected:
	bool m_analyzed;

	std::vector<delayed<column>>    m_column_list;
	std::vector<delayed<selection>> m_selection_list;
	std::vector<delayed<counter>>   m_counter_list;

};

template <typename T>
template <typename U>
class analysis<T>::node
{

public:
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
	~node() = default;

public:

	virtual delayed<U> nominal() const = 0;
	virtual delayed<U> variation(const std::string& varname) const = 0;

	virtual void set_nominal(delayed<U> const& nom) = 0;
	virtual void set_variation(const std::string& varname, delayed<U> const& nom) = 0;

	virtual bool has_variation(const std::string& varname) const = 0;
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
// typename ana::analysis<T>::template delayed<ana::term<Val>> ana::analysis<T>::read(const std::string& name)
auto ana::analysis<T>::read(const std::string& name) -> delayed<input::read_column_t<input::read_dataset_t<T>,Val>>
{
	using column_reader_t = typename decltype(this->m_processors.model()->template read<Val>(name))::element_type;
	auto nd = delayed<column_reader_t>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc) { return proc.template read<Val>(name); } ));
	this->add_column(nd);
	return nd;
}

template <typename T>
template <typename Val>
auto ana::analysis<T>::constant(const Val& val) -> delayed<ana::column::constant<Val>>
{
	auto nd = delayed<term<Val>>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc) { return proc.template constant<Val>(val); } ));
	this->add_column(nd);
  return nd;
}

template <typename T>
template <typename Def, typename... Args>
auto ana::analysis<T>::define(const Args&... arguments) -> typename analysis<T>::template delayed<column::calculator<Def>>
{
	auto nd = delayed<column::calculator<Def>>(*this, this->m_processors.from_slots( [&](processor<dataset_reader_type>& proc) { return proc.template define<Def>(arguments...); } ));
	// this->add_column(nd);
	return nd;
}

template <typename T>
template <typename F>
auto ana::analysis<T>::define(F callable) ->  typename analysis<T>::template delayed<column::calculator<equation_t<F>>>
{
	auto nd = delayed<column::calculator<equation_t<F>>>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc) { return proc.template define(callable); } ));
	// this->add_column(nd);
  return nd;
}

template <typename T>
template <typename Def, typename... Cols>
auto ana::analysis<T>::compute(delayed<column::calculator<Def>> const& calc, delayed<Cols> const&... columns) -> delayed<Def>
{
	auto col = delayed<Def>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc, column::calculator<Def>& calc, Cols const&... cols) { return proc.template compute(calc, cols...); }, calc.get_slots(), columns.get_slots()... ));
	this->add_column(col);
  return col;
}

template <typename T>
template <typename Sel, typename F>
auto ana::analysis<T>::filter(const std::string& name, F lmbd) -> delayed<custom_selection_calculator_t<Sel,F>>
{
	return delayed<custom_selection_calculator_t<Sel,F>>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc) { return proc.template filter<Sel>(name,lmbd); } ));
}

template <typename T>
template <typename Sel, typename F>
auto ana::analysis<T>::channel(const std::string& name, F lmbd) -> delayed<custom_selection_calculator_t<Sel,F>>
{
	auto sel = delayed<custom_selection_calculator_t<Sel,F>>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc) { return proc.template channel<Sel>(name,lmbd); } ));
	return sel;	
}

template <typename T>
template <typename Sel>
auto ana::analysis<T>::filter(const std::string& name) -> delayed<simple_selection_calculator_t<Sel>>
{
	auto sel = delayed<simple_selection_calculator_t<Sel>>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc) { return proc.template filter<Sel>(name,[](double x){return x;}); } ));
	return sel;	
}

template <typename T>
template <typename Sel>
auto ana::analysis<T>::channel(const std::string& name) -> delayed<simple_selection_calculator_t<Sel>>
{
	auto sel = delayed<simple_selection_calculator_t<Sel>>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc) { return proc.template channel<Sel>(name,[](double x){return x;}); } ));
	return sel;	
}

template <typename T>
template <typename Calc, typename... Cols>
auto ana::analysis<T>::apply(delayed<Calc> const& calc, delayed<Cols> const&... columns) -> typename ana::analysis<T>::template delayed<typename Calc::selection_type>
{
	auto sel = delayed<typename Calc::selection_type>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc, Calc& calc, Cols&... cols) { return proc.template apply(calc, cols...); }, calc.get_slots(), columns.get_slots()... ));
	this->add_selection(sel);
  return sel;
}

template <typename T>
template <typename Cnt, typename... Args>
typename ana::analysis<T>::template delayed<ana::counter::booker<Cnt>> ana::analysis<T>::book(Args&&... args)
{
	auto bkr = delayed<counter::booker<Cnt>>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc) { return proc.template book<Cnt>(args...); } ));
  return bkr;
}

template <typename T>
template <typename Cnt>
typename ana::analysis<T>::template delayed<ana::counter::booker<Cnt>> ana::analysis<T>::book(delayed<counter::booker<Cnt>> const& nom)
{
	auto bkr = delayed<counter::booker<Cnt>>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc) { return proc.template book<Cnt>(nom); } ));
  return bkr;
}

template <typename T>
template <typename Cnt, typename Sel>
typename ana::analysis<T>::template delayed<Cnt> ana::analysis<T>::count(delayed<counter::booker<Cnt>> const& bkr, delayed<Sel> const& sel)
{
	// any time a new counter is booked, means the analysis must run: so reset its status
	this->reset();
	auto cnt = delayed<Cnt>(*this, this->m_processors.from_slots( [=](processor<dataset_reader_type>& proc, counter::booker<Cnt>& bkr, Sel const& sel) { return proc.template count<Cnt>(bkr,sel); }, bkr.get_slots(), sel.get_slots() ));
	this->add_counter(cnt);
  return cnt;
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

// template <typename T>
// ana::analysis<T>& ana::analysis<T>::rebase(const delayed<selection>& sel)
// {
// 	this->m_processors.to_slots( [] (processor<dataset_reader_type>& proc, selection& sel) { proc.rebase(sel); }, sel.get_slots() );	
//   return *this;
// }

template <typename T>
void ana::analysis<T>::add_column(typename ana::analysis<T>::template delayed<column> delayed)
{
	m_column_list.push_back(delayed);
}

template <typename T>
void ana::analysis<T>::add_selection(typename ana::analysis<T>::template delayed<selection> delayed)
{
	m_selection_list.push_back(delayed);
}

template <typename T>
void ana::analysis<T>::add_counter(typename ana::analysis<T>::template delayed<counter> delayed)
{
	m_counter_list.push_back(delayed);
}

template <typename T>
template <typename Def, typename... Args>
auto ana::analysis<T>::vary_column(delayed<column::calculator<Def>> const& calc, Args&&... args) -> delayed<column::calculator<Def>>
{
	return delayed<column::calculator<Def>>(*this, this->m_processors.from_slots( [&](processor<dataset_reader_type>& proc, column::calculator<Def> const& calc){ return proc.vary_column(calc, std::forward<Args>(args)...); }, calc.get_slots() ));
}

template <typename T>
template <typename Sel, typename F>
auto ana::analysis<T>::repeat_selection(delayed<custom_selection_calculator_t<Sel,F>> const& calc) -> delayed<custom_selection_calculator_t<Sel,F>>
{
	return delayed<custom_selection_calculator_t<Sel,F>>(*this, this->m_processors.from_slots( [](processor<dataset_reader_type>& proc, custom_selection_calculator_t<Sel,F> const& calc){ return proc.repeat_selection(calc); }, calc.get_slots() ));
}

template <typename T>
template <typename Cnt>
auto ana::analysis<T>::repeat_counter(delayed<counter::booker<Cnt>> const& bkr) -> delayed<counter::booker<Cnt>>
{
	return delayed<counter::booker<Cnt>>(*this, this->m_processors.from_slots( [](processor<dataset_reader_type>& proc, counter::booker<Cnt> const& bkr){ return proc.repeat_counter(bkr); }, bkr.get_slots() ));
}

template <typename... Nodes>
auto ana::list_all_variation_names(Nodes const&... nodes) -> std::set<std::string> {
	std::set<std::string> variation_names;
	(variation_names.merge(nodes.list_variation_names()),...);
	// (variation_names.insert(nodes.list_variation_names().begin(),nodes.list_variation_names().end()),...);
	return variation_names;
}