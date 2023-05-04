#pragma once

#include <iostream>
#include <type_traits>


#include "ana/analysis.h"
#include "ana/computation.h"
#include "ana/experiment.h"

// operator checks
// https://stackoverflow.com/questions/6534041/how-to-check-whether-operator-exists
#define CHECK_FOR_BINARY_OP(op_name,op_symbol)\
struct has_no_  ## op_name {};\
template <typename T, typename Arg> has_no_ ## op_name operator op_symbol(const T&, const Arg&);\
template <typename T, typename Arg = T> struct has_ ## op_name { enum { value = !std::is_same<decltype(std::declval<T>() op_symbol std::declval<Arg>()), has_no_ ## op_name>::value }; };\
template <typename T, typename Arg = T> static constexpr bool has_ ## op_name ## _v = has_ ## op_name<T,Arg>::value; 
#define CHECK_FOR_UNARY_OP(op_name,op_symbol)\
struct has_no_  ## op_name {};\
template <typename T> has_no_ ## op_name operator op_symbol(const T&);\
template <typename T> struct has_ ## op_name { enum { value = !std::is_same<decltype( op_symbol std::declval<T>()), has_no_ ## op_name>::value }; };\
template <typename T> static constexpr bool has_ ## op_name ## _v = has_ ## op_name<T>::value; 
// operator definitions
#define DEFINE_DELAYED_BINARY_OP(op_name,op_symbol)\
template <typename Arg, typename V = U, typename std::enable_if_t<ana::is_column_v<V> && op_check::has_ ## op_name ## _v<cell_value_t<V>, cell_value_t<typename Arg::action_type>>,V>* = nullptr>\
auto operator op_symbol(Arg const& arg) const\
{\
	return this->m_analysis->define([](cell_value_t<V> const& me, cell_value_t<typename Arg::action_type> const& you){ return me op_symbol you; })(*this,arg);\
}
#define DEFINE_DELAYED_UNARY_OP(op_name,op_symbol)\
template <typename V = U, typename std::enable_if_t<ana::is_column_v<V> && op_check::has_ ## op_name ## _v<cell_value_t<V>>,V>* = nullptr>\
auto operator op_symbol() const\
{\
	return this->m_analysis->define([](cell_value_t<V> const& me){ return (op_symbol me); })(*this);\
}

namespace ana
{

namespace op_check
{
CHECK_FOR_UNARY_OP(logical_not,!)
CHECK_FOR_UNARY_OP(minus,-)
CHECK_FOR_BINARY_OP(addition,+)
CHECK_FOR_BINARY_OP(subtraction,-)
CHECK_FOR_BINARY_OP(multiplication,*)
CHECK_FOR_BINARY_OP(division,/)
CHECK_FOR_BINARY_OP(remainder,%)
CHECK_FOR_BINARY_OP(greater_than,>)
CHECK_FOR_BINARY_OP(less_than,<)
CHECK_FOR_BINARY_OP(greater_than_or_equal_to,>=)
CHECK_FOR_BINARY_OP(less_than_or_equal_to,<=)
CHECK_FOR_BINARY_OP(equality,==)
CHECK_FOR_BINARY_OP(logical_and,&&)
CHECK_FOR_BINARY_OP(logical_or,||)
// subscript operator check
// https://stackoverflow.com/questions/31305894/how-to-check-for-the-existence-of-a-subscript-operator
template <class T, class Index>
struct has_subscript_impl
{
  template <class T1, class IndexDeduced = Index, class Reference = decltype((*std::declval<T*>())[std::declval<IndexDeduced>()]), class = typename std::enable_if<!std::is_void<Reference>::value>::type> static std::true_type test(int);
  template <class> static std::false_type test(...);
  using type = decltype(test<T>(0));
};
template <class T, class Index> using has_subscript = typename has_subscript_impl<T,Index>::type;
template <class T, class Index> static constexpr bool has_subscript_v = has_subscript<T,Index>::value;
}

template <typename Calc> using calculated_column_t = typename Calc::column_type;
template <typename Bkr> using booked_counter_t = typename Bkr::counter_type;

template <typename T>
template <typename U>
class analysis<T>::delayed : public node<U>
{

public:
	using analysis_t = typename node<U>::analysis_type;
	using dataset_type = typename node<U>::dataset_type;
	using action_type = typename node<U>::action_type;

	template <typename Sel, typename... Args>
	using delayed_selection_calculator_t = decltype(std::declval<analysis<T>>().template filter<Sel>(std::declval<std::string>(),std::declval<Args>()...));

	template <typename Sel, typename... Args>
	using selection_calculator_t = typename decltype(std::declval<analysis<T>>().template filter<Sel>(std::declval<std::string>(),std::declval<Args>()...))::action_type;

public:
	// friends with the main analysis graph & any other delayed nodes
	friend class analysis<T>;
	template <typename> friend class delayed;

public:
	delayed() :
		node<U>::node()
	{}

	delayed(analysis<T>& analysis, const concurrent<U>& action) :
		node<U>::node(analysis),
		m_threaded(action)
	{}

	virtual ~delayed() = default;

	template <typename V>
	delayed(const delayed<V>& other) :
		node<U>::node(*other.m_analysis),
		m_threaded(other.m_threaded)
	{}

	template <typename V>
	delayed& operator=(const delayed<V>& other)
	{
		this->m_analysis = other.m_analysis;
		this->m_threaded = other.m_threaded;	
		return *this;
	}

	// implemented as as nominal-only node
	virtual void set_nominal(const delayed& nom) override;
 	virtual void set_variation(const std::string& var_name, const delayed& var) override;

	virtual delayed<U> nominal() const override;
	virtual delayed<U> variation(const std::string& var_name) const override;
	
	virtual bool has_variation(const std::string& var_name) const override;
	virtual std::set<std::string> list_variation_names() const override;

	// ONLY FOR COLUMNS
	// created a varied node that will contain the original as nominal
	// and a single variation under the specified name and constructor arguments
	// further calls to add more variations are handled by varied<V>::vary()
	template <typename... Args, typename V = U, typename std::enable_if_t<ana::is_column_reader_v<V> || ana::is_column_constant_v<V>,V>* = nullptr>
	auto vary(const std::string& var_name, Args&&... args) -> varied<V>;
	template <typename... Args, typename V = U, typename std::enable_if_t<ana::is_column_calculator_v<V>,V>* = nullptr>
	auto vary(const std::string& var_name, Args&&... args) -> varied<V>;

	// evaluate a column definition with input columns
	// propagate any incoming variations through
	template <typename... Args, typename V = U, typename std::enable_if_t<ana::is_column_calculator_v<V>,V>* = nullptr>
	auto evaluate(Args&&... args) const -> decltype(std::declval<delayed<V>>().evaluate_column(std::declval<Args>()...))
	{
		static_assert( is_column_calculator_v<U>, "non-columns cannot be evaluated" );
		return this->evaluate_column(std::forward<Args>(args)...);
	}

  template <typename... Nodes, typename V = U, typename std::enable_if_t<ana::is_column_calculator_v<V> && ana::analysis<T>::template has_no_variation_v<Nodes...>,V>* = nullptr>
	auto evaluate_column(Nodes const&... columns) const -> delayed<calculated_column_t<V>>
	{
		return this->m_analysis->evaluate_column(*this, columns...);
	}

	template <typename... Nodes, typename V = U, typename std::enable_if_t<ana::is_column_calculator_v<V> && ana::analysis<T>::template has_variation_v<Nodes...>,V>* = nullptr>
	auto evaluate_column(Nodes const&... columns) const -> varied<calculated_column_t<V>>
	{
		auto nom = this->m_analysis->evaluate_column( *this, columns.nominal()... );
		varied<calculated_column_t<V>> syst(nom);
		for (auto const& var_name : list_all_variation_names(columns...)) {
			auto var = this->m_analysis->evaluate_column( *this, columns.variation(var_name)... );
			syst.set_variation(var_name, var);
		}
		return syst;
	}

	// ONLY FOR SELECTIONS
	// downstream additional selections	
  template <typename Sel, typename... Args>
  auto filter(const std::string& name, Args&&... args) -> delayed_selection_calculator_t<Sel,Args...>;
  template <typename Sel, typename... Args>
  auto channel(const std::string& name, Args&&... args) -> delayed_selection_calculator_t<Sel,Args...>;

	// apply a selection based on input columns
	// propagate any incoming variations through
	template <typename... Args, typename V = U, typename std::enable_if_t<is_selection_calculator_v<V>,V>* = nullptr>
	auto apply(Args&&... args) const -> decltype(std::declval<delayed<V>>().evaluate_selection(std::declval<Args>()...))
	{
		static_assert( is_selection_calculator_v<U>, "non-selections cannot be applied" );
		return this->evaluate_selection(std::forward<Args>(args)...);
	}

	template <typename... Nodes, typename V = U, typename std::enable_if_t<is_selection_calculator_v<V> && has_no_variation_v<Nodes...>,V>* = nullptr>
	auto evaluate_selection(Nodes const&... columns) const -> delayed<selection>
	{
		return this->m_analysis->evaluate_selection(*this,columns...);
	}

	template <typename... Nodes , typename V = U, typename std::enable_if_t<is_selection_calculator_v<V> && has_variation_v<Nodes...>,V>* = nullptr>
	auto evaluate_selection(Nodes const&... columns) const -> varied<selection>
	{
		varied<selection> syst(this->nominal().evaluate_selection(columns.nominal()...));
		auto var_names = list_all_variation_names(columns...);
		for (auto const& var_name : var_names) {
			syst.set_variation(var_name, this->variation(var_name).evaluate_selection(columns.variation(var_name)...));
		}
		return syst;
	}

	// ONLY FOR COUNTER BOOKERS
	// filling a counter booker results in a new booker with the fill calls booked for any new counters out of it
	template <typename... Args, typename V = U, typename std::enable_if_t<ana::is_counter_booker_v<V>,V>* = nullptr>
	auto fill(Args&&... args) const -> decltype(std::declval<delayed<V>>().enter_columns(std::declval<Args>()...))
	{
		static_assert( is_counter_booker_v<U>, "non-counter(booker) cannot be filled");
		return this->enter_columns(std::forward<Args>(args)...);
	}
	// nominal
	template <typename... Nodes, typename V = U, typename std::enable_if_t<is_counter_booker_v<V> && has_no_variation_v<Nodes...>,V>* = nullptr>
	auto enter_columns(Nodes const&... columns) const -> delayed<V>
	{
		auto filled = delayed<V>(*this->m_analysis, m_threaded.from_slots( [] (U& fillable, typename Nodes::action_type&... cols) { return fillable.book_fill(cols...); }, columns.get_slots()... ));
		return filled;
	}
	// variations
	template <typename... Nodes, typename V = U, typename std::enable_if_t<is_counter_booker_v<V> && has_variation_v<Nodes...>,V>* = nullptr>
	auto enter_columns(Nodes const&... columns) const -> varied<V>
	{
		auto syst = varied<V>(this->enter_columns(columns.nominal()...));
		for (auto const& var_name : list_all_variation_names(columns...)) {
			syst.set_variation(var_name, this->enter_columns(columns.variation(var_name)...));
		}
		return syst;
	}
	// setting selection(s) instantiates the counter(s)
	template <typename Arg>
	auto at(Arg&& arg) const
	// single selection
	{
		static_assert( is_counter_booker_v<U>, "non-counter(booker) cannot be counted at selection" );
		return this->count_selection(std::forward<Arg>(arg));
	}
	template <typename... Args>
	auto at(Args&&... args) const
	// multiple selections
	{
		static_assert( is_counter_booker_v<U>, "non-counter(booker) cannot be counted at selection" );
		return this->count_selections(std::forward<Args>(args)...);
	}
	template <typename Node, typename V = U, std::enable_if_t<is_counter_booker_v<V> && is_nominal_v<Node>, V>* = nullptr>
	auto count_selection(Node const& sel) const -> delayed<booked_counter_t<V>>
	// nominal booker + nominal selection -> nominal counter
	{
		// ensure a copy is used every time
		// auto nom = this->annalysis->repeat_booker(*this);
		return this->m_analysis->count_selection(*this, sel);
	}
	template <typename Node, typename V = U, std::enable_if_t<is_counter_booker_v<V> && is_varied_v<Node>, V>* = nullptr>
	auto count_selection(Node const& sel) const -> varied<booked_counter_t<V>>
	// nominal booker + varied selection -> varied counter
	{
		auto syst = varied<booked_counter_t<V>>(this->m_analysis->count_selection(*this, sel.nominal()));
		for (auto const& var_name : list_all_variation_names(sel)) {
			syst.set_variation(var_name,this->m_analysis->count_selection(*this, sel.variation(var_name)));
		}
		return syst;
	}
	template <typename... Nodes, typename V = U, std::enable_if_t<is_counter_booker_v<V> && has_no_variation_v<Nodes...>, V>* = nullptr>
	auto count_selections(Nodes const&... sels) const -> delayed<V>
	// nominal booker + nominal selections -> nominal booker
	{
		// apply selections in sequence
		return this->m_analysis->count_selections(*this,sels...);
	}
	template <typename... Nodes, typename V = U, std::enable_if_t<is_counter_booker_v<V> && has_variation_v<Nodes...>, V>* = nullptr>
	auto count_selections(Nodes const&... sels) const -> varied<V>
	// nominal booker + varied selections -> varied booker
	{
		auto syst = varied<V>(this->m_analysis->count_selections(*this,sels.nominal()...));
		for (auto const& var_name : list_all_variation_names(sels...)) {
			syst.set_variation(var_name,this->m_analysis->count_selections(*this,sels.variation(var_name)...));
		}
		return syst;
	}
	// access more information about the booked counters
	template <typename V = U, typename std::enable_if_t<is_counter_booker_v<V>,V>* = nullptr>
	auto list_selection_paths() const-> std::vector<std::string>
	{
		return m_threaded.from_model([=](U& bkr){ return bkr.list_selection_paths(); });
	}
	template <typename V = U, typename std::enable_if_t<is_counter_booker_v<V>,V>* = nullptr>
	auto get_counter_at(const std::string& sel_path) const-> delayed<booked_counter_t<V>>
	{
		return delayed<typename V::counter_type>(*this->m_analysis, m_threaded.from_slots([=](U& bkr){ return bkr.get_counter_at(sel_path); }) );
	}

	template <typename V = U, typename std::enable_if<is_counter_implemented_v<V>,void>::type* = nullptr>
	decltype(std::declval<V>().result()) result() const
	{
		this->m_analysis->analyze();
		this->merge_results();
		return m_threaded.model()->result();
	}

	// short-hand operations for some above methods
	template <typename... Args, typename V = U, typename std::enable_if_t<is_column_calculator_v<V>,V>* = nullptr>
	auto operator()(Args&&... args) -> decltype(std::declval<delayed<V>>().evaluate(std::declval<Args>()...))
	// function = evaluate a column based on input columns
	{
		return this->evaluate(std::forward<Args>(args)...);
	}
	template <typename... Args, typename V = U, typename std::enable_if_t<is_selection_calculator_v<V>,V>* = nullptr>
	auto operator()(Args&&... args) -> decltype(std::declval<delayed<V>>().apply(std::declval<Args>()...))
	// function = evaluate a selection based on input columns
	{
		return this->apply(std::forward<Args>(args)...);
	}
	template <typename V = U, typename std::enable_if_t<is_counter_booker_v<V>,V>* = nullptr>
	auto operator[](const std::string& sel_path) const-> delayed<booked_counter_t<V>>
	// subscript = access a counter at a selection path
	{
		return this->get_counter_at(sel_path);
	}

	// access the threaded container
	concurrent<U> const& get_slots() const { return m_threaded; }

	// mathematical operations
	DEFINE_DELAYED_UNARY_OP(logical_not,!)
	DEFINE_DELAYED_UNARY_OP(minus,-)
	DEFINE_DELAYED_BINARY_OP(equality,==)
	DEFINE_DELAYED_BINARY_OP(addition,+)
	DEFINE_DELAYED_BINARY_OP(subtraction,-)
	DEFINE_DELAYED_BINARY_OP(multiplication,*)
	DEFINE_DELAYED_BINARY_OP(division,/)
	DEFINE_DELAYED_BINARY_OP(logical_or,||)
	DEFINE_DELAYED_BINARY_OP(logical_and,&&)
	DEFINE_DELAYED_BINARY_OP(greater_than,>)
	DEFINE_DELAYED_BINARY_OP(less_than,<)
	DEFINE_DELAYED_BINARY_OP(greater_than_or_equal_to,>=)
	DEFINE_DELAYED_BINARY_OP(less_than_or_equal_to,<=)
	template <typename Arg, typename V = U, typename std::enable_if_t<is_column_v<V> && op_check::has_subscript_v<cell_value_t<V>, cell_value_t<typename Arg::action_type>>,V>* = nullptr>
	auto operator[](Arg const& arg) const
	{
		return this->m_analysis->define( [](cell_value_t<V> me, cell_value_t<typename Arg::action_type> index){return me[index];})(*this, arg);
	}

protected:
	template <typename V = U, typename std::enable_if<is_counter_implemented_v<V>,void>::type* = nullptr>
	void merge_results() const
	{
		auto model = m_threaded.model();
		for (size_t islot=1 ; islot<m_threaded.concurrency() ; ++islot) {
			auto slot = m_threaded.get_slot(islot);
			if (!slot->is_merged()) model->merge(slot->result());
			slot->set_merged(true);
		}
	}


protected:
	concurrent<U> m_threaded;

};

// analysis<T> of analysis<T>::node<U>
template <typename T> using analysis_t = typename T::analysis_type;
template <typename T> using action_t = typename T::action_type;

}

#include "ana/varied.h"
#include "ana/reader.h"
#include "ana/definition.h"
#include "ana/equation.h"

template <typename T>
template <typename Act>
void ana::analysis<T>::delayed<Act>::set_nominal(delayed const& nom)
{
	// get nominal from other action
	this->m_analysis = nom.m_analysis; 
	m_threaded  = nom.m_threaded;
}

template <typename T>
template <typename Act>
void ana::analysis<T>::delayed<Act>::set_variation(const std::string&, delayed const&)
{
	// this is nomial -- should never be called!
	throw std::logic_error("cannot vary to a nominal-only delayed action");
}

template <typename T>
template <typename Act>
auto ana::analysis<T>::delayed<Act>::nominal() const -> delayed<Act>
{
	// this is nomial -- return itself
	return *this;
}

template <typename T>
template <typename Act>
auto ana::analysis<T>::delayed<Act>::variation(const std::string&) const -> delayed<Act>
{
	// used when other variations ask the same of this, which it doesn't have -- return itself
	return *this;
}

template <typename T>
template <typename Act>
std::set<std::string>  ana::analysis<T>::delayed<Act>::list_variation_names() const
{
	return std::set<std::string>();
}

template <typename T>
template <typename Act>
bool ana::analysis<T>::delayed<Act>::has_variation(const std::string&) const
{
	return false;
}

template <typename T>
template <typename Act>
template <typename... Args, typename V, typename std::enable_if_t<ana::is_column_reader_v<V> || ana::is_column_constant_v<V>,V>* ptr>
auto ana::analysis<T>::delayed<Act>::vary(const std::string& var_name, Args&&... args) -> varied<V>
{
  // create a delayed varied with the this as nominal
  auto syst = varied<V>(*this);
	// set variation of the column according to new constructor arguments
  syst.set_variation(var_name, this->m_analysis->vary_column(*this, std::forward<Args>(args)...));
  // done
  return syst;
}

template <typename T>
template <typename Act>
template <typename... Args, typename V, typename std::enable_if_t<ana::is_column_calculator_v<V>,V>* ptr>
auto ana::analysis<T>::delayed<Act>::vary(const std::string& var_name, Args&&... args) -> varied<V>
{
  // create a delayed varied with the this as nominal
  auto syst = varied<V>(*this);
	// set variation of the column according to new constructor arguments
  syst.set_variation(var_name, this->m_analysis->vary_definition(*this, std::forward<Args>(args)...));
  // done
  return syst;
}

template <typename T>
template <typename Act>
template <typename Sel, typename... Args>
auto ana::analysis<T>::delayed<Act>::filter(const std::string& name, Args&&... args) -> delayed_selection_calculator_t<Sel,Args...>
{
	if constexpr(std::is_base_of_v<selection,Act>) {
		auto sel = this->m_analysis->template filter<Sel>(*this, name, std::forward<Args>(args)...);
		// sel.get_slots().to_slots( [](typename delayed_selection_calculator_t<Sel,Args...>::action_type& calc, selection const& prev){calc.set_previous(prev);}, this->get_slots() );
		return sel;
	} else {
		static_assert(std::is_base_of_v<selection,Act>, "filter must be called from a selection");
	}
}

template <typename T>
template <typename Act>
template <typename Sel, typename... Args>
auto ana::analysis<T>::delayed<Act>::channel(const std::string& name, Args&&... args) -> delayed_selection_calculator_t<Sel,Args...>
{
	if constexpr(std::is_base_of_v<selection,Act>) {
		auto sel = this->m_analysis->template channel<Sel>(*this, name, std::forward<Args>(args)...);
		// sel.get_slots().to_slots( [](typename delayed_selection_calculator_t<Sel,Args...>::action_type& calc, selection const& prev){calc.set_previous(prev);}, this->get_slots() );
		return sel;
	} else {
		static_assert(std::is_base_of_v<selection,Act>, "channel must be called from a selection");
	}
}