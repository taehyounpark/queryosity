#pragma once

#include <set>
#include <string>
#include <type_traits>
#include <vector>

#include "column.h"
#include "concurrent.h"
#include "counter.h"
#include "dataset.h"
#include "processor.h"
#include "sample.h"
#include "selection.h"

namespace ana {

/**
 * @brief Analysis of an input dataset
 */
template <typename T> class dataflow : public sample<T> {

public:
  using dataset_reader_type = typename sample<T>::dataset_reader_type;

public:
  template <typename U> class node;

  template <typename U> class delayed;
  template <typename U> friend class delayed;

  template <typename U> class lazy;
  template <typename U> friend class lazy;

  template <typename U> class varied;
  template <typename U> friend class varied;

  template <typename U>
  static constexpr std::true_type
  check_lazy(typename dataflow<T>::template lazy<U> const &);
  static constexpr std::false_type check_lazy(...);
  template <typename U>
  static constexpr std::true_type
  check_delayed(typename dataflow<T>::template delayed<U> const &);
  static constexpr std::false_type check_delayed(...);
  template <typename V>
  static constexpr bool is_nominal_v =
      decltype(check_lazy(std::declval<V>()))::value;

  template <typename U>
  static constexpr std::true_type
  check_varied(typename dataflow<T>::template varied<U> const &);
  static constexpr std::false_type check_varied(...);
  template <typename V>
  static constexpr bool is_varied_v =
      decltype(check_varied(std::declval<V>()))::value;

  template <typename... Args>
  static constexpr bool has_no_variation_v = (is_nominal_v<Args> && ...);
  template <typename... Args>
  static constexpr bool has_variation_v = (is_varied_v<Args> || ...);

public:
  virtual ~dataflow() = default;

  /**
   * @brief Constructor using arguments for input dataset.
   * @param arguments Constructor arguments for the input dataset.
   */
  template <typename... Args> dataflow(Args &&...args);
  // shortcuts for file paths provided with initializer braces
  template <typename U = T, typename = std::enable_if_t<std::is_constructible_v<
                                U, std::string, std::vector<std::string>>>>

  dataflow(const std::string &key, const std::vector<std::string> &file_paths);
  // shortcuts for file paths provided with initializer braces
  template <typename U = T, typename = std::enable_if_t<std::is_constructible_v<
                                U, std::vector<std::string>, std::string>>>
  dataflow(const std::vector<std::string> &file_paths, const std::string &key);

  dataflow(dataflow const &) = delete;
  dataflow &operator=(dataflow const &) = delete;

  dataflow(dataflow &&) = default;
  dataflow &operator=(dataflow &&) = default;

  /**
   * @brief Read a column from the dataset.
   * @tparam Val Column data type.
   * @param name Column name.
   * @return The `lazy` read column.
   */
  template <typename Val>
  auto read(const std::string &name)
      -> lazy<read_column_t<read_dataset_t<T>, Val>>;

  /**
   * @brief Define a constant.
   * @tparam Val Constant data type.
   * @param value Constant data value.
   * @return The `lazy` defined constant.
   */
  template <typename Val>
  auto constant(const Val &value) -> lazy<column::constant<Val>>;

  /**
   * @brief Define a custom definition or representation.
   * @tparam Def The full definition/representation user-implementation.
   * @param args Constructor arguments for the definition/representation.
   * @return The `lazy` definition "evaluator" to be evaluated with input
   * columns.
   */
  template <typename Def, typename... Args>
  auto define(Args &&...args) -> delayed<column::template evaluator_t<Def>>;

  /**
   * @brief Define an equation.
   * @tparam F Any function/functor/callable type.
   * @param callable The function/functor/callable object used as the
   * expression.
   * @return The `lazy` equation "evaluator" to be evaluated with input columns.
   */
  template <typename F>
  auto define(F callable) -> delayed<column::template evaluator_t<F>>;

  /**
   * @brief Apply a filter.
   * @tparam Sel Type of selection, i.e. `selection::cut` or
   * `selection::weight`.
   * @tparam F Any function/functor/callable type.
   * @param name The name of the selection.
   * @param callable The function/functor/callable object used as the
   * expression.
   * @return The `lazy` selection "applicator" to be applied with input columns.
   * @details Perform a filter operation from the dataflow to define one without
   * a preselection.
   */
  template <typename Sel, typename F>
  auto filter(const std::string &name, F callable)
      -> delayed<selection::template custom_applicator_t<F>>;

  /**
   * @brief Apply a filter as a channel.
   * @tparam Sel Type of selection, i.e. `selection::cut` or
   * `selection::weight`.
   * @tparam F Any function/functor/callable type.
   * @param name The name of the selection.
   * @param callable The function/functor/callable object used as the
   * expression.
   * @return The `lazy` selection "applicator" to be applied with input columns.
   * @details Perform a filter operation from the dataflow to define one without
   * a preselection.
   */
  template <typename Sel, typename F>
  auto channel(const std::string &name, F callable)
      -> delayed<selection::template custom_applicator_t<F>>;
  template <typename Sel>

  /**
   * @brief Apply a selection.
   * @tparam Sel Type of selection, i.e. `selection::cut` or
   * `selection::weight`.
   * @param name The name of the selection.
   * @return The `lazy` selection "applicator" to be applied with the input
   * column.
   * @details When a filter operation is called without a custom expression, the
   * value of the input column itself is used as its decision.
   */
  auto filter(const std::string &name)
      -> delayed<selection::trivial_applicator_type>;
  template <typename Sel>

  /**
   * @brief Apply a selection.
   * @tparam Sel Type of selection, i.e. `selection::cut` or
   * `selection::weight`.
   * @param name The name of the selection.
   * @return The `lazy` selection "applicator" to be applied with the input
   * column.
   * @details When a filter operation is called without a custom expression, the
   * value of the input column itself is used as its decision.
   */
  auto channel(const std::string &name)
      -> delayed<selection::trivial_applicator_type>;

  /**
   * @brief Book a counter
   * @tparam Cnt Any full user-implementation of `counter`.
   * @param args Constructor arguments for the **Cnt**.
   * @return The `lazy` counter "booker" to be filled with input column(s) and
   * booked at selection(s).
   */
  template <typename Cnt, typename... Args>
  auto book(Args &&...args) -> delayed<counter::booker<Cnt>>;

protected:
  /**
   * @brief Default constructor for initial flags and values.
   */
  dataflow();
  void analyze();
  void reset();

  template <typename Def, typename... Cols>
  auto evaluate_column(delayed<column::evaluator<Def>> const &calc,
                       lazy<Cols> const &...columns) -> lazy<Def>;
  template <typename Eqn, typename... Cols>
  auto apply_selection(delayed<selection::applicator<Eqn>> const &calc,
                       lazy<Cols> const &...columns) -> lazy<selection>;
  template <typename Cnt>
  auto book_selection(delayed<counter::booker<Cnt>> const &bkr,
                      lazy<selection> const &sel) -> lazy<Cnt>;
  template <typename Cnt, typename... Sels>
  auto book_selections(delayed<counter::booker<Cnt>> const &bkr,
                       lazy<Sels> const &...sels)
      -> delayed<counter::booker<Cnt>>;

  template <typename Sel, typename F>
  auto filter(lazy<selection> const &prev, const std::string &name, F callable)
      -> delayed<selection::template custom_applicator_t<F>>;
  template <typename Sel, typename F>
  auto channel(lazy<selection> const &prev, const std::string &name, F callable)
      -> delayed<selection::template custom_applicator_t<F>>;
  template <typename Sel>
  auto filter(lazy<selection> const &prev, const std::string &name)
      -> delayed<selection::trivial_applicator_type>;
  template <typename Sel>
  auto channel(lazy<selection> const &prev, const std::string &name)
      -> delayed<selection::trivial_applicator_type>;
  template <typename Sel>
  auto join(lazy<selection> const &a, lazy<selection> const &b)
      -> lazy<selection>;

  // recreate a lazy node as a variation under new arguments
  template <typename V,
            typename std::enable_if_t<ana::column::template is_reader_v<V>, V>
                * = nullptr>
  auto vary_column(lazy<V> const &nom, const std::string &colname) -> lazy<V>;
  template <typename Val, typename V,
            typename std::enable_if_t<ana::column::template is_constant_v<V>, V>
                * = nullptr>
  auto vary_column(lazy<V> const &nom, Val const &val) -> lazy<V>;
  template <
      typename... Args, typename V,
      typename std::enable_if_t<ana::column::template is_definition_v<V> &&
                                    !ana::column::template is_equation_v<V>,
                                V> * = nullptr>

  auto vary_definition(delayed<column::evaluator<V>> const &nom, Args &&...args)
      -> delayed<column::evaluator<V>>;
  template <typename F, typename V,
            typename std::enable_if_t<ana::column::template is_equation_v<V>, V>
                * = nullptr>
  auto vary_equation(delayed<column::evaluator<V>> const &nom, F callable)
      -> delayed<column::evaluator<V>>;

  void add_action(concurrent<action> const &act);

protected:
  bool m_analyzed;
  std::vector<concurrent<action>> m_actions;
};

template <typename T> template <typename U> class dataflow<T>::node {

public:
  using dataflow_type = dataflow<T>;
  using dataset_type = T;
  using nominal_type = U;

public:
  friend class dataflow<T>;
  template <typename> friend class node;

public:
  node(dataflow<T> &dataflow);

  virtual ~node() = default;

public:
  virtual void set_variation(const std::string &var_name, U const &nom) = 0;

  virtual U get_nominal() const = 0;
  virtual U get_variation(const std::string &var_name) const = 0;

  virtual bool has_variation(const std::string &var_name) const = 0;
  virtual std::set<std::string> list_variation_names() const = 0;

protected:
  dataflow<T> *m_df;
};

template <typename T> using dataflow_t = typename T::dataflow_type;
template <typename T> using action_t = typename T::action_type;

template <typename... Nodes>
auto list_all_variation_names(Nodes const &...nodes) -> std::set<std::string>;

} // namespace ana

#include "delayed.h"
#include "lazy.h"
#include "varied.h"

// ----------------------------------------------------------------------------
// node
// ----------------------------------------------------------------------------

template <typename T>
template <typename U>
ana::dataflow<T>::node<U>::node(dataflow<T> &df) : m_df(&df) {}

// ----------------------------------------------------------------------------
// dataflow
// ----------------------------------------------------------------------------

template <typename T> ana::dataflow<T>::dataflow() : m_analyzed(false) {}

template <typename T>
template <typename... Args>
ana::dataflow<T>::dataflow(Args &&...args) : dataflow<T>::dataflow() {
  this->prepare(std::forward<Args>(args)...);
}

template <typename T>
template <typename U, typename>
ana::dataflow<T>::dataflow(const std::string &key,
                           const std::vector<std::string> &file_paths)
    : dataflow<T>::dataflow() {
  this->prepare(key, file_paths);
}

template <typename T>
template <typename U, typename>
ana::dataflow<T>::dataflow(const std::vector<std::string> &file_paths,
                           const std::string &key)
    : dataflow<T>::dataflow() {
  this->prepare(file_paths, key);
}

template <typename T>
template <typename Val>
// typename ana::dataflow<T>::template lazy<ana::term<Val>>
// ana::dataflow<T>::read(const std::string& name)
auto ana::dataflow<T>::read(const std::string &name)
    -> lazy<read_column_t<read_dataset_t<T>, Val>> {
  this->initialize();
  auto act = this->m_processors.get_concurrent_result(
      [name = name](processor<dataset_reader_type> &proc) {
        return proc.template read<Val>(name);
      });
  this->add_action(act);
  return lazy<read_column_t<read_dataset_t<T>, Val>>(*this, act);
}

template <typename T>
template <typename Val>
auto ana::dataflow<T>::constant(const Val &val)
    -> lazy<ana::column::constant<Val>> {
  auto act = this->m_processors.get_concurrent_result(
      [val = val](processor<dataset_reader_type> &proc) {
        return proc.template constant<Val>(val);
      });
  this->add_action(act);
  auto nd = lazy<column::constant<Val>>(*this, act);
  return nd;
}

template <typename T>
template <typename Def, typename... Args>
auto ana::dataflow<T>::define(Args &&...args)
    -> delayed<ana::column::template evaluator_t<Def>> {
  return delayed<ana::column::template evaluator_t<Def>>(
      *this,
      this->m_processors.get_concurrent_result(
          [&args...](processor<dataset_reader_type> &proc) {
            return proc.template define<Def>(std::forward<Args>(args)...);
          }));
}

template <typename T>
template <typename F>
auto ana::dataflow<T>::define(F callable)
    -> delayed<column::template evaluator_t<F>> {
  return delayed<ana::column::template evaluator_t<F>>(
      *this, this->m_processors.get_concurrent_result(
                 [callable = callable](processor<dataset_reader_type> &proc) {
                   return proc.template define(callable);
                 }));
}

template <typename T>
template <typename Def, typename... Cols>
auto ana::dataflow<T>::evaluate_column(
    delayed<column::evaluator<Def>> const &calc, lazy<Cols> const &...columns)
    -> lazy<Def> {
  auto act = this->m_processors.get_concurrent_result(
      [](processor<dataset_reader_type> &proc, column::evaluator<Def> &calc,
         Cols const &...cols) {
        return proc.template evaluate_column(calc, cols...);
      },
      lockstep<column::evaluator<Def>>(calc), columns...);
  this->add_action(act);
  return lazy<Def>(*this, act);
}

template <typename T>
template <typename Sel, typename F>
auto ana::dataflow<T>::filter(const std::string &name, F callable)
    -> delayed<selection::template custom_applicator_t<F>> {
  return delayed<selection::template custom_applicator_t<F>>(
      *this, this->m_processors.get_concurrent_result(
                 [name = name,
                  callable = callable](processor<dataset_reader_type> &proc) {
                   return proc.template filter<Sel>(name, callable);
                 }));
}

template <typename T>
template <typename Sel, typename F>
auto ana::dataflow<T>::channel(const std::string &name, F callable)
    -> delayed<selection::template custom_applicator_t<F>> {
  return delayed<selection::template custom_applicator_t<F>>(
      *this, this->m_processors.get_concurrent_result(
                 [name = name,
                  callable = callable](processor<dataset_reader_type> &proc) {
                   return proc.template channel<Sel>(name, callable);
                 }));
}

template <typename T>
template <typename Sel>
auto ana::dataflow<T>::filter(const std::string &name)
    -> delayed<selection::trivial_applicator_type> {
  auto callable = [](double x) { return x; };
  auto sel = delayed<selection::trivial_applicator_type>(
      *this, this->m_processors.get_concurrent_result(
                 [name = name,
                  callable = callable](processor<dataset_reader_type> &proc) {
                   return proc.template filter<Sel>(name, callable);
                 }));
  return sel;
}

template <typename T>
template <typename Sel>
auto ana::dataflow<T>::channel(const std::string &name)
    -> delayed<selection::trivial_applicator_type> {
  auto callable = [](double x) { return x; };
  auto sel = delayed<selection::trivial_applicator_type>(
      *this, this->m_processors.get_concurrent_result(
                 [name = name,
                  callable = callable](processor<dataset_reader_type> &proc) {
                   return proc.template channel<Sel>(name, callable);
                 }));
  return sel;
}

template <typename T>
template <typename Sel, typename F>
auto ana::dataflow<T>::filter(lazy<selection> const &prev,
                              const std::string &name, F callable)
    -> delayed<selection::template custom_applicator_t<F>> {
  return delayed<selection::template custom_applicator_t<F>>(
      *this,
      this->m_processors.get_concurrent_result(
          [name = name, callable = callable](
              processor<dataset_reader_type> &proc, selection const &prev) {
            return proc.template filter<Sel>(prev, name, callable);
          },
          prev));
}

template <typename T>
template <typename Sel, typename F>
auto ana::dataflow<T>::channel(lazy<selection> const &prev,
                               const std::string &name, F callable)
    -> delayed<selection::template custom_applicator_t<F>> {
  return delayed<selection::template custom_applicator_t<F>>(
      *this,
      this->m_processors.get_concurrent_result(
          [name = name, callable = callable](
              processor<dataset_reader_type> &proc, selection const &prev) {
            return proc.template channel<Sel>(prev, name, callable);
          },
          prev));
}

template <typename T>
template <typename Sel>
auto ana::dataflow<T>::filter(lazy<selection> const &prev,
                              const std::string &name)
    -> delayed<selection::trivial_applicator_type> {
  auto callable = [](double x) { return x; };
  return delayed<selection::trivial_applicator_type>(
      *this,
      this->m_processors.get_concurrent_result(
          [name = name, callable = callable](
              processor<dataset_reader_type> &proc, selection const &prev) {
            return proc.template filter<Sel>(prev, name, callable);
          },
          prev));
}

template <typename T>
template <typename Sel>
auto ana::dataflow<T>::channel(lazy<selection> const &prev,
                               const std::string &name)
    -> delayed<selection::trivial_applicator_type> {
  auto callable = [](double x) { return x; };
  return delayed<selection::trivial_applicator_type>(
      *this,
      this->m_processors.get_concurrent_result(
          [name = name, callable = callable](
              processor<dataset_reader_type> &proc, selection const &prev) {
            return proc.template channel<Sel>(prev, name, callable);
          },
          prev));
}

template <typename T>
template <typename Eqn, typename... Cols>
auto ana::dataflow<T>::apply_selection(
    delayed<selection::applicator<Eqn>> const &calc,
    lazy<Cols> const &...columns) -> lazy<selection> {
  auto act = this->m_processors.get_concurrent_result(
      [](processor<dataset_reader_type> &proc, selection::applicator<Eqn> &calc,
         Cols &...cols) {
        return proc.template apply_selection(calc, cols...);
      },
      lockstep<selection::applicator<Eqn>>(calc), columns...);
  this->add_action(act);
  return lazy<selection>(*this, act);
}

template <typename T>
template <typename Sel>
auto ana::dataflow<T>::join(lazy<selection> const &a, lazy<selection> const &b)
    -> lazy<selection> {
  auto act = this->m_processors.get_concurrent_result(
      [](processor<dataset_reader_type> &proc, selection const &a,
         selection const &b) { return proc.template join<Sel>(a, b); },
      a, b);
  this->add_action(act);
  return lazy<selection>(*this, act);
}

template <typename T>
template <typename Cnt, typename... Args>
auto ana::dataflow<T>::book(Args &&...args) -> delayed<counter::booker<Cnt>> {
  return delayed<counter::booker<Cnt>>(
      *this, this->m_processors.get_concurrent_result(
                 [&args...](processor<dataset_reader_type> &proc) {
                   return proc.template book<Cnt>(std::forward<Args>(args)...);
                 }));
}

template <typename T>
template <typename Cnt>
auto ana::dataflow<T>::book_selection(delayed<counter::booker<Cnt>> const &bkr,
                                      lazy<selection> const &sel) -> lazy<Cnt> {
  // any time a new counter is booked, means the dataflow must run: so reset its
  // status
  this->reset();
  auto act = this->m_processors.get_concurrent_result(
      [](processor<dataset_reader_type> &proc, counter::booker<Cnt> &bkr,
         const selection &sel) { return proc.book_selection(bkr, sel); },
      lockstep<counter::booker<Cnt>>(bkr), sel);
  this->add_action(act);
  return lazy<Cnt>(*this, act);
}

template <typename T>
template <typename Cnt, typename... Sels>
auto ana::dataflow<T>::book_selections(delayed<counter::booker<Cnt>> const &bkr,
                                       lazy<Sels> const &...sels)
    -> delayed<counter::booker<Cnt>> {
  // any time a new counter is booked, means the dataflow must run: so reset its
  // status
  this->reset();
  auto bkr2 = lazy<counter::booker<Cnt>>(
      *this, this->m_processors.get_concurrent_result(
                 [](processor<dataset_reader_type> &proc,
                    counter::booker<Cnt> &bkr, Sels const &...sels) {
                   return proc.book_selections(bkr, sels...);
                 },
                 bkr, sels...));
  // add all counters that were booked
  for (auto const &sel_path : bkr2.list_selection_paths()) {
    this->add_action(bkr2.get_counter(sel_path));
  }
  return bkr2;
}

template <typename T> void ana::dataflow<T>::analyze() {
  // do not analyze if already done
  if (m_analyzed)
    return;

  this->m_dataset->start_dataset();

  // multithreaded (if enabled)
  this->m_processors.run_slots(
      [](processor<dataset_reader_type> &proc) { proc.process(); });

  this->m_dataset->finish_dataset();

  // clear counters in counter::experiment
  // if they are present, they will be repeated in future runs
  this->m_processors.call_all(
      [](processor<dataset_reader_type> &proc) { proc.clear_counters(); });

  // ignore future analyze() requests,
  // until reset() is called
  m_analyzed = true;
}

template <typename T> void ana::dataflow<T>::reset() { m_analyzed = false; }

template <typename T>
void ana::dataflow<T>::add_action(concurrent<action> const &action) {
  m_actions.emplace_back(action);
}

template <typename... Nodes>
auto ana::list_all_variation_names(Nodes const &...nodes)
    -> std::set<std::string> {
  std::set<std::string> variation_names;
  (variation_names.merge(nodes.list_variation_names()), ...);
  return variation_names;
}

template <typename T>
template <typename V, typename std::enable_if_t<
                          ana::column::template is_reader_v<V>, V> *ptr>
auto ana::dataflow<T>::vary_column(lazy<V> const &, const std::string &colname)
    -> lazy<V> {
  return this->read<cell_value_t<std::decay_t<V>>>(colname);
}

template <typename T>
template <
    typename Val, typename V,
    typename std::enable_if_t<ana::column::template is_constant_v<V>, V> *ptr>
auto ana::dataflow<T>::vary_column(lazy<V> const &nom, Val const &val)
    -> lazy<V> {
  return this->constant<Val>(val);
}

template <typename T>
template <typename... Args, typename V,
          typename std::enable_if_t<ana::column::template is_definition_v<V> &&
                                        !ana::column::template is_equation_v<V>,
                                    V> *ptr>
auto ana::dataflow<T>::vary_definition(delayed<column::evaluator<V>> const &,
                                       Args &&...args)
    -> delayed<column::evaluator<V>> {
  return this->define<V>(std::forward<Args>(args)...);
}

template <typename T>
template <
    typename F, typename V,
    typename std::enable_if_t<ana::column::template is_equation_v<V>, V> *ptr>
auto ana::dataflow<T>::vary_equation(delayed<column::evaluator<V>> const &,
                                     F callable)
    -> delayed<column::evaluator<V>> {
  return this->define(typename V::function_type(callable));
}