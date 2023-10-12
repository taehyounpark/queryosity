#pragma once

/** @file */

#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

#include "multithread.h"
#include "sample.h"

#include "aggregation.h"
#include "column.h"
#include "selection.h"

namespace ana {

class dataflow {

public:
  template <typename DS> class reader;

  template <typename U> class systematic;

  template <typename U> class delayed;

  template <typename U> class lazy;

public:
  template <typename U>
  static constexpr std::true_type check_lazy(lazy<U> const &);
  static constexpr std::false_type check_lazy(...);
  template <typename U>
  static constexpr std::true_type check_delayed(delayed<U> const &);
  static constexpr std::false_type check_delayed(...);

  template <typename V>
  static constexpr bool is_nominal_v =
      (decltype(check_lazy(std::declval<V>()))::value ||
       decltype(check_delayed(std::declval<V>()))::value);
  template <typename V> static constexpr bool is_varied_v = !is_nominal_v<V>;

  template <typename... Args>
  static constexpr bool has_no_variation_v = (is_nominal_v<Args> && ...);
  template <typename... Args>
  static constexpr bool has_variation_v = (is_varied_v<Args> || ...);

public:
  dataflow();
  ~dataflow() = default;

  template <typename KWArg> dataflow(KWArg kwarg);
  template <typename KWArg1, typename KWArg2>
  dataflow(KWArg1 kwarg1, KWArg2 kwarg2);
  template <typename KWArg1, typename KWArg2, typename KWArg3>
  dataflow(KWArg1 kwarg1, KWArg2 kwarg2, KWArg3 kwarg3);

  dataflow(dataflow const &) = delete;
  dataflow &operator=(dataflow const &) = delete;

  dataflow(dataflow &&) = default;
  dataflow &operator=(dataflow &&) = default;

  template <typename DS, typename... Args> reader<DS> open(Args &&...args);

  /**
   * @brief Read a column from the dataset.
   * @tparam Val Column data type.
   * @param name Column name.
   * @return The `lazy` read column.
   */
  template <typename DS, typename Val>
  auto read(dataset::input<DS> &ds, const std::string &name)
      -> lazy<read_column_t<DS, Val>>;

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

  auto filter(const std::string &name);
  auto weight(const std::string &name);

  template <typename F> auto filter(const std::string &name, F callable);
  template <typename F>
  auto weight(const std::string &name, F callable)
      -> delayed<selection::template custom_applicator_t<F>>;

  auto channel(const std::string &name);
  template <typename F> auto channel(const std::string &name, F callable);

  template <typename Sel, typename F>
  auto select(const std::string &name, F callable)
      -> delayed<selection::template custom_applicator_t<F>>;
  template <typename Sel, typename F>
  auto channel(const std::string &name, F callable)
      -> delayed<selection::template custom_applicator_t<F>>;

  /**
   * @brief Book a aggregation
   * @tparam Cnt Any full user-implementation of `aggregation`.
   * @param args Constructor arguments for the **Cnt**.
   * @return The `lazy` aggregation "booker" to be filled with input column(s)
   * and booked at selection(s).
   */
  template <typename Cnt, typename... Args>
  auto book(Args &&...args) -> delayed<aggregation::booker<Cnt>>;

protected:
  template <typename KWArg> void accept_kwarg(KWArg kwarg);

  void analyze();
  void reset();

  template <typename Def, typename... Cols>
  auto evaluate_column(delayed<column::evaluator<Def>> const &calc,
                       lazy<Cols> const &...columns) -> lazy<Def>;
  template <typename Eqn, typename... Cols>
  auto apply_selection(delayed<selection::applicator<Eqn>> const &calc,
                       lazy<Cols> const &...columns) -> lazy<selection>;
  template <typename Cnt>
  auto select_aggregation(delayed<aggregation::booker<Cnt>> const &bkr,
                          lazy<selection> const &sel) -> lazy<Cnt>;
  template <typename Cnt, typename... Sels>
  auto select_aggregations(delayed<aggregation::booker<Cnt>> const &bkr,
                           lazy<Sels> const &...sels)
      -> delayed<aggregation::bookkeeper<Cnt>>;

  template <typename Sel>
  auto select(lazy<selection> const &prev, const std::string &name);
  template <typename Sel, typename F>
  auto select(lazy<selection> const &prev, const std::string &name, F callable)
      -> delayed<selection::template custom_applicator_t<F>>;

  template <typename Sel, typename F>
  auto channel(lazy<selection> const &prev, const std::string &name, F callable)
      -> delayed<selection::template custom_applicator_t<F>>;
  template <typename Sel>
  auto channel(lazy<selection> const &prev, const std::string &name);

  // recreate a lazy node as a variation under new arguments
  template <typename V,
            std::enable_if_t<column::template is_reader_v<V>, bool> = false>
  auto vary_column(lazy<V> const &nom, const std::string &colname) -> lazy<V>;

  template <typename Val, typename V,
            std::enable_if_t<column::template is_constant_v<V>, bool> = false>
  auto vary_column(lazy<V> const &nom, Val const &val) -> lazy<V>;

  template <typename... Args, typename V,
            std::enable_if_t<column::template is_definition_v<V> &&
                                 !column::template is_equation_v<V>,
                             bool> = false>
  auto vary_evaluator(delayed<column::evaluator<V>> const &nom, Args &&...args)
      -> delayed<column::evaluator<V>>;

  template <typename F, typename V,
            std::enable_if_t<column::template is_equation_v<V>, bool> = false>
  auto vary_evaluator(delayed<column::evaluator<V>> const &nom, F callable)
      -> delayed<column::evaluator<V>>;

  void add_operation(lockstep::node<operation> act);
  void add_operation(std::unique_ptr<operation> act);

protected:
  multithread::configuration m_mtcfg;
  long long m_nrows;
  double m_weight;

  dataset::partition m_partition;
  double m_norm;

  lockstep::node<dataset::range> m_parts;
  lockstep::node<dataset::row> m_rows;
  lockstep::node<dataset::processor> m_processors;

  std::vector<std::unique_ptr<operation>> m_operations;
  bool m_analyzed;
};

template <typename T> using operation_t = typename T::nominal_type;

} // namespace ana

#include "dataflow_delayed.h"
#include "dataflow_lazy.h"
#include "dataflow_reader.h"

inline ana::dataflow::dataflow()
    : m_mtcfg(ana::multithread::disable()), m_nrows(-1), m_weight(1.0),
      m_analyzed(false) {}

template <typename KWArg> ana::dataflow::dataflow(KWArg kwarg) : dataflow() {
  this->accept_kwarg<KWArg>(kwarg);
}

template <typename KWArg1, typename KWArg2>
ana::dataflow::dataflow(KWArg1 kwarg1, KWArg2 kwarg2) : dataflow() {
  static_assert(!std::is_same_v<KWArg1, KWArg2>, "repeated keyword arguments.");
  this->accept_kwarg<KWArg1>(kwarg1);
  this->accept_kwarg<KWArg2>(kwarg2);
}

template <typename KWArg> void ana::dataflow::accept_kwarg(KWArg kwarg) {
  constexpr bool is_mt = std::is_same_v<KWArg, ana::multithread::configuration>;
  constexpr bool is_weight = std::is_same_v<KWArg, ana::sample::weight>;
  constexpr bool is_nrows = std::is_same_v<KWArg, ana::dataset::head>;
  if constexpr (is_mt) {
    this->m_mtcfg = kwarg;
  } else if (is_weight) {
    this->m_weight = kwarg.value;
  } else if (is_nrows) {
    this->m_nrows = kwarg.value;
  } else {
    static_assert(is_mt || is_weight || is_nrows,
                  "unrecognized keyword argument");
  }
}

template <typename KWArg1, typename KWArg2, typename KWArg3>
ana::dataflow::dataflow(KWArg1 kwarg1, KWArg2 kwarg2, KWArg3 kwarg3)
    : dataflow() {
  static_assert(!std::is_same_v<KWArg1, KWArg2>, "repeated keyword arguments.");
  static_assert(!std::is_same_v<KWArg1, KWArg3>, "repeated keyword arguments.");
  static_assert(!std::is_same_v<KWArg2, KWArg3>, "repeated keyword arguments.");
  this->accept_kwarg<KWArg1>(kwarg1);
  this->accept_kwarg<KWArg2>(kwarg2);
  this->accept_kwarg<KWArg3>(kwarg3);
}

template <typename DS, typename... Args>
ana::dataflow::reader<DS> ana::dataflow::open(Args &&...args) {
  auto ds = std::make_unique<DS>(std::forward<Args>(args)...);

  // 1. allocate the dataset partition
  this->m_partition = ds->allocate();
  // 2. truncate entries to limit
  this->m_partition.truncate(this->m_nrows);
  // 3. merge parts to concurrency limit
  this->m_partition.merge(this->m_mtcfg.concurrency);
  // 4. normalize scale
  this->m_norm = ds->normalize();

  // put partition into slots
  this->m_parts.clear_slots();
  // model reprents whole dataset
  this->m_parts.set_model(
      std::make_unique<dataset::range>(this->m_partition.total()));
  // each slot takes a part
  for (unsigned int ipart = 0; ipart < m_partition.size(); ++ipart) {
    this->m_parts.add_slot(
        std::make_unique<dataset::range>(this->m_partition.get_part(ipart)));
  }

  // open dataset reader and processor for each thread
  // slot for each partition range
  this->m_rows = lockstep::invoke_node(
      [&ds](dataset::range &part) { return ds->open_rows(part); },
      this->m_parts.get_view());
  this->m_processors = lockstep::node<dataset::processor>(
      m_parts.concurrency(), this->m_weight / this->m_norm);

  return reader<DS>(*this, std::move(ds));
}

template <typename DS, typename Val>
auto ana::dataflow::read(dataset::input<DS> &ds, const std::string &name)
    -> lazy<read_column_t<DS, Val>> {
  auto act = this->m_processors.get_lockstep_node(
      [name, &ds](dataset::processor &proc, dataset::range &part) {
        return proc.template read<DS, Val>(std::ref(ds), part, name);
      },
      this->m_parts.get_view());
  auto lzy = lazy<read_column_t<DS, Val>>(*this, act);
  this->add_operation(std::move(act));
  return lzy;
}

template <typename Val>
auto ana::dataflow::constant(const Val &val)
    -> lazy<ana::column::constant<Val>> {
  auto act = this->m_processors.get_lockstep_node(
      [val = val](dataset::processor &proc) {
        return proc.template constant<Val>(val);
      });
  auto lzy = lazy<column::constant<Val>>(*this, act);
  this->add_operation(std::move(act));
  return lzy;
}

template <typename Def, typename... Args>
auto ana::dataflow::define(Args &&...args)
    -> delayed<ana::column::template evaluator_t<Def>> {
  return delayed<ana::column::template evaluator_t<Def>>(
      *this,
      this->m_processors.get_lockstep_node(
          [&args...](dataset::processor &proc) {
            return proc.template define<Def>(std::forward<Args>(args)...);
          }));
}

template <typename F>
auto ana::dataflow::define(F callable)
    -> delayed<column::template evaluator_t<F>> {
  return delayed<ana::column::template evaluator_t<F>>(
      *this, this->m_processors.get_lockstep_node(
                 [callable = callable](dataset::processor &proc) {
                   return proc.template define(callable);
                 }));
}

inline auto ana::dataflow::filter(const std::string &name) {
  auto callable = [](double x) { return x; };
  return this->template select<selection::cut, decltype(callable)>(name,
                                                                   callable);
}

inline auto ana::dataflow::weight(const std::string &name) {
  auto callable = [](double x) { return x; };
  return this->template select<selection::weight, decltype(callable)>(name,
                                                                      callable);
}

inline auto ana::dataflow::channel(const std::string &name) {
  auto callable = [](double x) { return x; };
  return this->template channel<selection::cut, decltype(callable)>(name,
                                                                    callable);
}

template <typename F>
auto ana::dataflow::filter(const std::string &name, F callable) {
  return this->template select<selection::cut, F>(name, callable);
}

template <typename F>
auto ana::dataflow::weight(const std::string &name, F callable)
    -> delayed<selection::template custom_applicator_t<F>> {
  return this->template select<selection::weight, F>(name, callable);
}

template <typename F>
auto ana::dataflow::channel(const std::string &name, F callable) {
  return this->template channel<selection::cut, F>(name, callable);
}

template <typename Sel, typename F>
auto ana::dataflow::select(const std::string &name, F callable)
    -> delayed<selection::template custom_applicator_t<F>> {
  return delayed<selection::template custom_applicator_t<F>>(
      *this, this->m_processors.get_lockstep_node(
                 [name = name, callable = callable](dataset::processor &proc) {
                   return proc.template select<Sel>(nullptr, name, callable);
                 }));
}

template <typename Sel, typename F>
auto ana::dataflow::channel(const std::string &name, F callable)
    -> delayed<selection::template custom_applicator_t<F>> {
  return delayed<selection::template custom_applicator_t<F>>(
      *this, this->m_processors.get_lockstep_node(
                 [name = name, callable = callable](dataset::processor &proc) {
                   return proc.template channel<Sel>(nullptr, name, callable);
                 }));
}

template <typename Cnt, typename... Args>
auto ana::dataflow::book(Args &&...args) -> delayed<aggregation::booker<Cnt>> {
  return delayed<aggregation::booker<Cnt>>(
      *this, this->m_processors.get_lockstep_node(
                 [&args...](dataset::processor &proc) {
                   return proc.template book<Cnt>(std::forward<Args>(args)...);
                 }));
}

template <typename Sel>
auto ana::dataflow::select(lazy<selection> const &prev,
                           const std::string &name) {
  auto callable = [](double x) { return x; };
  return this->template select<Sel, decltype(callable)>(prev, name, callable);
}

template <typename Sel>
auto ana::dataflow::channel(lazy<selection> const &prev,
                            const std::string &name) {
  auto callable = [](double x) { return x; };
  return this->template channel<Sel, decltype(callable)>(prev, name, callable);
}

template <typename Sel, typename F>
auto ana::dataflow::select(lazy<selection> const &prev, const std::string &name,
                           F callable)
    -> delayed<selection::template custom_applicator_t<F>> {
  return delayed<selection::template custom_applicator_t<F>>(
      *this, this->m_processors.get_lockstep_node(
                 [name = name, callable = callable](dataset::processor &proc,
                                                    selection const &prev) {
                   return proc.template select<Sel>(&prev, name, callable);
                 },
                 prev));
}

template <typename Sel, typename F>
auto ana::dataflow::channel(lazy<selection> const &prev,
                            const std::string &name, F callable)
    -> delayed<selection::template custom_applicator_t<F>> {
  return delayed<selection::template custom_applicator_t<F>>(
      *this, this->m_processors.get_lockstep_node(
                 [name = name, callable = callable](dataset::processor &proc,
                                                    selection const &prev) {
                   return proc.template channel<Sel>(&prev, name, callable);
                 },
                 prev));
}

template <typename Def, typename... Cols>
auto ana::dataflow::evaluate_column(delayed<column::evaluator<Def>> const &calc,
                                    lazy<Cols> const &...columns) -> lazy<Def> {
  auto act = this->m_processors.get_lockstep_node(
      [](dataset::processor &proc, column::evaluator<Def> &calc,
         Cols const &...cols) {
        return proc.template evaluate_column(calc, cols...);
      },
      calc.get_view(), columns...);
  auto lzy = lazy<Def>(*this, act);
  this->add_operation(std::move(act));
  return lzy;
}

template <typename Eqn, typename... Cols>
auto ana::dataflow::apply_selection(
    delayed<selection::applicator<Eqn>> const &calc,
    lazy<Cols> const &...columns) -> lazy<selection> {
  auto act = this->m_processors.get_lockstep_node(
      [](dataset::processor &proc, selection::applicator<Eqn> &calc,
         Cols &...cols) {
        return proc.template apply_selection(calc, cols...);
      },
      calc.get_view(), columns...);
  auto lzy = lazy<selection>(*this, act);
  this->add_operation(std::move(act));
  return lzy;
}

template <typename Cnt>
auto ana::dataflow::select_aggregation(
    delayed<aggregation::booker<Cnt>> const &bkr, lazy<selection> const &sel)
    -> lazy<Cnt> {
  // any time a new aggregation is booked, means the dataflow must run: so reset
  // its status
  this->reset();
  auto act = this->m_processors.get_lockstep_node(
      [](dataset::processor &proc, aggregation::booker<Cnt> &bkr,
         const selection &sel) { return proc.select_aggregation(bkr, sel); },
      bkr.get_view(), sel);
  auto lzy = lazy<Cnt>(*this, act);
  this->add_operation(std::move(act));
  return lzy;
}

template <typename Cnt, typename... Sels>
auto ana::dataflow::select_aggregations(
    delayed<aggregation::booker<Cnt>> const &bkr, lazy<Sels> const &...sels)
    -> delayed<aggregation::bookkeeper<Cnt>> {
  // any time a new aggregation is booked, means the dataflow must run: so reset
  // its status
  this->reset();

  using delayed_bookkeeper_type = delayed<aggregation::bookkeeper<Cnt>>;
  auto bkpr = delayed_bookkeeper_type(
      *this, this->m_processors.get_lockstep_node(
                 [this](dataset::processor &proc, aggregation::booker<Cnt> &bkr,
                        Sels const &...sels) {
                   // get bookkeeper and aggregations
                   auto bkpr_and_cntrs = proc.select_aggregations(bkr, sels...);

                   // add each aggregation to this dataflow
                   for (auto &&cntr : bkpr_and_cntrs.second) {
                     this->add_operation(std::move(cntr));
                   }

                   // take the bookkeeper
                   return std::move(bkpr_and_cntrs.first);
                 },
                 bkr.get_view(), sels...));
  //  lockstep::node<aggregation::booker<Cnt>>(bkr), sels...));

  return bkpr;
}

inline void ana::dataflow::analyze() {
  // do not analyze if already done
  if (m_analyzed)
    return;

  this->m_processors.run_slots(
      this->m_mtcfg,
      [](dataset::processor &proc, dataset::row &rdr,
         const dataset::range &part) { proc.process(rdr, part); },
      this->m_rows.get_view(), this->m_parts.get_view());

  // clear aggregations so they are not run more than once
  this->m_processors.call_all_slots(
      [](dataset::processor &proc) { proc.clear_aggregations(); });

  // ignore future analyze() requests until reset() is called
  m_analyzed = true;
}

inline void ana::dataflow::reset() { m_analyzed = false; }

template <typename V,
          std::enable_if_t<ana::column::template is_reader_v<V>, bool>>
auto ana::dataflow::vary_column(lazy<V> const &, const std::string &colname)
    -> lazy<V> {
  return this->read<cell_value_t<std::decay_t<V>>>(colname);
}

template <typename Val, typename V,
          std::enable_if_t<ana::column::template is_constant_v<V>, bool>>
auto ana::dataflow::vary_column(lazy<V> const &, Val const &val) -> lazy<V> {
  return this->constant<Val>(val);
}

template <typename... Args, typename V,
          std::enable_if_t<ana::column::template is_definition_v<V> &&
                               !ana::column::template is_equation_v<V>,
                           bool>>
auto ana::dataflow::vary_evaluator(delayed<column::evaluator<V>> const &,
                                   Args &&...args)
    -> delayed<column::evaluator<V>> {
  return this->define<V>(std::forward<Args>(args)...);
}

template <typename F, typename V,
          std::enable_if_t<ana::column::template is_equation_v<V>, bool>>
auto ana::dataflow::vary_evaluator(delayed<column::evaluator<V>> const &,
                                   F callable)
    -> delayed<column::evaluator<V>> {
  return this->define(typename V::function_type(callable));
}

inline void ana::dataflow::add_operation(lockstep::node<operation> operation) {
  m_operations.emplace_back(std::move(operation.m_model));
  for (unsigned int i = 0; i < operation.concurrency(); ++i) {
    m_operations.emplace_back(std::move(operation.m_slots[i]));
  }
}

inline void ana::dataflow::add_operation(std::unique_ptr<operation> operation) {
  m_operations.emplace_back(std::move(operation));
}