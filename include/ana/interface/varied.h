#pragma once

#include <set>
#include <unordered_map>
#include <type_traits>
#include <utility>

#include "analysis.h"
#include "lazy.h"
#include "column.h"

#define DECLARE_VARIED_BINARY_OP(op_symbol)\
template <typename Arg>\
auto operator op_symbol(Arg&& b) const  -> varied<typename decltype(std::declval<lazy<Act>>().operator op_symbol(std::forward<Arg>(b).get_nominal()))::action_type>;
#define DEFINE_VARIED_BINARY_OP(op_symbol)\
template <typename T>\
template <typename Act>\
template <typename Arg>\
auto ana::analysis<T>::varied<Act>::operator op_symbol(Arg&& b) const  -> varied<typename decltype(std::declval<lazy<Act>>().operator  op_symbol(std::forward<Arg>(b).get_nominal()))::action_type>\
{\
	auto syst = varied<typename decltype(std::declval<lazy<Act>>().operator  op_symbol(std::forward<Arg>(b).get_nominal()))::action_type>(this->get_nominal().operator op_symbol(std::forward<Arg>(b).get_nominal()));\
	for (auto const& var_name : list_all_variation_names(*this, std::forward<Arg>(b))) {\
		syst.set_variation(var_name, get_variation(var_name).operator op_symbol(std::forward<Arg>(b).get_variation(var_name)) );\
	}\
	return syst;\
}
#define DECLARE_VARIED_UNARY_OP(op_symbol)\
template <typename V = Act, std::enable_if_t<ana::is_column_v<V>, bool> = false>\
auto operator op_symbol() const  -> varied<typename decltype(std::declval<lazy<V>>().operator op_symbol())::action_type>;
#define DEFINE_VARIED_UNARY_OP(op_name,op_symbol)\
template <typename T>\
template <typename Act>\
template <typename V, std::enable_if_t<ana::is_column_v<V>,bool>>\
auto ana::analysis<T>::varied<Act>::operator op_symbol() const  -> varied<typename decltype(std::declval<lazy<V>>().operator  op_symbol())::action_type>\
{\
	auto syst = varied<typename decltype(std::declval<lazy<V>>().operator  op_symbol())::action_type>(this->get_nominal().operator op_symbol());\
	for (auto const& var_name : list_all_variation_names(*this)) {\
		syst.set_variation(var_name, get_variation(var_name).operator op_symbol());\
	}\
	return syst;\
}

namespace ana
{

template <typename T>
template <typename Act>
class analysis<T>::varied : public node<Act>
{

public:
	using analysis_t = typename node<Act>::analysis_type;
	using dataset_type = typename node<Act>::dataset_type;
	using action_type = typename node<Act>::action_type;

	template <typename Sel, typename... Args>
	using selection_evaluator_t = typename decltype(std::declval<analysis<T>>().template filter<Sel>(std::declval<std::string>(),std::declval<Args>()...))::action_type;

public:
	friend class analysis<T>;
	template <typename> friend class lazy;

public:
	varied(lazy<Act> const& nom) :
	 node<Act>(*nom.m_analysis),
	 m_nominal(nom)
	{}

	virtual ~varied() = default;

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

	virtual void set_variation(const std::string& var_name, lazy<Act> const& var) override;

	virtual lazy<Act> get_nominal() const override;
	virtual lazy<Act> get_variation(const std::string& var_name) const override;

	virtual bool has_variation(const std::string& var_name) const override;
	virtual std::set<std::string> list_variation_names() const override;

	template <typename... Args, typename V = Act, std::enable_if_t<ana::is_column_v<V> || ana::is_column_evaluator_v<V>, bool> = false>
	auto vary(const std::string& var_name, Args&&... args) -> varied<V>;

	template <typename... Args, typename V = Act, std::enable_if_t<ana::is_column_evaluator_v<V>, bool> = false>
	auto evaluate(Args&&... args) -> varied<evaluated_t<V>>;

	template <typename Sel, typename Lmbd, typename V = Act, std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  auto filter(const std::string& name, Lmbd&& args) -> varied<custom_selection_evaluator_t<Lmbd>>;

	template <typename Sel, typename Lmbd, typename V = Act, std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  auto channel(const std::string& name, Lmbd&& args) -> varied<custom_selection_evaluator_t<Lmbd>>;

	template <typename Sel, typename V = Act, std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  auto filter(const std::string& name) -> varied<simple_selection_evaluator_type>;

	template <typename Sel, typename V = Act, std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  auto channel(const std::string& name) -> varied<simple_selection_evaluator_type>;

	template <typename... Nodes, typename V = Act, std::enable_if_t<ana::is_selection_evaluator_v<V>, bool> = false>
	auto evaluate(Nodes const&... columns) -> varied<selection>;

	template <typename... Nodes, typename V = Act, std::enable_if_t<ana::is_counter_booker_v<V>, bool> = false>
	auto fill(Nodes const&... columns) -> varied<V>;

	template <typename... Nodes, typename V = Act, std::enable_if_t<ana::is_counter_booker_v<V>, bool> = false>
	auto at(Nodes const&... selections) -> varied<typename decltype(std::declval<lazy<V>>().at(selections.get_nominal()...))::action_type>;

	template <typename... Args>
	auto operator()(Args&&... args) -> varied<typename decltype(std::declval<lazy<Act>>().operator()(std::forward<Args>(args).get_nominal()...))::action_type>;

	template <typename V = Act, typename std::enable_if<ana::is_counter_booker_v<V> || ana::is_counter_implemented_v<V>,void>::type* = nullptr>
	auto operator[](const std::string& sel_path) const -> lazy<V>;

	template <typename Node, typename V = Act, std::enable_if_t<ana::is_selection_v<V>,bool> = false>
	auto operator||(const Node& selection) const -> varied<V>;

	template <typename Node, typename V = Act, std::enable_if_t<ana::is_selection_v<V>,bool> = false>
	auto operator&&(const Node& selection) const -> varied<V>;

	DECLARE_VARIED_UNARY_OP(-)
	DECLARE_VARIED_UNARY_OP(!)
	DECLARE_VARIED_BINARY_OP(+)
	DECLARE_VARIED_BINARY_OP(-)
	DECLARE_VARIED_BINARY_OP(*)
	DECLARE_VARIED_BINARY_OP(/)
	DECLARE_VARIED_BINARY_OP(&&)
	DECLARE_VARIED_BINARY_OP(||)
	DECLARE_VARIED_BINARY_OP(<)
	DECLARE_VARIED_BINARY_OP(>)
	DECLARE_VARIED_BINARY_OP(<=)
	DECLARE_VARIED_BINARY_OP(>=)
	DECLARE_VARIED_BINARY_OP(==)
	DECLARE_VARIED_BINARY_OP([])

protected:
	lazy<Act>                                 m_nominal;
	std::unordered_map<std::string,lazy<Act>> m_variation_map;
	std::set<std::string>                     m_variation_names;

};

}

template <typename T>
template <typename Act>
void ana::analysis<T>::varied<Act>::set_variation(const std::string& var_name, lazy<Act> const& var)
{
	m_variation_map.insert(std::make_pair(var_name,var));
	m_variation_names.insert(var_name);
}

template <typename T>
template <typename Act>
auto ana::analysis<T>::varied<Act>::get_nominal() const -> lazy<Act>
{
	return m_nominal;
}

template <typename T>
template <typename Act>
auto ana::analysis<T>::varied<Act>::get_variation(const std::string& var_name) const -> lazy<Act>
{
	return (this->has_variation(var_name) ? m_variation_map.at(var_name) : m_nominal);
}

template <typename T>
template <typename Act>
template <typename V , typename std::enable_if<ana::is_counter_booker_v<V> || ana::is_counter_implemented_v<V>,void>::type* ptr>
auto ana::analysis<T>::varied<Act>::operator[](const std::string& var_name) const -> lazy<V>
{
	if (!this->has_variation(var_name)) {
		throw std::out_of_range("variation does not exist"); 
	}
	return m_variation_map.at(var_name);
}

template <typename T>
template <typename Act>
std::set<std::string> ana::analysis<T>::varied<Act>::list_variation_names() const
{
	return m_variation_names;
}

template <typename T>
template <typename Act>
bool ana::analysis<T>::varied<Act>::has_variation(const std::string& var_name) const
{
	return m_variation_map.find(var_name)!=m_variation_map.end();
}

template <typename T>
template <typename Act>
template<typename... Args, typename V, std::enable_if_t<ana::is_column_evaluator_v<V>,bool>>
auto ana::analysis<T>::varied<Act>::evaluate(Args&&... args) -> typename ana::analysis<T>::template varied<evaluated_t<V>>
{
	varied<evaluated_t<V>> syst(this->get_nominal().evaluate(std::forward<Args>(args).get_nominal()...));
	for (auto const& var_name : list_all_variation_names(*this, std::forward<Args>(args)...)) {
		syst.set_variation(var_name, get_variation(var_name).evaluate(std::forward<Args>(args).get_variation(var_name)...));
	}
	return syst;
}

template <typename T>
template <typename Act>
template <typename Sel, typename Lmbd, typename V, std::enable_if_t<ana::is_selection_v<V>,bool>>
auto ana::analysis<T>::varied<Act>::filter(const std::string& name, Lmbd&& lmbd) -> varied<custom_selection_evaluator_t<Lmbd>>
{
	varied<custom_selection_evaluator_t<Lmbd>> syst(this->get_nominal().template filter<Sel>(name,std::forward<Lmbd>(lmbd)));
	for (auto const& var_name : this->list_variation_names()) {
		syst.set_variation(var_name, get_variation(var_name).template filter<Sel>(name,std::forward<Lmbd>(lmbd)));
	}
	return syst;
}

template <typename T>
template <typename Act>
template <typename Sel, typename Lmbd, typename V, std::enable_if_t<ana::is_selection_v<V>,bool>>
auto ana::analysis<T>::varied<Act>::channel(const std::string& name, Lmbd&& lmbd) -> varied<custom_selection_evaluator_t<Lmbd>>
{
	varied<custom_selection_evaluator_t<Lmbd>> syst(this->get_nominal().template channel<Sel>(name,std::forward<Lmbd>(lmbd)));
	for (auto const& var_name : this->list_variation_names()) {
		syst.set_variation(var_name, get_variation(var_name).template channel<Sel>(name,std::forward<Lmbd>(lmbd)));
	}
	return syst;
}

template <typename T>
template <typename Act>
template <typename Sel, typename V, std::enable_if_t<ana::is_selection_v<V>,bool>>
auto ana::analysis<T>::varied<Act>::filter(const std::string& name) -> varied<simple_selection_evaluator_type>
{
	varied<simple_selection_evaluator_type> syst(this->get_nominal().template filter<Sel>(name));
	for (auto const& var_name : this->list_variation_names()) {
		syst.set_variation(var_name, get_variation(var_name).template filter<Sel>(name));
	}
	return syst;
}

template <typename T>
template <typename Act>
template <typename Sel, typename V, std::enable_if_t<ana::is_selection_v<V>,bool>>
auto ana::analysis<T>::varied<Act>::channel(const std::string& name) -> varied<simple_selection_evaluator_type>
{
	varied<simple_selection_evaluator_type> syst(this->get_nominal().template channel<Sel>(name));
	for (auto const& var_name : this->list_variation_names()) {
		syst.set_variation(var_name, get_variation(var_name).template channel<Sel>(name));
	}
	return syst;
}


template <typename T>
template <typename Act>
template <typename... Nodes, typename V, std::enable_if_t<ana::is_selection_evaluator_v<V>, bool>>
auto ana::analysis<T>::varied<Act>::evaluate(Nodes const&... columns) -> varied<selection>
{
	varied<selection> syst(this->get_nominal().evaluate(columns.get_nominal()...));
	for (auto const& var_name : list_all_variation_names(*this, columns...)) {
		syst.set_variation(var_name,variation(var_name).evaluate(columns.get_variation(var_name)...));
	}
	return syst;
}

template <typename T>
template <typename Act>
template <typename... Nodes, typename V, std::enable_if_t<ana::is_counter_booker_v<V>,bool>>
auto ana::analysis<T>::varied<Act>::fill(Nodes const&... columns) -> varied<V>
// varied version of filling a counter with columns
{
	varied<V> syst(this->get_nominal().fill(columns.get_nominal()...));
	for (auto const& var_name : list_all_variation_names(*this, columns...)) {
		syst.set_variation(var_name, get_variation(var_name).fill(columns.get_variation(var_name)...));
	}
	return syst;
}

template <typename T>
template <typename Act>
template <typename... Nodes, typename V, std::enable_if_t<ana::is_counter_booker_v<V>,bool>>
auto ana::analysis<T>::varied<Act>::at(Nodes const&... selections) -> varied<typename decltype(std::declval<lazy<V>>().at(selections.get_nominal()...))::action_type>
// varied version of booking counter at a selection operation
{
	varied<typename decltype(std::declval<lazy<V>>().at(selections.get_nominal()...))::action_type> syst(this->get_nominal().at(selections.get_nominal()...));
	for (auto const& var_name : list_all_variation_names(*this, selections...)) {
		syst.set_variation(var_name, get_variation(var_name).at(selections.get_variation(var_name)...));
	}
	return syst;
}

template <typename T>
template <typename Act>
template <typename... Args, typename V, std::enable_if_t<ana::is_column_v<V> || ana::is_column_evaluator_v<V>,bool>>
auto ana::analysis<T>::varied<Act>::vary(const std::string& var_name, Args&&... args) -> varied<V>
{
	auto syst = varied<V>(this->get_nominal());
	for (auto const& var_name : this->list_variation_names()) {
		syst.set_variation(var_name, this->get_variation(var_name));
	}
	// set new variation
	syst.set_variation(var_name, this->get_nominal().vary(var_name,std::forward<Args>(args)...).get_variation(var_name));	
	return syst;
}

template <typename T>
template <typename Act>
template <typename... Args>
auto ana::analysis<T>::varied<Act>::operator()(Args&&... args) -> varied<typename decltype(std::declval<lazy<Act>>().operator()(std::forward<Args>(args).get_nominal()...))::action_type>
{
	auto syst = varied<typename decltype(std::declval<lazy<Act>>().operator()(std::forward<Args>(args).get_nominal()...))::action_type>(this->get_nominal().operator()(std::forward<Args>(args).get_nominal()...));
	for (auto const& var_name : list_all_variation_names(*this, std::forward<Args>(args)...)) {
		syst.set_variation(var_name, get_variation(var_name).operator()(std::forward<Args>(args).get_variation(var_name)...) );
	}
	return syst;
}

template <typename T>
template <typename Act>
template <typename Node, typename V, std::enable_if_t<ana::is_selection_v<V>,bool>>
auto ana::analysis<T>::varied<Act>::operator||(const Node& b) const -> varied<V>
{
	auto syst = varied<typename decltype(std::declval<lazy<Act>>().operator||(b.get_nominal()))::action_type>(this->get_nominal().operator||(b.get_nominal()));
	for (auto const& var_name : list_all_variation_names(*this, b)) {
		syst.set_variation(var_name, this->get_variation(var_name).operator||(b.get_variation(var_name)) );
	}
	return syst;	
}

template <typename T>
template <typename Act>
template <typename Node, typename V, std::enable_if_t<ana::is_selection_v<V>,bool>>
auto ana::analysis<T>::varied<Act>::operator&&(const Node& b) const -> varied<V>
{
	auto syst = varied<typename decltype(std::declval<lazy<Act>>().operator&&(b.get_nominal()))::action_type>(this->get_nominal().operator&&(b.get_nominal()));
	for (auto const& var_name : list_all_variation_names(*this, b)) {
		syst.set_variation(var_name, this->get_variation(var_name).operator&&(b.get_variation(var_name)) );
	}
	return syst;	
}

DEFINE_VARIED_UNARY_OP(minus,-)
DEFINE_VARIED_UNARY_OP(logical_not,!)

DEFINE_VARIED_BINARY_OP(+)
DEFINE_VARIED_BINARY_OP(-)
DEFINE_VARIED_BINARY_OP(*)
DEFINE_VARIED_BINARY_OP(/)
DEFINE_VARIED_BINARY_OP(<)
DEFINE_VARIED_BINARY_OP(>)
DEFINE_VARIED_BINARY_OP(<=)
DEFINE_VARIED_BINARY_OP(>=)
DEFINE_VARIED_BINARY_OP(==)
DEFINE_VARIED_BINARY_OP(&&)
DEFINE_VARIED_BINARY_OP(||)
DEFINE_VARIED_BINARY_OP([])
