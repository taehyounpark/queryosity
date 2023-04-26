#pragma once

#include "ana/analysis.h"

namespace ana
{

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
		m_analysis(nullptr),
		m_action()
	{}
	node(analysis<T>& analysis, const concurrent<U>& action) :
		m_analysis(&analysis),
		m_action(action)
	{}
	~node() = default;

	template <typename V>
	node(const node<V>& other) :
		m_analysis(other.m_analysis),
		m_action(other.m_action)
	{}

	template <typename V>
	node& operator=(const node<V>& other)
	{
		m_analysis = other.m_analysis;
		m_action = other.m_action;	
		return *this;
	}

	operator varied<U>() const
	{
		auto syst = varied<U>();
		syst.set_nominal(*this);
		return syst;
	}

	void set_nominal(const node& nom);
	void set_variation(const std::string& varname, const node& var);

	auto get_nominal() const -> node const&;
	auto get_variation(const std::string& varname) const -> node const&;

	std::string get_name() const
	{
		if constexpr(std::is_base_of_v<selection,U>) {
			return m_action.from_model( [] (const selection& sel) { return sel.get_name(); } );
		} else {
			static_assert((std::is_base_of_v<selection,U> || std::is_base_of_v<counter,U>), "analysis node is not a selection");
		}
	}

	std::string get_path() const
	{
		if constexpr(std::is_base_of_v<selection,U>) {
			return m_action.from_model( [] (const selection& sel) { return sel.get_path(); } );
		} else {
			static_assert((std::is_base_of_v<selection,U> || std::is_base_of_v<counter,U>), "analysis node is not a selection");
		}
	}

	std::string get_full_path() const
	{
		if constexpr(std::is_base_of_v<selection,U>) {
			return m_action.from_model( [] (const selection& sel) { return sel.get_full_path(); } );
		} else {
			static_assert((std::is_base_of_v<selection,U> || std::is_base_of_v<counter,U>), "analysis node is not a selection");
		}
	}

  template <typename V = U, typename... Vars>
	typename std::enable_if<std::tuple_size<decltype(std::declval<V>().get_arguments())>::value!=0, void>::type evaluate(const node<Vars>&... arguments)
	{
		m_action.to_slots( [] (U& defn, Vars&... args) { defn.set_arguments(args...); }, arguments.get_action()... );
	}

	template<typename... Args>
	auto vary(const std::string& varname, Args&&... args) -> varied<U>;

	template <typename Sel, typename Syst>
  auto filter(const std::string& name, Syst const& column) -> decltype(std::declval<analysis<T>>().template filter<Sel>(name, column));;

  template <typename Sel, typename F, typename... Systs>
  auto filter(const std::string& name, F callable, Systs const&... columns) -> decltype(std::declval<analysis<T>>().template filter<Sel>(name, callable, columns...));;

  // template <typename Sel, typename Var>
  // node<selection> channel(const std::string& name, const node<Var>& column)
	// {
	// 	if constexpr(std::is_base_of_v<selection,U>) {
	// 		return m_analysis->rebase(*this).template channel<Sel>(name, column);
	// 	} else {
	// 		static_assert(std::is_base_of_v<selection,U>, "filter cannot be applied to non-selection node");
	// 	}
	// }

  // template <typename Sel, typename F, typename... Vars>
  // node<selection> channel(const std::string& name, F callable, const node<Vars>&... columns)
	// {
	// 	if constexpr(std::is_base_of_v<selection,U>) {
	// 		return m_analysis->rebase(*this).template channel<Sel>(name, callable, columns...);
	// 	} else {
	// 		static_assert(std::is_base_of_v<selection,U>, "filter cannot be applied to non-selection node");
	// 	}
	// }

	// void weighted(bool weighted=true)
	// {
	// 	if constexpr(std::is_base_of_v<counter,U>) {
	// 		m_action.to_slots( [=] (counter& cnt) { cnt.use_weight(weighted); } );
	// 	} else {
	// 		static_assert(std::is_base_of_v<counter,U>, "non-counter cannot be set to be (un-)weighted");
	// 	}
	// }

	// void scale(double scale)
	// {
	// 	if constexpr(std::is_base_of_v<counter,U>) {
	// 		m_action.to_slots( [=] (counter& cnt) { cnt.set_scale(scale); } );
	// 	} else {
	// 		static_assert(std::is_base_of_v<counter,U>, "non-counter cannot be scaled");
	// 	}
	// }

	template <typename... Cols>
	void fill(node<Cols> const&... columns);

	template <typename... Systs>
	auto fill(Systs const&... columns) -> varied<U>;

	// template <typename Cnt, typename V = U, std::enable_if_t<std::is_base_of_v<selection,V>>* = nullptr>
	// node<Cnt> book(const node<counter::booker<Cnt>>& bkr)
	// {
	// 	return m_analysis->rebase(*this).template book(bkr);
	// }

	// template <typename V = U, typename std::enable_if<is_counter_booker_v<V>,void>::type* = nullptr>
	// node<typename V::counter_type> book(const node<selection>& filter)
	// {
	// 	return m_analysis->rebase(filter).template book(*this);
	// }

	// template <typename... Ats>
	// void book(const node<Ats>&... ats)
	// {
	// 	(this->book(ats),...);
	// }

	template <typename V = U, typename std::enable_if<is_counter_booker_v<V>,void>::type* = nullptr>
	node<typename V::counter_type> operator[](const std::string& sel_path)
	{
		return node<typename V::counter_type>(*m_analysis, m_action.from_slots([=](U& bkr){ return bkr.get_counter(sel_path); }) );
	}

	template <typename V = U, typename std::enable_if<is_counter_implemented_v<V>,void>::type* = nullptr>
	decltype(std::declval<V>().result()) result()
	{
		m_analysis->analyze();
		this->merge_results();
		return m_action.model()->result();
	}

	analysis<T>* get_analysis() { return m_analysis; }
	concurrent<U> const& get_action() const { return m_action; }

protected:
	template <typename V = U, typename std::enable_if<is_counter_implemented_v<V>,void>::type* = nullptr>
	void merge_results()
	{
		auto model = m_action.model();
		for (size_t islot=1 ; islot<m_action.concurrency() ; ++islot) {
			auto slot = m_action.get_slot(islot);
			if (!slot->is_merged()) model->merge(slot->result());
			slot->set_merged(true);
		}
	}

protected:
	analysis<T>*  m_analysis;
	concurrent<U> m_action;

};

}

#include "ana/varied.h"
#include "ana/reader.h"
#include "ana/definition.h"
#include "ana/equation.h"

// template<typename T>
// template<typename Act>
// typename ana::analysis<T>::template varied<Act> ana::analysis<T>::node<Act>::operator() const
// {
// 	auto syst = varied<Act>();
// 	syst.set_nominal(*this);
// 	return syst;
// }

template<typename T>
template<typename Act>
void ana::analysis<T>::node<Act>::set_nominal(node const& nom)
{
	// get nominal from other action
	m_analysis = nom.m_analysis; 
	m_action  = nom.m_action;
}

template<typename T>
template<typename Act>
void ana::analysis<T>::node<Act>::set_variation(const std::string& varname, node const&)
{
	// this is nominal -- ignore all variations
	return;
}

template<typename T>
template<typename Act>
auto ana::analysis<T>::node<Act>::get_nominal() const -> node const&
{
	return *this;
}

template<typename T>
template<typename Act>
auto ana::analysis<T>::node<Act>::get_variation(const std::string& varname) const -> node const&
{
	return *this;
}

template<typename T>
template<typename Act>
template<typename... Args>
auto ana::analysis<T>::node<Act>::vary(const std::string& varname, Args&&... args) -> typename ana::analysis<T>::template varied<Act>
{
  // create a node varied with the current as nominal
  varied<Act> syst;
  syst.set_nominal(*this);

  // vary the column as instructed
  node<Act> var;
  if constexpr(ana::is_column_reader_v<Act>) {
    var = m_analysis->template read<term_value_t<Act>>(std::forward<Args>(args)...);
  } else if constexpr(ana::is_column_definition_v<Act>) {
    var = m_analysis->template define<Act>(std::forward<Args>(args)...);
  } else if constexpr(ana::is_column_equation_v<Act>) {
    var = m_analysis->template evaluate(std::forward<Args>(args)...);
  } else {
    static_assert(std::is_base_of_v<column,Act>, "only columns can be variedally varied");
  }

  // add it as a variation
  syst.set_variation(varname, var);

  // done
  return syst;
}

template<typename T>
template<typename Act>
template <typename Sel, typename Syst>
auto ana::analysis<T>::node<Act>::filter(const std::string& name, Syst const& column) -> decltype(std::declval<analysis<T>>().template filter<Sel>(name, column))
{
	if constexpr(std::is_base_of_v<selection,Act>) {
		auto sel = m_analysis->template filter<Sel>(name, column);
		sel.get_nominal().get_action().to_slots( [](selection& prev){sel.set_previous(prev);}, *this );
		for (auto const& varname : list_applied_variation_names(sel)) {
			sel.get_variation(varname).get_action().to_slots( [](selection& prev){sel.set_previous(prev); }, *this );
		}
	return sel;
	} else {
		static_assert(std::is_base_of_v<selection,Act>, "not a selection, cannot apply another");
	}
}

template<typename T>
template<typename Act>
template <typename Sel, typename F, typename... Systs>
auto ana::analysis<T>::node<Act>::filter(const std::string& name, F callable, Systs const&... columns) -> decltype(std::declval<analysis<T>>().template filter<Sel>(name, callable, columns...))
{
	if constexpr(std::is_base_of_v<selection,Act>) {
		auto sel = m_analysis->template filter<Sel>(name, callable, columns...);
		sel.get_nominal().get_action().to_slots( [](selection& prev){sel.set_previous(prev);}, *this );
		for (auto const& varname : list_applied_variation_names(sel)) {
			sel.get_variation(varname).get_action().to_slots( [](selection& prev){sel.set_previous(prev); }, *this );
		}
		return sel;
	} else {
		static_assert(std::is_base_of_v<selection,Act>, "not a selection, cannot apply another");
	}
}

template<typename T>
template<typename Act>
template <typename... Cols>
void ana::analysis<T>::node<Act>::fill(node<Cols> const&... columns)
{
	if constexpr(is_counter_booker_v<Act> ||  is_counter_fillable_v<Act>) {
		// nominal
		m_action.to_slots( [] (Act& fillable, Cols&... cols) { fillable.enter(cols...); }, columns.get_action()... );
	} else {
		static_assert( (is_counter_booker_v<Act> ||  is_counter_fillable_v<Act>), "non-fillable counter" );
	}
}

template<typename T>
template<typename Act>
template <typename... Systs>
auto ana::analysis<T>::node<Act>::fill(Systs const&... columns) -> varied<Act>
{
	if constexpr(is_counter_booker_v<Act>) {
		auto syst = varied<Act>();
		// nominal
		auto nom = std::make_shared<Act>(*this);
		nom.get_action().to_slots( [] (Act& fillable, typename Systs::action_type::counter_type&... cols) { fillable.enter(cols...); }, columns.get_nominal().get_action()... );
		syst.set_nominal(nom);
		//variations
		auto varnames = list_applied_variation_names(columns...);
		for (auto const& varname : varnames) {
			auto var = std::make_shared<Act>(*this);
			var.get_action().to_slots( [] (Act& fillable, typename Systs::action_type::counter_type&... cols) { fillable.enter(cols...); }, columns.get_variation(varname).get_action()... );
			syst.set_variation(varname,var);
		}
	} else {
		static_assert( (is_counter_booker_v<Act>), "not a counter booker, cannot enter with columns" );
	}
}