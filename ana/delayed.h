#pragma once

#include <iostream>
#include <type_traits>

#include "ana/analysis.h"
#include "ana/computation.h"
#include "ana/experiment.h"

namespace ana
{

template <typename T>
template <typename U>
class analysis<T>::delayed : public analysis<T>::node<U>
{

public:
	using action_type = U;

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
 	virtual void set_variation(const std::string& varname, const delayed& var) override;

	virtual delayed<U>& get_nominal() override;
	virtual delayed<U>& get_variation(const std::string& varname) override;
	
	virtual bool has_variation(const std::string& varname) const override;
	virtual std::set<std::string> list_variation_names() const override;

	std::string get_name() const
	{
		if constexpr(std::is_base_of_v<selection,U>) {
			return m_threaded.from_model( [] (const selection& sel) { return sel.get_name(); } );
		} else {
			static_assert( (std::is_base_of_v<selection,U>), "not a selection" );
		}
	}

	std::string get_path() const
	{
		if constexpr(std::is_base_of_v<selection,U>) {
			return m_threaded.from_model( [] (const selection& sel) { return sel.get_path(); } );
		} else {
			static_assert((std::is_base_of_v<selection,U>), "not a selection");
		}
	}

	std::string get_full_path() const
	{
		if constexpr(std::is_base_of_v<selection,U>) {
			return m_threaded.from_model( [] (const selection& sel) { return sel.get_full_path(); } );
		} else {
			static_assert((std::is_base_of_v<selection,U>), "not a selection");
		}
	}

	template<typename... Args, typename V=U, typename std::enable_if_t<ana::is_column_calculator_v<V>, V>* = nullptr>
	auto vary(const std::string& varname, Args&&... args) const -> varied<V>;

  template <typename... Nodes, std::enable_if_t<has_no_variation_v<Nodes...>, int> = 0>
	auto evaluate(Nodes... columns) const
	{
		if constexpr( is_column_calculator_v<U> ) {
			auto col = this->m_analysis->compute(*this, columns...);
			this->m_analysis->add_column(col);
			return col;
		} else {
			static_assert( is_column_calculator_v<U> , "non-column cannot be evaluated" );
		}
	}

	template <typename... Nodes, typename V = U, std::enable_if_t<has_variation_v<Nodes...>&&is_column_calculator_v<V>, int> = 0>
	auto evaluate(Nodes... columns) const -> varied<calculated_column_t<V>>
	{
		if constexpr( is_column_calculator_v<V> ) {
			// this is nominal
			varied<calculated_column_t<V>> syst;
			auto nom = this->m_analysis->compute( *this, columns.get_nominal()... );
			syst.set_nominal(nom);
			// variations
			for (auto const& varname : list_all_variation_names(columns...)) {
				auto var = this->m_analysis->compute( *this, columns.get_variation(varname)... );
				syst.set_variation(varname, var);
			}
			return syst;
		} else {
			static_assert( is_column_calculator_v<V> , "not a definition, cannot be evaluated" );
		}
	}

	// filter: regular selection operation
	template <typename Sel, typename... Args>
	using delayed_selection_calculator_t = decltype(std::declval<analysis<T>>().template filter<Sel>(std::declval<std::string>(),std::declval<Args>()...));
  template <typename Sel, typename... Args>
  auto filter(const std::string& name, Args&&... args) -> delayed_selection_calculator_t<Sel,Args...>;
  template <typename Sel, typename... Args>
  auto channel(const std::string& name, Args&&... args) -> delayed_selection_calculator_t<Sel,Args...>;

	template <typename... Nodes, typename V = U, std::enable_if_t<is_selection_calculator_v<V> && has_no_variation_v<Nodes...>, int> = 0>
	auto apply(Nodes const&... columns) -> delayed<typename V::selection_type>
	{
		return this->m_analysis->apply(*this, columns...);
	}

	template <typename... Nodes , std::enable_if_t<(is_varied_v<Nodes>||...), int> = 0>
	auto apply(Nodes... columns) -> varied<U>
	{
		if constexpr(is_selection_calculator_v<U>) {
			auto syst = varied<U>(*this);
			// nominal
			// auto nom = m_analysis->template filter<U>(this->get_name()).apply( columns.get_nominal()...);
			// this->apply( columns.get_nominal()...);
			auto nom = delayed(*this).apply(columns.get_nominal()...);
			syst.set_nominal(*this);
			//variations
			auto varnames = list_all_variation_names(columns...);
			for (auto const& varname : varnames) {
				auto var = delayed(*this).apply( columns.get_variation(varname)...);
				syst.set_variation(varname,var);
			}
			return syst;
		} else {
			static_assert( (is_selection_calculator_v<U>), "cannot apply non-selection" );
		}
	}

	// void weighted(bool weighted=true)
	// {
	// 	if constexpr(std::is_base_of_v<counter,U>) {
	// 		m_threaded.to_slots( [=] (counter& cnt) { cnt.use_weight(weighted); } );
	// 	} else {
	// 		static_assert(std::is_base_of_v<counter,U>, "non-counter cannot be set to be (un-)weighted");
	// 	}
	// }

	template <typename... Nodes , std::enable_if_t<has_no_variation_v<Nodes...>, int> = 0>
	auto fill(Nodes... columns) -> delayed<U>
	{
		if constexpr(is_counter_booker_v<U>) {
			m_threaded.to_slots( [] (U& fillable, typename Nodes::action_type&... cols) { fillable.enter_columns(cols...); }, columns.get_slots()... );
			return *this;
		} else {
			static_assert( (is_counter_booker_v<U>), "non-fillable delayed action" );
		}
	}

	template <typename... Nodes, std::enable_if_t<(is_varied_v<Nodes>||...), int> = 0>
	auto fill(Nodes... columns) -> varied<U>
	{
		if constexpr(is_counter_booker_v<U>) {
			auto syst = varied<U>(*this);
			// important: make sure to always copy the booker,
			// for both nominal & each variation.
			// otherwise if an existing one is used and passed along, 
			// the wrong and extra fill call will be registered.
			// nominal
			auto nom = this->m_analysis->repeat_counter(*this);
			nom.fill(columns.get_nominal()...);
			syst.set_nominal(nom);
			// variations
			for (auto const& varname : list_all_variation_names(columns...)) {
				// copy-construct a new counter booker
				auto var = this->m_analysis->repeat_counter(*this);
				var.fill(columns.get_variation(varname)...);
				syst.set_variation(varname,var);
			}
			return syst;
		} else {
			static_assert( (is_counter_booker_v<U>), "not a counter" );
		}
	}

	template <typename Sel, typename V = U, typename std::enable_if_t<is_counter_booker_v<V>, void>* = nullptr>
	auto at(const delayed<Sel>& sel) const -> delayed<booked_counter_t<V>>
	{
		auto cnt = this->m_analysis->count(*this, sel);
		this->m_analysis->add_counter(cnt);
		return cnt;
	}

	// book 1 x N counting operators
	// 1 filter x N counters
	// 1 counter x N filters
	template <typename... Nodes, std::enable_if_t<has_no_variation_v<Nodes...>, int> = 0>
	auto at(Nodes... sels) const -> delayed<U>
	{
		(this->at(sels),...);
		return *this;
	}

	template <typename V = U, typename std::enable_if<is_counter_booker_v<V>,void>::type* = nullptr>
	auto operator[](const std::string& sel_path) const -> delayed<booked_counter_t<V>>
	{
		return delayed<typename V::counter_type>(*this->m_analysis, m_threaded.from_slots([=](U& bkr){ return bkr.get_counter(sel_path); }) );
	}

	template <typename V = U, typename std::enable_if<is_counter_implemented_v<V>,void>::type* = nullptr>
	decltype(std::declval<V>().result()) result()
	{
		this->m_analysis->analyze();
		this->merge_results();
		return m_threaded.model()->result();
	}

	template <typename... Args>
	auto operator()(Args&&... args)
	{
		constexpr bool valid_args = is_column_calculator_v<U> || is_selection_calculator_v<U>;
		if constexpr( is_column_calculator_v<U> ) {
			return this->evaluate(std::forward<Args>(args)...);
		} else if constexpr( is_selection_calculator_v<U> ) {
			return this->apply(std::forward<Args>(args)...);
		} else {
			static_assert( valid_args, "no valid input operation" );
		}
	}

	analysis<T>* get_analysis() { return this->m_analysis; }
	concurrent<U> const& get_slots() const { 
		return m_threaded; 
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

}

#include "ana/varied.h"
#include "ana/reader.h"
#include "ana/definition.h"
#include "ana/equation.h"

template<typename T>
template<typename Act>
void ana::analysis<T>::delayed<Act>::set_nominal(delayed const& nom)
{
	// get nominal from other action
	this->m_analysis = nom.m_analysis; 
	m_threaded  = nom.m_threaded;
}

template<typename T>
template<typename Act>
void ana::analysis<T>::delayed<Act>::set_variation(const std::string& varname, delayed const&)
{
	// this is nominal -- ignore all variations
	return;
}

template<typename T>
template<typename Act>
typename ana::analysis<T>::template delayed<Act>& ana::analysis<T>::delayed<Act>::get_nominal()
{
	return *this;
}

template<typename T>
template<typename Act>
typename ana::analysis<T>::template delayed<Act>& ana::analysis<T>::delayed<Act>::get_variation(const std::string& varname)
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

template<typename T>
template<typename Act>
template<typename... Args, typename V, typename std::enable_if_t<ana::is_column_calculator_v<V>, V>* ptr> inline
auto ana::analysis<T>::delayed<Act>::vary(const std::string& varname, Args&&... args) const -> varied<V>
{
  // create a delayed varied with the this as nominal
  auto syst = varied<V>(*this);

	// TODO implement systematic variations
	syst.set_nominal(*this);
  syst.set_variation(varname, *this);

  // done
  return syst;
}

template<typename T>
template<typename Act>
template <typename Sel, typename... Args>
auto ana::analysis<T>::delayed<Act>::filter(const std::string& name, Args&&... args) -> delayed_selection_calculator_t<Sel,Args...>
{
	if constexpr(std::is_base_of_v<selection,Act>) {
		auto sel = this->m_analysis->template filter<Sel>(name, std::forward<Args>(args)...);
		return sel;
	} else {
		static_assert(std::is_base_of_v<selection,Act>, "filter must be called from a selection");
	}
}

template<typename T>
template<typename Act>
template <typename Sel, typename... Args>
auto ana::analysis<T>::delayed<Act>::channel(const std::string& name, Args&&... args) -> delayed_selection_calculator_t<Sel,Args...>
{
	if constexpr(std::is_base_of_v<selection,Act>) {
		auto sel = this->m_analysis->template channel<Sel>(name, std::forward<Args>(args)...);
		return sel;
	} else {
		static_assert(std::is_base_of_v<selection,Act>, "filter must be called from a selection");
	}
}

// template<typename T>
// template<typename Act>
// template <typename... Cols>
// void ana::analysis<T>::delayed<Act>::fill(delayed<Cols> const&... columns)
// {
// 	if constexpr(is_counter_booker_v<Act> ||  is_counter_fillable_v<Act>) {
// 		// nominal
// 		m_threaded.to_slots( [] (Act& fillable, Cols&... cols) { fillable.enter_columns(cols...); }, columns.get_slots()... );
// 	} else {
// 		static_assert( (is_counter_booker_v<Act> ||  is_counter_fillable_v<Act>), "non-fillable counter" );
// 	}
// }

// template<typename T>
// template<typename Act>
// template <typename... Nodes>
// auto ana::analysis<T>::delayed<Act>::fill(Nodes... columns) -> varied<Act>
// {
// 	if constexpr(is_counter_booker_v<Act>) {
// 		auto syst = varied<Act>();
// 		// nominal
// 		auto nom = std::make_shared<Act>(*this);
// 		nom.get_slots().to_slots( [] (Act& fillable, typename Nodes::action_type::counter_type&... cols) { fillable.enter_columns(cols...); }, columns.get_nominal().get_slots()... );
// 		syst.set_nominal(nom);
// 		//variations
// 		auto varnames = list_all_variation_names(columns...);
// 		for (auto const& varname : varnames) {
// 			auto var = std::make_shared<Act>(*this);
// 			var.get_slots().to_slots( [] (Act& fillable, typename Nodes::action_type::counter_type&... cols) { fillable.enter_columns(cols...); }, columns.get_variation(varname).get_slots()... );
// 			syst.set_variation(varname,var);
// 		}
// 	} else {
// 		static_assert( (is_counter_booker_v<Act>), "not a counter booker, cannot enter with columns" );
// 	}
// }