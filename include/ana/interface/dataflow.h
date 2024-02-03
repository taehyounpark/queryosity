#pragma once

/** @file */

#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "aggregation.h"
#include "column.h"
#include "dataset.h"
#include "dataset_partition.h"
#include "dataset_player.h"
#include "dataset_processor.h"
#include "multithread.h"
#include "selection.h"
#include "systematic.h"

namespace ana {

template <typename T> class lazy;

template <typename U> class delayed;

class dataflow {

public:
  template <typename> friend class dataset::opened;
  template <typename> friend class lazy;
  template <typename> friend class delayed;

public:
  /**
   * @brief Default constructor.
   */
  dataflow();
  ~dataflow() = default;

  /**
   * @brief Constructor with one keyword argument.
   * @tparam Kwd Keyword argument type.
   * @details A keyword argument can be:
   *  - @p ana::dataset::weight(float)
   *  - @p ana::dataset::limit(unsigned int)
   *  - @p ana::multithread::enable(unsigned int)
   */
  template <typename Kwd> dataflow(Kwd kwarg);
  template <typename Kwd1, typename Kwd2> dataflow(Kwd1 kwarg1, Kwd2 kwarg2);
  template <typename Kwd1, typename Kwd2, typename Kwd3>
  dataflow(Kwd1 kwarg1, Kwd2 kwarg2, Kwd3 kwarg3);

  dataflow(dataflow const &) = delete;
  dataflow &operator=(dataflow const &) = delete;

  dataflow(dataflow &&) = default;
  dataflow &operator=(dataflow &&) = default;

  /**
   * @brief Open a dataset input.
   * @tparam DS Dataset input.
   * @tparam Args... Dataset input constructor arguments
   * @return Opened dataset input
   */
  template <typename DS, typename... Args>
  auto open(Args &&...args) -> dataset::opened<DS>;

  /**
   * @brief Define a constant.
   * @tparam Val Constant data type.
   * @param value Constant data value.
   * @return The `lazy` defined constant.
   */
  template <typename Val>
  auto constant(Val const &val) -> lazy<column::constant<Val>>;
  template <typename Val, typename... Args>
  auto constant(Args &&...args) -> lazy<column::constant<Val>>;

  /**
   * @brief Define a custom definition or representation.
   * @tparam Def The full definition/representation user-implementation.
   * @param args Constructor arguments for the definition/representation.
   * @return The `lazy` definition "evaluator" to be evaluated with input
   * columns.
   */
  template <typename Def, typename... Args> auto define(Args &&...args);

  /**
   * @brief Define an equation.
   * @tparam F Any function/functor/callable type.
   * @param callable The function/functor/callable object used as the
   * expression.
   * @return The `lazy` equation "evaluator" to be evaluated with input columns.
   */
  template <typename F> auto define(F const &callable);

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

  template <typename Cnt, typename... Args>
  auto agg(Args &&...args) -> delayed<aggregation::booker<Cnt>>;

  template <typename Val, typename... Vars>
  auto vary(lazy<column::template constant<Val>> const &nom,
            Vars const &...vars);

  template <typename Col, typename... Vars>
  auto vary(delayed<column::template evaluator<Col>> &&nom,
            Vars const &...vars);

protected:
  template <typename Kwd> void accept_kwarg(Kwd const &kwarg);

  void analyze();
  void reset();

  template <typename DS, typename Val>
  auto read(dataset::input<DS> &ds, const std::string &name)
      -> lazy<read_column_t<DS, Val>>;

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

  template <typename Col, typename Eval, typename... Args>
  void vary_evaluator(Eval &syst, const std::string &name,
                      std::tuple<Args...> args);

  template <typename Val, typename Lazy, typename... Args>
  void vary_constant(Lazy &syst, const std::string &name,
                     std::tuple<Args...> args);

  void add_operation(lockstep::node<operation> act);
  void add_operation(std::unique_ptr<operation> act);

protected:
  multithread::core m_mt;
  long long m_nrows;
  dataset::weight m_weight;

  dataset::partition m_partition;
  double m_norm;

  std::unique_ptr<dataset::source> m_source;

  lockstep::node<dataset::range> m_parts;
  lockstep::node<dataset::player> m_players;
  lockstep::node<dataset::processor> m_processors;

  std::vector<std::unique_ptr<operation>> m_operations;
  bool m_analyzed;
};

template <typename T> using operation_t = typename T::nominal_type;

} // namespace ana

#include "delayed.h"
#include "lazy.h"
#include "lazy_varied.h"

#include "dataset_opened.h"
#include "dataset_range.h"

#include "systematic_resolver.h"
#include "systematic_variation.h"

inline ana::dataflow::dataflow()
    : m_mt(ana::multithread::disable()), m_nrows(-1), m_weight(1.0),
      m_source(nullptr), m_analyzed(false) {}

template <typename Kwd> ana::dataflow::dataflow(Kwd kwarg) : dataflow() {
  this->accept_kwarg<Kwd>(kwarg);
}

template <typename Kwd1, typename Kwd2>
ana::dataflow::dataflow(Kwd1 kwarg1, Kwd2 kwarg2) : dataflow() {
  static_assert(!std::is_same_v<Kwd1, Kwd2>, "repeated keyword arguments.");
  this->accept_kwarg<Kwd1>(kwarg1);
  this->accept_kwarg<Kwd2>(kwarg2);
}

template <typename Kwd> void ana::dataflow::accept_kwarg(Kwd const &kwarg) {
  constexpr bool is_mt = std::is_same_v<Kwd, multithread::core>;
  constexpr bool is_weight = std::is_same_v<Kwd, dataset::weight>;
  constexpr bool is_nrows = std::is_same_v<Kwd, dataset::limit>;
  if constexpr (is_mt) {
    this->m_mt = kwarg;
  } else if (is_weight) {
    this->m_weight = kwarg;
  } else if (is_nrows) {
    this->m_nrows = kwarg.nrows;
  } else {
    static_assert(is_mt || is_weight || is_nrows,
                  "unrecognized keyword argument");
  }
}

template <typename Kwd1, typename Kwd2, typename Kwd3>
ana::dataflow::dataflow(Kwd1 kwarg1, Kwd2 kwarg2, Kwd3 kwarg3) : dataflow() {
  static_assert(!std::is_same_v<Kwd1, Kwd2>, "repeated keyword arguments.");
  static_assert(!std::is_same_v<Kwd1, Kwd3>, "repeated keyword arguments.");
  static_assert(!std::is_same_v<Kwd2, Kwd3>, "repeated keyword arguments.");
  this->accept_kwarg<Kwd1>(kwarg1);
  this->accept_kwarg<Kwd2>(kwarg2);
  this->accept_kwarg<Kwd3>(kwarg3);
}

template <typename DS, typename... Args>
auto ana::dataflow::open(Args &&...args) -> ana::dataset::opened<DS> {

  if (m_source) {
    std::runtime_error("opening multiple datasets is not yet supported.");
  }

  auto source = std::make_unique<DS>(std::forward<Args>(args)...);
  auto ds = source.get();
  m_source = std::move(source);

  // 1. parallelize the dataset partition
  this->m_partition = ds->parallelize();
  // 2. truncate entries to limit
  this->m_partition.truncate(this->m_nrows);
  // 3. merge parts to concurrency limit
  this->m_partition.merge(this->m_mt.concurrency);
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
        std::make_unique<dataset::range>(this->m_partition[ipart]));
  }

  // open dataset reader and processor for each thread
  // slot for each partition range
  this->m_players = lockstep::get_node(
      [ds](dataset::range *part) { return ds->open_player(*part); },
      this->m_parts);
  this->m_processors = lockstep::node<dataset::processor>(
      m_parts.concurrency(), this->m_weight / this->m_norm);

  return dataset::opened<DS>(*this, *ds);
}

template <typename DS, typename Val>
auto ana::dataflow::read(dataset::input<DS> &ds, const std::string &name)
    -> lazy<read_column_t<DS, Val>> {
  auto act = lockstep::get_node(
      [name, &ds](dataset::processor *proc, dataset::range *part) {
        return proc->template read<DS, Val>(std::ref(ds), *part, name);
      },
      this->m_processors, this->m_parts);
  auto lzy = lazy<read_column_t<DS, Val>>(*this, act);
  this->add_operation(std::move(act));
  return lzy;
}

template <typename Val>
auto ana::dataflow::constant(Val const &val)
    -> lazy<ana::column::constant<Val>> {
  auto act = lockstep::get_node(
      [&val](dataset::processor *proc) {
        return proc->template constant<Val>(val);
      },
      this->m_processors);
  auto lzy = lazy<column::constant<Val>>(*this, act);
  this->add_operation(std::move(act));
  return lzy;
}

template <typename Val, typename... Args>
auto ana::dataflow::constant(Args &&...args)
    -> lazy<ana::column::constant<Val>> {
  auto act = lockstep::get_node(
      [args...](dataset::processor *proc) {
        return proc->template constant<Val>(std::forward<Args>(args)...);
      },
      this->m_processors);
  auto lzy = lazy<column::constant<Val>>(*this, act);
  this->add_operation(std::move(act));
  return lzy;
}

template <typename Def, typename... Args>
auto ana::dataflow::define(Args &&...args) {
  return delayed<ana::column::template evaluator_t<Def>>(
      *this,
      lockstep::get_node(
          [&args...](dataset::processor *proc) {
            return proc->template define<Def>(std::forward<Args>(args)...);
          },
          this->m_processors));
}

template <typename F> auto ana::dataflow::define(F const &callable) {
  return delayed<ana::column::template evaluator_t<F>>(
      *this, lockstep::get_node(
                 [callable](dataset::processor *proc) {
                   return proc->template define(callable);
                 },
                 this->m_processors));
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
      *this, lockstep::get_node(
                 [name, callable](dataset::processor *proc) {
                   return proc->template select<Sel>(nullptr, name, callable);
                 },
                 this->m_processors));
}

template <typename Sel, typename F>
auto ana::dataflow::channel(const std::string &name, F callable)
    -> delayed<selection::template custom_applicator_t<F>> {
  return delayed<selection::template custom_applicator_t<F>>(
      *this, lockstep::get_node(
                 [name, callable](dataset::processor *proc) {
                   return proc->template channel<Sel>(nullptr, name, callable);
                 },
                 this->m_processors));
}

template <typename Cnt, typename... Args>
auto ana::dataflow::agg(Args &&...args) -> delayed<aggregation::booker<Cnt>> {
  return delayed<aggregation::booker<Cnt>>(
      *this, lockstep::get_node(
                 [&args...](dataset::processor *proc) {
                   return proc->template agg<Cnt>(std::forward<Args>(args)...);
                 },
                 this->m_processors));
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
      *this,
      lockstep::get_node(
          [name, callable](dataset::processor *proc, selection const *prev) {
            return proc->template select<Sel>(prev, name, callable);
          },
          this->m_processors, prev));
}

template <typename Sel, typename F>
auto ana::dataflow::channel(lazy<selection> const &prev,
                            const std::string &name, F callable)
    -> delayed<selection::template custom_applicator_t<F>> {
  return delayed<selection::template custom_applicator_t<F>>(
      *this,
      lockstep::get_node(
          [name, callable](dataset::processor *proc, selection const *prev) {
            return proc->template channel<Sel>(prev, name, callable);
          },
          this->m_processors, prev));
}

template <typename Def, typename... Cols>
auto ana::dataflow::evaluate_column(delayed<column::evaluator<Def>> const &calc,
                                    lazy<Cols> const &...columns) -> lazy<Def> {
  auto act = lockstep::get_node(
      [](dataset::processor *proc, column::evaluator<Def> *calc,
         Cols const *...cols) {
        return proc->template evaluate_column(*calc, *cols...);
      },
      this->m_processors, calc, columns...);
  auto lzy = lazy<Def>(*this, act);
  this->add_operation(std::move(act));
  return lzy;
}

template <typename Eqn, typename... Cols>
auto ana::dataflow::apply_selection(
    delayed<selection::applicator<Eqn>> const &calc,
    lazy<Cols> const &...columns) -> lazy<selection> {
  auto act = lockstep::get_node(
      [](dataset::processor *proc, selection::applicator<Eqn> *calc,
         Cols *...cols) {
        return proc->template apply_selection(*calc, *cols...);
      },
      this->m_processors, calc, columns...);
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
  auto act = lockstep::get_node(
      [](dataset::processor *proc, aggregation::booker<Cnt> *bkr,
         const selection *sel) { return proc->select_aggregation(*bkr, *sel); },
      this->m_processors, bkr, sel);
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
      *this, lockstep::get_node(
                 [this](dataset::processor *proc, aggregation::booker<Cnt> *bkr,
                        Sels const *...sels) {
                   // get bookkeeper and aggregations
                   auto bkpr_and_cntrs =
                       proc->select_aggregations(*bkr, *sels...);

                   // add each aggregation to this dataflow
                   for (auto &&cntr : bkpr_and_cntrs.second) {
                     this->add_operation(std::move(cntr));
                   }

                   // take the bookkeeper
                   return std::move(bkpr_and_cntrs.first);
                 },
                 this->m_processors, bkr, sels...));
  return bkpr;
}

inline void ana::dataflow::analyze() {
  // do not analyze if already done
  if (m_analyzed)
    return;

  // ignore future analyze() requests until reset() is called
  m_analyzed = true;

  m_source->initialize();

  this->m_mt.run(
      [](dataset::processor *proc, dataset::player *plyr,
         const dataset::range *part) { proc->process(*plyr, *part); },
      this->m_processors, this->m_players, this->m_parts);

  m_source->finalize();

  // clear aggregations so they are not run more than once
  this->m_processors.call_all_slots(
      [](dataset::processor *proc) { proc->clear_aggregations(); });
}

inline void ana::dataflow::reset() { m_analyzed = false; }

template <typename Val, typename... Vars>
auto ana::dataflow::vary(lazy<column::template constant<Val>> const &nom,
                         Vars const &...vars) {
  typename lazy<column::template constant<Val>>::varied syst(nom);
  ((this->vary_constant<Val>(syst, vars.name(), vars.args())), ...);
  return syst;
}

template <typename Val, typename Lazy, typename... Args>
void ana::dataflow::vary_constant(Lazy &syst, const std::string &name,
                                  std::tuple<Args...> args) {
  auto var = std::apply(
      [this](Args... args) { return this->constant<Val>(args...); }, args);
  syst.set_variation(name, std::move(var));
}

template <typename Col, typename... Vars>
auto ana::dataflow::vary(delayed<column::template evaluator<Col>> &&nom,
                         Vars const &...vars) {
  typename delayed<column::template evaluator<Col>>::varied syst(
      std::move(nom));
  ((this->vary_evaluator<Col>(syst, vars.name(), vars.args())), ...);
  return syst;
}

template <typename Col, typename Eval, typename... Args>
void ana::dataflow::vary_evaluator(Eval &syst, const std::string &name,
                                   std::tuple<Args...> args) {
  auto var = std::apply(
      [this](Args... args) { return this->define<Col>(args...); }, args);
  syst.set_variation(name, std::move(var));
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