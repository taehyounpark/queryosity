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
	varied(delayed<Act> const& nom) :
	 node<Act>(*nom.m_analysis),
	 m_nominal(nom)
	{}
	~varied() = default;

	varied(varied const& other) :
		node<Act>(*other.m_analysis),
		m_nominal(other.m_nominal),
		m_variation_map(other.m_variation_map)
	{}

	// template <typename Oth>
	varied& operator=(varied const& other)
	{
		m_nominal = other.m_nominal;
		m_variation_map = other.m_variation_map;
		return *this;
	}

	// template <typename Oth>
	// varied& operator=(delayed<Act> const& other)
	// {
	// 	this->m_analysis = other.m_analysis;
	// 	m_nominal = other;
	// 	m_variation_map.clear();
	// 	return *this;
	// }

	virtual void set_nominal(delayed<Act> const& nom) override;
	virtual void set_variation(const std::string& varname, delayed<Act> const& var) override;

	virtual delayed<Act> nominal() const override;
	virtual delayed<Act> variation(const std::string& varname) const override;

	virtual bool has_variation(const std::string& varname) const override;
	virtual std::set<std::string> list_variation_names() const override;

	template <typename... Args, typename V = Act, typename std::enable_if_t<ana::is_column_v<V> || ana::is_column_calculator_v<V>, V>* = nullptr>
	auto vary(const std::string& varname, Args&&... args) const -> varied<V>;

	template <typename... Args, typename V = Act, typename std::enable_if_t<ana::is_column_calculator_v<V>, V>* = nullptr>
	auto evaluate(Args&&... args) -> varied<calculated_column_t<V>>;

	template <typename Sel, typename F, typename V = Act, typename std::enable_if_t<ana::is_selection_v<V>, V>* = nullptr>
  auto filter(const std::string& name, F&& args) -> varied<custom_selection_calculator_t<Sel,F>>;

	template <typename Sel, typename F, typename V = Act, typename std::enable_if_t<ana::is_selection_v<V>, V>* = nullptr>
  auto channel(const std::string& name, F&& args) -> varied<custom_selection_calculator_t<Sel,F>>;

	template <typename Sel, typename V = Act, typename std::enable_if_t<ana::is_selection_v<V>, V>* = nullptr>
  auto filter(const std::string& name) -> varied<simple_selection_calculator_t<Sel>>;

	template <typename Sel, typename V = Act, typename std::enable_if_t<ana::is_selection_v<V>, V>* = nullptr>
  auto channel(const std::string& name) -> varied<simple_selection_calculator_t<Sel>>;

	template <typename... Nodes, typename V = Act, typename std::enable_if_t<ana::is_selection_calculator_v<V>, V>* = nullptr>
	auto apply(Nodes const&... columns) -> varied<booked_counter_t<V>>;

	template <typename... Nodes, typename V = Act, typename std::enable_if_t<ana::is_counter_booker_v<V>, V>* = nullptr>
	auto fill(Nodes const&... columns) -> varied<booked_counter_t<V>>;

	template <typename... Nodes, typename V = Act, typename std::enable_if_t<ana::is_counter_booker_v<V>, V>* = nullptr>
	auto at(Nodes const&... selections) -> varied<typename decltype(std::declval<delayed<V>>().at(selections.nominal()...))::action_type>;

	template <typename... Args>
	auto operator()(Args&&... args) -> varied<typename decltype(std::declval<delayed<Act>>().operator()(std::forward<Args>(args).nominal()...))::action_type>;

	template <typename Arg>
	auto operator/(Arg&& b) const  -> varied<typename decltype(std::declval<delayed<Act>>().operator/(std::forward<Arg>(b).nominal()))::action_type>;

	template <typename V = Act, typename std::enable_if<ana::is_counter_booker_v<V> || ana::is_counter_implemented_v<V>,void>::type* = nullptr>
	auto operator[](const std::string& sel_path) const -> delayed<V>;

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
	m_variation_map.insert(std::make_pair(varname,var));
	m_variation_names.insert(varname);
}

template <typename T>
template <typename Act>
auto ana::analysis<T>::varied<Act>::nominal() const -> delayed<Act>
{
	return m_nominal;
}

template <typename T>
template <typename Act>
auto ana::analysis<T>::varied<Act>::variation(const std::string& varname) const -> delayed<Act>
{
	if (!this->has_variation(varname)) {
		return m_nominal;
	}
	return m_variation_map.at(varname);
}

template <typename T>
template <typename Act>
template <typename V , typename std::enable_if<ana::is_counter_booker_v<V> || ana::is_counter_implemented_v<V>,void>::type* ptr>
auto ana::analysis<T>::varied<Act>::operator[](const std::string& varname) const -> delayed<V>
{
	if (!this->has_variation(varname)) {
		throw std::logic_error(std::string("variation '")+varname+"' does not exist"); 
	}
	return m_variation_map.at(varname);
}

template <typename T>
template <typename Act>
std::set<std::string> ana::analysis<T>::varied<Act>::list_variation_names() const
{
	return m_variation_names;
}

template <typename T>
template <typename Act>
bool ana::analysis<T>::varied<Act>::has_variation(const std::string& varname) const
{
	return m_variation_map.find(varname)!=m_variation_map.end();
}

template <typename T>
template <typename Act>
template<typename... Args, typename V, typename std::enable_if_t<ana::is_column_calculator_v<V>, V>* ptr> inline
auto ana::analysis<T>::varied<Act>::evaluate(Args&&... args) -> typename ana::analysis<T>::template varied<calculated_column_t<V>>
// varied version of evaluating a column
{
	auto nom = nominal().evaluate(std::forward<Args>(args).nominal()...);
	varied<calculated_column_t<V>> syst(nom);
	for (auto const& varname : list_all_variation_names(*this, std::forward<Args>(args)...)) {
		syst.set_variation(varname, variation(varname).evaluate(std::forward<Args>(args).variation(varname)...));
	}
	return syst;
}

template <typename T>
template <typename Act>
template <typename Sel, typename F, typename V, typename std::enable_if_t<ana::is_selection_v<V>, V>* ptr>
auto ana::analysis<T>::varied<Act>::filter(const std::string& name, F&& lmbd) -> varied<custom_selection_calculator_t<Sel,F>>
{
	auto nom = nominal().template filter<Sel>(name,std::forward<F>(lmbd));
	varied<custom_selection_calculator_t<Sel,F>> syst(nom);
	for (auto const& varname : this->list_variation_names()) {
		syst.set_variation(varname, variation(varname).template filter<Sel>(name,std::forward<F>(lmbd)));
	}
	return syst;
}

template <typename T>
template <typename Act>
template <typename Sel, typename F, typename V, typename std::enable_if_t<ana::is_selection_v<V>, V>* ptr>
auto ana::analysis<T>::varied<Act>::channel(const std::string& name, F&& lmbd) -> varied<custom_selection_calculator_t<Sel,F>>
{
	auto nom = nominal().template channel<Sel>(name,std::forward<F>(lmbd));
	varied<custom_selection_calculator_t<Sel,F>> syst(nom);
	for (auto const& varname : this->list_variation_names()) {
		syst.set_variation(varname, variation(varname).template channel<Sel>(name,std::forward<F>(lmbd)));
	}
	return syst;
}

template <typename T>
template <typename Act>
template <typename Sel, typename V, typename std::enable_if_t<ana::is_selection_v<V>, V>* ptr>
auto ana::analysis<T>::varied<Act>::filter(const std::string& name) -> varied<simple_selection_calculator_t<Sel>>
{
	auto nom = nominal().template filter<Sel>(name);
	varied<simple_selection_calculator_t<Sel>> syst(nom);
	for (auto const& varname : this->list_variation_names()) {
		syst.set_variation(varname, variation(varname).template filter<Sel>(name));
	}
	return syst;
}

template <typename T>
template <typename Act>
template <typename Sel, typename V, typename std::enable_if_t<ana::is_selection_v<V>, V>* ptr>
auto ana::analysis<T>::varied<Act>::channel(const std::string& name) -> varied<simple_selection_calculator_t<Sel>>
{
	auto nom = nominal().template channel<Sel>(name);
	varied<simple_selection_calculator_t<Sel>> syst(nom);
	for (auto const& varname : this->list_variation_names()) {
		syst.set_variation(varname, variation(varname).template channel<Sel>(name));
	}
	return syst;
}

template <typename T>
template <typename Act>
template <typename... Nodes, typename V, typename std::enable_if_t<ana::is_counter_booker_v<V>, V>* ptr> inline
auto ana::analysis<T>::varied<Act>::fill(Nodes const&... columns) -> typename ana::analysis<T>::template varied<booked_counter_t<V>>
// varied version of filling a counter with columns
{
	auto nom = nominal().fill(columns.nominal()...);
	varied<booked_counter_t<V>> syst(nom);
	for (auto const& varname : list_all_variation_names(*this, columns...)) {
		syst.set_variation(varname, variation(varname).fill(columns.variation(varname)...));
	}
	return syst;

}

template <typename T>
template <typename Act>
template <typename... Nodes, typename V, typename std::enable_if_t<ana::is_counter_booker_v<V>, V>* ptr> inline
auto ana::analysis<T>::varied<Act>::at(Nodes const&... selections) -> varied<typename decltype(std::declval<delayed<V>>().at(selections.nominal()...))::action_type>
// varied version of booking counter at a selection operation
{
	varied<typename decltype(std::declval<delayed<V>>().at(selections.nominal()...))::action_type> syst(nominal().at(selections.nominal()...));
	for (auto const& varname : list_all_variation_names(*this, selections...)) {
		syst.set_variation(varname, variation(varname).at(selections.variation(varname)...));
	}
	return syst;
}

template <typename T>
template <typename Act>
template <typename... Args, typename V, typename std::enable_if_t<ana::is_column_v<V> || ana::is_column_calculator_v<V>, V>* ptr> inline
auto ana::analysis<T>::varied<Act>::vary(const std::string& varname, Args&&... args) const -> varied<V>
// use its nominal to make another variation
// set it to self and reflect back
{
	auto syst = varied<V>(this->nominal());
	for (auto const& varname : this->list_variation_names()) {
		syst.set_variation(varname, this->variation(varname));
	}
	// set new variation
	syst.set_variation(varname, this->nominal().vary(varname,std::forward<Args>(args)...).variation(varname));	
	return syst;
}

template <typename T>
template <typename Act>
template <typename... Args>
auto ana::analysis<T>::varied<Act>::operator()(Args&&... args) -> varied<typename decltype(std::declval<delayed<Act>>().operator()(std::forward<Args>(args).nominal()...))::action_type>
{
	auto syst = varied<typename decltype(std::declval<delayed<Act>>().operator()(std::forward<Args>(args).nominal()...))::action_type>(nominal().operator()(std::forward<Args>(args).nominal()...));
	for (auto const& varname : list_all_variation_names(*this, std::forward<Args>(args)...)) {
		syst.set_variation(varname, variation(varname).operator()(std::forward<Args>(args).variation(varname)...) );
	}
	return syst;
}

template <typename T>
template <typename Act>
template <typename Arg>
auto ana::analysis<T>::varied<Act>::operator/(Arg&& b) const  -> varied<typename decltype(std::declval<delayed<Act>>().operator/(std::forward<Arg>(b).nominal()))::action_type>
{
	auto syst = varied<typename decltype(std::declval<delayed<Act>>().operator/(std::forward<Arg>(b).nominal()))::action_type>(nominal().operator/(std::forward<Arg>(b).nominal()));
	for (auto const& varname : list_all_variation_names(*this, std::forward<Arg>(b))) {
		syst.set_variation(varname, variation(varname).operator/(std::forward<Arg>(b).variation(varname)) );
	}
	return syst;
}