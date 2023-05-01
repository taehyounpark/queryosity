#pragma once

#include <iostream>
#include <type_traits>


#include "ana/analysis.h"
#include "ana/computation.h"
#include "ana/experiment.h"

// check existience of operators for T
// https://stackoverflow.com/questions/6534041/how-to-check-whether-operator-exists
// binary
#define CHECK_FOR_BINARY_OP(op_name,op_symbol)\
struct has_no_  ## op_name {};\
template <typename T, typename Arg> has_no_ ## op_name operator op_symbol(const T&, const Arg&);\
template <typename T, typename Arg = T> struct has_ ## op_name { enum { value = !std::is_same<decltype(std::declval<T>() == std::declval<Arg>()), has_no_ ## op_name>::value }; };\
template <typename T, typename Arg = T> static constexpr bool has_ ## op_name ## _v = has_ ## op_name<T,Arg>::value; 
#define DEFINE_DELAYED_BINARY_OP(op_name,op_symbol)\
template <typename Arg, typename V = U, typename std::enable_if_t<ana::is_column_v<V> && op_check::has_ ## op_name ## _v<cell_value_t<V>, cell_value_t<typename Arg::action_type>>, V>* = nullptr>\
auto operator op_symbol(const Arg& other) const\
{\
	return this->m_analysis->define([](cell_value_t<V> const& me, cell_value_t<typename Arg::action_type> const& you){ return me op_symbol you; })(*this,other);\
}
// unary
#define CHECK_FOR_UNARY_OP(op_name,op_symbol)\
struct has_no_  ## op_name {};\
template <typename T> has_no_ ## op_name operator op_symbol(const T&);\
template <typename T> struct has_ ## op_name { enum { value = !std::is_same<decltype( op_symbol std::declval<T>()), has_no_ ## op_name>::value }; };\
template <typename T> static constexpr bool has_ ## op_name ## _v = has_ ## op_name<T>::value; 
#define DEFINE_DELAYED_UNARY_OP(op_name,op_symbol)\
template <typename V = U, typename std::enable_if_t<ana::is_column_v<V> && op_check::has_ ## op_name ## _v<cell_value_t<V>>, V>* = nullptr>\
auto operator op_symbol() const\
{\
	return this->m_analysis->define([](cell_value_t<V> const& me){ return (op_symbol me); })(*this);\
}

namespace ana
{

namespace op_check
{
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

CHECK_FOR_UNARY_OP(logical_not,!)
CHECK_FOR_UNARY_OP(minus,-)

// subscript operator needs special treatment
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
template <typename Calc> using calculated_selection_t = typename Calc::selection_type;

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
	~delayed() = default;

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

	virtual void set_nominal(const delayed& nom) override;
 	virtual void set_variation(const std::string& var_name, const delayed& var) override;

	virtual delayed<U> nominal() const override;
	virtual delayed<U> variation(const std::string& var_name) const override;
	
	virtual bool has_variation(const std::string& var_name) const override;
	virtual std::set<std::string> list_variation_names() const override;

	// filter: regular selection operation
  template <typename Sel, typename... Args>
  auto filter(const std::string& name, Args&&... args) -> delayed_selection_calculator_t<Sel,Args...>;
  template <typename Sel, typename... Args>
  auto channel(const std::string& name, Args&&... args) -> delayed_selection_calculator_t<Sel,Args...>;

	// created a varied node that will contain the original as nominal
	// and a single variation under the specified name and constructor arguments
	// further calls to add more variations are handled by varied<V>::vary()
	template <typename... Args, typename V = U, typename std::enable_if_t<ana::is_column_reader_v<V> || ana::is_column_constant_v<V>, V>* = nullptr>
	auto vary(const std::string& var_name, Args&&... args) -> varied<V>;
	template <typename... Args, typename V = U, typename std::enable_if_t<ana::is_column_calculator_v<V>, V>* = nullptr>
	auto vary(const std::string& var_name, Args&&... args) -> varied<V>;

  template <typename... Nodes, std::enable_if_t<has_no_variation_v<Nodes...>, int> = 0>
	auto evaluate(Nodes const&... columns) const
	{
		if constexpr( is_column_calculator_v<U> ) {
			auto col = this->m_analysis->compute(*this, columns...);
			return col;
		} else {
			static_assert( is_column_calculator_v<U> , "non-column cannot be evaluated" );
		}
	}

	template <typename... Nodes, typename V = U, std::enable_if_t<has_variation_v<Nodes...>&&is_column_calculator_v<V>, int> = 0>
	auto evaluate(Nodes const&... columns) const -> varied<calculated_column_t<V>>
	{
		if constexpr( is_column_calculator_v<V> ) {
			// this is nominal
			auto nom = this->m_analysis->compute( *this, columns.nominal()... );
			varied<calculated_column_t<V>> syst(nom);
			// variations
			for (auto const& var_name : list_all_variation_names(columns...)) {
				auto var = this->m_analysis->compute( *this, columns.variation(var_name)... );
				syst.set_variation(var_name, var);
			}
			return syst;
		} else {
			static_assert( is_column_calculator_v<V> , "not a definition, cannot be evaluated" );
		}
	}

	template <typename... Nodes, typename V = U, std::enable_if_t<is_selection_calculator_v<V> && has_no_variation_v<Nodes...>, int> = 0>
	auto apply(Nodes const&... columns) -> delayed<typename V::selection_type>
	{
		return this->m_analysis->apply(*this, columns...);
	}

	template <typename... Nodes , typename V = U, std::enable_if_t<is_selection_calculator_v<V> && has_variation_v<Nodes...>, int> = 0>
	auto apply(Nodes const&... columns) -> varied<calculated_selection_t<V>>
	{
		if constexpr(is_selection_calculator_v<V>) {
			varied<calculated_selection_t<V>> syst(this->nominal().apply(columns.nominal()...));
			auto var_names = list_all_variation_names(columns...);
			for (auto const& var_name : var_names) {
				syst.set_variation(var_name, this->variation(var_name).apply(columns.variation(var_name)...));
			}
			return syst;
		} else {
			static_assert( (is_selection_calculator_v<V>), "cannot apply non-selection" );
		}
	}

	template <typename... Nodes , std::enable_if_t<has_no_variation_v<Nodes...>, int> = 0>
	auto fill(Nodes const&... columns) -> delayed<U>
	{
		if constexpr(is_counter_booker_v<U>) {
			m_threaded.to_slots( [] (U& fillable, typename Nodes::action_type&... cols) { fillable.fill_columns(cols...); }, columns.nominal().get_slots()... );
			return *this;
		} else {
			static_assert( (is_counter_booker_v<U>), "non-fillable delayed action" );
		}
	}

	template <typename... Nodes, std::enable_if_t< has_variation_v<Nodes...>, int> = 0>
	auto fill(Nodes const&... columns) -> varied<U>
	{
		// always repeat the counter, so each fill operation is performed independently
		if constexpr(is_counter_booker_v<U>) {
			// nominal
			auto nom = this->m_analysis->repeat_counter(*this);
			auto syst = varied<U>(nom.fill(columns.nominal()...));
			// variations
			for (auto const& var_name : list_all_variation_names(columns...)) {
				auto var = this->m_analysis->repeat_counter(*this);
				var.fill(columns.variation(var_name)...);
				syst.set_variation(var_name,var);
			}
			return syst;
		} else {
			static_assert( (is_counter_booker_v<U>), "not a counter" );
		}
	}

	template <typename Sel, typename V = U, typename std::enable_if_t<is_counter_booker_v<V>, void>* = nullptr>
	auto at(const delayed<Sel>& sel) const -> delayed<booked_counter_t<V>>
	{
		return this->m_analysis->count(*this, sel);
	}

	template <typename... Nodes, std::enable_if_t<has_no_variation_v<Nodes...>, int> = 0>
	auto at(Nodes... sels) const -> delayed<U>
	{
		(this->at(sels),...);
		return *this;
	}

	template <typename V = U, typename std::enable_if<is_counter_booker_v<V>,void>::type* = nullptr>
	auto operator[](const std::string& sel_path) const -> delayed<booked_counter_t<V>>
	{
		return delayed<typename V::counter_type>(*this->m_analysis, m_threaded.from_slots([=](U& bkr){ return bkr.get_counter_at(sel_path); }) );
	}

	template <typename V = U, typename std::enable_if<is_counter_implemented_v<V>,void>::type* = nullptr>
	decltype(std::declval<V>().result()) result()
	{
		this->m_analysis->analyze();
		this->merge_results();
		return m_threaded.model()->result();
	}

	analysis<T>* get_analysis() { return this->m_analysis; }
	concurrent<U> const& get_slots() const { 
		return m_threaded; 
	}

	template <typename... Args>
	auto operator()(Args&&... args)
	{
		constexpr bool is_calculator_v = is_column_calculator_v<U> || is_selection_calculator_v<U>;
		if constexpr( is_column_calculator_v<U> ) {
			return this->evaluate(std::forward<Args>(args)...);
		} else if constexpr( is_selection_calculator_v<U> ) {
			return this->apply(std::forward<Args>(args)...);
		} else {
			static_assert( is_calculator_v, "no valid input operation" );
		}
	}

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

	DEFINE_DELAYED_UNARY_OP(logical_not,!)
	DEFINE_DELAYED_UNARY_OP(minus,-)

	template <typename Arg, typename V = U, typename std::enable_if_t<is_column_v<V> && op_check::has_subscript_v<cell_value_t<V>, cell_value_t<typename Arg::action_type>>,V>* = nullptr>
	auto operator[](Arg const& arg) const
	{
		// using subscripted_t = decltype(std::declval<cell_value_t<V>>().operator[](std::declval<cell_value_t<typename Arg::action_type>>()));
		return this->m_analysis->define( [](cell_value_t<V> me, cell_value_t<typename Arg::action_type> index){return me[index];})(*this, arg);
	}

protected:
	template <typename V = U, typename std::enable_if<is_counter_implemented_v<V>,void>::type* = nullptr>
	void merge_results()
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
void ana::analysis<T>::delayed<Act>::set_variation(const std::string& var_name, delayed const&)
{
	// this is nominal -- ignore all variations
	return;
}

template <typename T>
template <typename Act>
auto ana::analysis<T>::delayed<Act>::nominal() const -> delayed<Act>
{
	return *this;
}

template <typename T>
template <typename Act>
auto ana::analysis<T>::delayed<Act>::variation(const std::string& var_name) const -> delayed<Act>
{
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
template <typename... Args, typename V, typename std::enable_if_t<ana::is_column_reader_v<V> || ana::is_column_constant_v<V>, V>* ptr>
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
template <typename... Args, typename V, typename std::enable_if_t<ana::is_column_calculator_v<V>, V>* ptr>
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