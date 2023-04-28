#pragma once

#include <set>
#include <unordered_map>
#include <type_traits>

#include "ana/analysis.h"
#include "ana/delayed.h"

namespace ana
{

template <typename T>
template <typename Act>
class analysis<T>::varied : public node<Act>
{

public:
	using action_type = typename node<Act>::action_type;

	template <typename Sel, typename... Args>
	using selection_calculator_t = typename decltype(std::declval<analysis<T>>().template filter<Sel>(std::declval<std::string>(),std::declval<Args>()...))::action_type;

public:
	friend class analysis<T>;
	template <typename> friend class delayed;

public:
	// varied() :
	// 	node<Act>(),
	// 	m_nominal(),
	// 	m_variation_map()
	// {}
	varied(delayed<Act> const& nom) :
	 node<Act>(*nom.m_analysis),
	 m_nominal(nom)
	{}
	~varied() = default;

	template <typename Oth>
	varied(const varied<Oth>& other) :
		node<Act>(*other.m_analysis),
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
	varied(const delayed<Oth>& other) :
		node<Act>(*other.m_analysis),
		m_nominal(other),
		m_variation_map()
	{}

	template <typename Oth>
	varied& operator=(const delayed<Oth>& other)
	{
		this->m_analysis = other.m_analysis;
		m_nominal = other;
		m_variation_map.clear();
		return *this;
	}

	virtual void set_nominal(delayed<Act> const& nom) override;
	virtual void set_variation(const std::string& varname, delayed<Act> const& var) override;

	virtual delayed<Act>& get_nominal() override;
	virtual delayed<Act>& get_variation(const std::string& varname) override;

	virtual bool has_variation(const std::string& varname) const override;
	virtual std::set<std::string> list_variation_names() const override;

	template <typename... Args, typename V = Act, typename std::enable_if_t<ana::is_column_calculator_v<V>, V>* = nullptr>
	auto evaluate(Args&&... args) -> varied<calculated_column_t<V>>;

	template <typename Sel, typename... Args, typename V = Act, typename std::enable_if_t<ana::is_selection_v<V>, V>* = nullptr>
  auto filter(const std::string& name, Args&&... args) -> varied<selection_calculator_t<Sel,Args...>>;

	template <typename Sel, typename... Args, typename V = Act, typename std::enable_if_t<ana::is_selection_v<V>, V>* = nullptr>
  auto channel(const std::string& name, Args&&... args) -> varied<selection_calculator_t<Sel,Args...>>;

	template <typename... Nodes, typename V = Act, typename std::enable_if_t<ana::is_selection_calculator_v<V>, V>* = nullptr>
	auto apply(Nodes... columns) -> varied<booked_counter_t<V>>;

	template <typename... Nodes, typename V = Act, typename std::enable_if_t<ana::is_counter_booker_v<V>, V>* = nullptr>
	auto fill(Nodes... columns) -> varied<booked_counter_t<V>>;

	template <typename... Nodes, typename V = Act, typename std::enable_if_t<ana::is_counter_booker_v<V>, V>* = nullptr>
	auto at(Nodes... selections) -> varied<typename decltype(std::declval<delayed<V>>().at(selections.get_nominal()...))::action_type>;

protected:
	delayed<Act>                                 m_nominal;
	std::unordered_map<std::string,delayed<Act>> m_variation_map;
	std::set<std::string>                        m_variation_names;

};

}

template <typename T>
template <typename Act>
void ana::analysis<T>::varied<Act>::set_nominal(delayed<Act> const& nom)
{
	m_nominal = nom;
}

template <typename T>
template <typename Act>
void ana::analysis<T>::varied<Act>::set_variation(const std::string& varname, delayed<Act> const& var)
{
	m_variation_map[varname] = var;
	m_variation_names.insert(varname);
}

template <typename T>
template <typename Act>
auto ana::analysis<T>::varied<Act>::get_nominal() -> typename ana::analysis<T>::template delayed<Act>&
{
	return m_nominal;
}

template <typename T>
template <typename Act>
auto ana::analysis<T>::varied<Act>::get_variation(const std::string& varname) -> typename ana::analysis<T>::template delayed<Act>&
{
	if (!this->has_variation(varname)) {
		return m_nominal;
	}
	return m_variation_map[varname];
}


template <typename T>
template <typename Act>
std::set<std::string>  ana::analysis<T>::varied<Act>::list_variation_names() const
{
	return m_variation_names;
}

template <typename T>
template <typename Act>
bool ana::analysis<T>::varied<Act>::has_variation(const std::string& varname) const
{
	return m_variation_map.find(varname)==m_variation_map.end();
}

template <typename T>
template <typename Act>
template<typename... Args, typename V, typename std::enable_if_t<ana::is_column_calculator_v<V>, V>* ptr> inline
auto ana::analysis<T>::varied<Act>::evaluate(Args&&... args) -> typename ana::analysis<T>::template varied<calculated_column_t<V>>
{
	// nominal
	auto nom = get_nominal().evaluate(std::forward<Args>(args)...);
	varied<calculated_column_t<V>> syst(nom);
	// variations
	for (auto const& varname : list_all_variation_names(*this, std::forward<Args>(args)...)) {
		syst.set_variation(varname, get_variation(varname).evaluate(std::forward<Args>(args)...));
	}
	return syst;
}

template <typename T>
template <typename Act>
template <typename... Nodes, typename V, typename std::enable_if_t<ana::is_counter_booker_v<V>, V>* ptr> inline
auto ana::analysis<T>::varied<Act>::fill(Nodes... columns) -> typename ana::analysis<T>::template varied<booked_counter_t<V>>
{
	// nominal
	auto nom = get_nominal().fill(columns.get_nominal()...);
	varied<booked_counter_t<V>> syst(nom);
	// variations
	for (auto const& varname : list_all_variation_names(*this, columns...)) {
		syst.set_variation(varname, get_variation(varname).fill(columns.get_variation(varname)...));
	}
	return syst;

}

template <typename T>
template <typename Act>
template <typename... Nodes, typename V, typename std::enable_if_t<ana::is_counter_booker_v<V>, V>* ptr> inline
// auto ana::analysis<T>::varied<Act>::at(Nodes... selections) -> typename ana::analysis<T>::template varied<V>
auto ana::analysis<T>::varied<Act>::at(Nodes... selections) -> varied<typename decltype(std::declval<delayed<V>>().at(selections.get_nominal()...))::action_type>
{
	// nominal
	varied<typename decltype(std::declval<delayed<V>>().at(selections.get_nominal()...))::action_type> syst(get_nominal().at(selections.get_nominal()...));
	// variations
	for (auto const& varname : list_all_variation_names(*this, selections...)) {
		syst.set_variation(varname, get_variation(varname).at(selections.get_variation(varname)...));
	}
	return syst;
}