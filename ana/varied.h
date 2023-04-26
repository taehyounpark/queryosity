#pragma once

#include <set>
#include <unordered_map>

#include "ana/analysis.h"

namespace ana
{

template <typename Act>
class nominal
{

protected:

};

template <typename... Systs>
auto list_applied_variation_names(const Systs&... systs) -> std::set<std::string>;

template <typename T>
template <typename Act>
class analysis<T>::varied
{

public:
	using action_type = Act;

public:
	friend class analysis<T>;
	template <typename> friend class node;

public:
	varied() = default;
	~varied() = default;

	template <typename Oth>
	varied(const varied<Oth>& other) :
		m_nominal(other.m_nominal),
		m_variation_map(other.m_variation_map)
	{}

	template <typename Oth>
	varied& operator=(const varied<Oth>& other)
	{
		m_nominal = other.m_nominal;
		m_variation_map = other.m_variation_map;
		return *this;
	}

	template <typename Oth>
	varied(const node<Oth>& other) :
		m_nominal(other),
		m_variation_map()
	{}

	template <typename Oth>
	varied& operator=(const node<Oth>& other)
	{
		m_nominal = other;
		return *this;
	}

	void set_nominal(node<Act> nom);
	void set_variation(const std::string& varname, node<Act> var);

	auto get_nominal() const -> node<Act> const&;
	auto get_variation(const std::string& varname) const -> node<Act> const&;

	bool has_variation(const std::string& varname)    const;
	std::set<std::string> list_applied_variation_names() const;

	// //
	//   template <typename Sel, typename Var>
  // varied<selection> filter(const std::string& name, const node<Var>& column)
	// {
	// 	if constexpr(std::is_base_of_v<selection,U>) {
	// 		return m_analysis->rebase(*this).template filter<Sel>(name, column);
	// 	} else {
	// 		static_assert(std::is_base_of_v<selection,U>, "filter cannot be applied to non-selection node");
	// 	}
	// }

  // template <typename Sel, typename F, typename... Vars>
  // varied<selection> filter(const std::string& name, F callable, const node<Vars>&... columns)
	// {
	// 	if constexpr(std::is_base_of_v<selection,U>) {
	// 		return m_analysis->rebase(*this).template filter<Sel>(name, callable, columns...);
	// 	} else {
	// 		static_assert(std::is_base_of_v<selection,U>, "filter cannot be applied to non-selection node");
	// 	}
	// }

  // template <typename Sel, typename Var>
  // varied<selection> channel(const std::string& name, const node<Var>& column)
	// {
	// 	if constexpr(std::is_base_of_v<selection,U>) {
	// 		return m_analysis->rebase(*this).template channel<Sel>(name, column);
	// 	} else {
	// 		static_assert(std::is_base_of_v<selection,U>, "filter cannot be applied to non-selection node");
	// 	}
	// }

  // template <typename Sel, typename F, typename... Vars>
  // varied<selection> channel(const std::string& name, F callable, const node<Vars>&... columns)
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

	// template <typename... Cols>
	// void fill(const node<Cols>&... columns)
	// {
	// 	if constexpr( is_counter_booker_v<U> ) {
	// 		m_action.to_slots( [] (U& bkr, Cols&... cols) { bkr.enter(cols...); }, columns.get_action()... );
	// 	} else if constexpr( is_counter_fillable_v<U> ) {
	// 		m_action.to_slots( [] (U& fillable, Cols&... cols) { fillable.enter(cols...); }, columns.get_action()... );
	// 	} else {
	// 		static_assert( (is_counter_booker_v<U> ||  is_counter_fillable_v<U>), "non-fillable counter" );
	// 	}
	// }

	template <typename... Cols>
	void fill(const node<Cols>&... columns);

	// template <typename Cnt, typename V = U, std::enable_if_t<std::is_base_of_v<selection,V>>* = nullptr>
	// node<Cnt> book(const varied<counter::booker<Cnt>>& bkr)
	// {
	// 	return m_analysis->rebase(*this).template book(bkr);
	// }

	// template <typename V = U, typename std::enable_if<is_counter_booker_v<V>,void>::type* = nullptr>
	// node<typename V::counter_type> book(const varied<selection>& filter)
	// {
	// 	return m_analysis->rebase(filter).template book(*this);
	// }

	// template <typename... Ats>
	// void book(const node<Ats>&... ats)
	// {
	// 	(this->book(ats),...);
	// }

protected:
	node<Act>                                 m_nominal;
	std::unordered_map<std::string,node<Act>> m_variation_map;
	std::set<std::string>                     m_applied_variation_names;

};

template <typename T, typename U>
constexpr std::true_type check_varied(const typename analysis<T>::template varied<U>&);
constexpr std::false_type check_varied(...);
template <typename T>
constexpr bool is_varied_v = decltype(check_varied(std::declval<T>()))::value;

}

#include "ana/node.h"

template <typename T>
template <typename Act>
void ana::analysis<T>::varied<Act>::set_nominal(node<Act> nom)
{
	m_nominal = nom;
}

template <typename T>
template <typename Act>
void ana::analysis<T>::varied<Act>::set_variation(const std::string& varname, node<Act> var)
{
	m_variation_map[varname] = var;
	m_applied_variation_names.insert(varname);
}

template <typename T>
template <typename Act>
auto ana::analysis<T>::varied<Act>::get_nominal() const -> typename ana::analysis<T>::template node<Act> const&
{
	return m_nominal;
}

template <typename T>
template <typename Act>
std::set<std::string>  ana::analysis<T>::varied<Act>::list_applied_variation_names() const
{
	return m_applied_variation_names;
}

template <typename T>
template <typename Act>
auto ana::analysis<T>::varied<Act>::get_variation(const std::string& varname) const -> typename ana::analysis<T>::template node<Act> const&
{
	if (!this->has_variation(varname)) {
		return m_nominal;
	}
	return m_variation_map[varname];
}


template <typename T>
template <typename Act>
bool ana::analysis<T>::varied<Act>::has_variation(const std::string& varname) const
{
	return m_variation_map.find(varname)==m_variation_map.end();
}

template <typename... Systs>
auto ana::list_applied_variation_names(Systs const&... systs) -> std::set<std::string> {
	std::set<std::string> applied_variation_names;
	(applied_variation_names.insert(systs.list_applied_variation_names().begin(),systs.list_applied_variation_names().end()),...);
	return applied_variation_names;
}

template<typename T>
template<typename Act>
template <typename... Cols>
void ana::analysis<T>::varied<Act>::fill(node<Cols> const&... columns)
{
	if constexpr(is_counter_booker_v<Act> ||  is_counter_fillable_v<Act>) {
		// nominal
		m_nominal.to_slots( [] (Act& fillable, Cols&... cols) { fillable.enter(cols...); }, columns.get_action()... );
		for (auto const& varname : m_applied_variation_names) {
			m_variation_map[varname].to_slots( [] (Act& fillable, Cols&... cols) { fillable.enter(cols...); }, columns.get_action()... );
		}
	} else {
		static_assert( (is_counter_booker_v<Act> ||  is_counter_fillable_v<Act>), "non-fillable counter" );
	}
}