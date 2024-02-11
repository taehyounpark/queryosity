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
#include "dataset_source.h"
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

  // template <typename> friend class column::definition;
  // template <typename> friend class column::expression;

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
  template <typename DS>
  auto open(dataset::input<DS> &&in) -> dataset::opened<DS>;

  /**
   * @brief Define a constant.
   * @tparam Val Constant data type.
   * @param value Constant data value.
   * @return The `lazy` defined constant.
   */
  template <typename Val>
  auto constant(Val const &val) -> lazy<column::constant<Val>>;

  template <typename Def, typename... Args> auto _define(Args &&...args);
  template <typename Def> auto _define(column::definition<Def> const &defn);

  template <typename F> auto _equate(F fn);
  template <typename Expr> auto _equate(column::expression<Expr> const &expr);

  template <typename Def, typename... Cols>
  auto define(column::definition<Def> const &defn, Cols const &...cols);

  template <typename Expr, typename... Cols>
  auto define(column::expression<Expr> const &expr, Cols const &...cols);

  template <typename Sel, typename F>
  auto _select(F callable)
      -> delayed<selection::template custom_applicator_t<F>>;

  template <typename Sel, typename F>
  auto _select(lazy<selection> const &prev, F callable)
      -> delayed<selection::template custom_applicator_t<F>>;

  template <typename Col> auto filter(lazy<Col> const &col) -> lazy<selection>;
  template <typename Col> auto weight(lazy<Col> const &col) -> lazy<selection>;

  template <typename Col> auto filter(Col const &col);
  template <typename Col> auto weight(Col const &col);

  template <typename Expr, typename... Cols>
  auto filter(ana::column::expression<Expr> const &expr, Cols const &...cols)
      -> lazy<selection>;

  template <typename Expr, typename... Cols>
  auto weight(ana::column::expression<Expr> const &expr, Cols const &...cols)
      -> lazy<selection>;

  template <typename Cnt, typename... Args>
  auto agg(Args &&...args) -> delayed<aggregation::booker<Cnt>>;

  template <typename Val, typename... Vars>
  auto vary(lazy<column::template constant<Val>> const &nom,
            Vars const &...vars);

  template <typename Expr, typename... Vars>
  auto vary(column::expression<Expr> const &expr, Vars const &...vars);

  template <typename Expr, typename... Vars>
  auto vary(column::definition<Expr> const &defn, Vars const &...vars);

protected:
  template <typename Kwd> void accept_kwarg(Kwd const &kwarg);

  void analyze();
  void reset();

  template <typename DS, typename Val>
  auto _read(dataset::source<DS> &ds, const std::string &name)
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
      -> std::array<lazy<Cnt>, sizeof...(Sels)>;

  template <typename Syst, typename Expr>
  void _vary(Syst &syst, const std::string &name,
             column::expression<Expr> const &expr);

  void add_operation(lockstep::node<operation> act);
  void add_operation(std::unique_ptr<operation> act);

protected:
  multithread::core m_mt;
  long long m_nrows;
  dataset::weight m_weight;

  dataset::partition m_partition;
  double m_norm;

  std::unique_ptr<dataset::inifin> m_ds;

  lockstep::node<dataset::range> m_parts;
  lockstep::node<dataset::player> m_players;
  lockstep::node<dataset::processor> m_processors;

  std::vector<std::unique_ptr<operation>> m_operations;
  bool m_analyzed;
};

template <typename T> using operation_t = typename T::nominal_type;

} // namespace ana

#include "dataset_input.h"
#include "dataset_opened.h"
#include "dataset_range.h"

#include "delayed.h"
#include "lazy.h"
#include "lazy_varied.h"

#include "column_expression.h"

#include "systematic_resolver.h"
#include "systematic_variation.h"

inline ana::dataflow::dataflow()
    : m_mt(ana::multithread::disable()), m_nrows(-1), m_weight(1.0),
      m_ds(nullptr), m_analyzed(false) {}

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

template <typename DS>
auto ana::dataflow::open(ana::dataset::input<DS> &&input)
    -> ana::dataset::opened<DS> {

  if (m_ds) {
    std::runtime_error("opening multiple datasets is not yet supported.");
  }

  auto ds = input.ds.get();
  m_ds = std::move(input.ds);

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
auto ana::dataflow::_read(dataset::source<DS> &ds, const std::string &name)
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
        return proc->template assign<Val>(val);
      },
      this->m_processors);
  auto lzy = lazy<column::constant<Val>>(*this, act);
  this->add_operation(std::move(act));
  return lzy;
}

template <typename Def, typename... Cols>
auto ana::dataflow::define(ana::column::definition<Def> const &defn,
                           Cols const &...cols) {
  return this->_define(defn).template evaluate(cols...);
}

template <typename F> auto ana::dataflow::_equate(F callable) {
  return delayed<ana::column::template evaluator_t<F>>(
      *this, lockstep::get_node(
                 [callable](dataset::processor *proc) {
                   return proc->template equate(callable);
                 },
                 this->m_processors));
}

template <typename Expr>
auto ana::dataflow::_equate(ana::column::expression<Expr> const &expr) {
  return expr._equate(*this);
}

template <typename Expr, typename... Cols>
auto ana::dataflow::define(ana::column::expression<Expr> const &expr,
                           Cols const &...cols) {
  return this->_equate(expr).template evaluate(cols...);
}

template <typename Col>
auto ana::dataflow::filter(lazy<Col> const &col) -> lazy<selection> {
  return this
      ->_select<selection::cut>(std::function([](double x) { return x; }))
      .apply(col);
}

template <typename Col>
auto ana::dataflow::weight(lazy<Col> const &col) -> lazy<selection> {
  return this
      ->_select<selection::weight>(std::function([](double x) { return x; }))
      .apply(col);
}

template <typename Col> auto ana::dataflow::filter(Col const &col) {
  auto appl =
      this->_select<selection::cut>(std::function([](double x) { return x; }));

  using varied_type = typename lazy<selection>::varied;

  varied_type syst(appl.template apply(col.nominal()));

  for (auto const &var_name : col.list_variation_names()) {
    syst.set_variation(var_name, appl.template apply(col.variation(var_name)));
  }

  return syst;
}

template <typename Col> auto ana::dataflow::weight(Col const &col) {
  auto appl = this->_select<selection::weight>(
      std::function([](double x) { return x; }));

  using varied_type = typename lazy<selection>::varied;

  varied_type syst(appl.template apply(col.nominal()));

  for (auto const &var_name : col.list_variation_names()) {
    syst.set_variation(var_name, appl.template apply(col.variation(var_name)));
  }

  return syst;
}

template <typename Expr, typename... Cols>
auto ana::dataflow::filter(ana::column::expression<Expr> const &expr,
                           Cols const &...cols) -> lazy<selection> {
  return expr.template _select<selection::cut>(*this).apply(cols...);
}

template <typename Expr, typename... Cols>
auto ana::dataflow::weight(ana::column::expression<Expr> const &expr,
                           Cols const &...cols) -> lazy<selection> {
  return expr.template _select<selection::weight>(*this).apply(cols...);
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
    -> std::array<lazy<Cnt>, sizeof...(Sels)> {

  return std::array<lazy<Cnt>, sizeof...(Sels)>{
      this->select_aggregation(bkr, sels)...};
}

inline void ana::dataflow::analyze() {
  // do not analyze if already done
  if (m_analyzed)
    return;

  // ignore future analyze() requests until reset() is called
  m_analyzed = true;

  m_ds->initialize();

  this->m_mt.run(
      [](dataset::processor *proc, dataset::player *plyr,
         const dataset::range *part) { proc->process(*plyr, *part); },
      this->m_processors, this->m_players, this->m_parts);

  m_ds->finalize();

  // clear aggregations so they are not run more than once
  this->m_processors.call_all_slots(
      [](dataset::processor *proc) { proc->clear_aggregations(); });
}

inline void ana::dataflow::reset() { m_analyzed = false; }

// template <typename Val, typename... Vars>
// auto ana::dataflow::vary(lazy<column::template constant<Val>> const &nom,
//                          Vars const &...vars) {
//   typename lazy<column::template constant<Val>>::varied syst(nom);
//   ((this->vary_constant<Val>(syst, vars.name(), vars.args())), ...);
//   return syst;
// }

// template <typename Val, typename Lazy, typename... Args>
// void ana::dataflow::vary_constant(Lazy &syst, const std::string &name,
//                                   std::tuple<Args...> args) {
//   auto var = std::apply(
//       [this](Args... args) { return this->constant<Val>(args...); }, args);
//   syst.set_variation(name, std::move(var));
// }

template <typename Expr, typename... Vars>
auto ana::dataflow::vary(ana::column::expression<Expr> const &expr,
                         Vars const &...vars) {
  auto nom = this->_equate(expr);
  using varied_type = typename decltype(nom)::varied;
  using function_type = typename column::expression<Expr>::function_type;
  varied_type syst(std::move(nom));
  ((this->_vary(syst, vars.name(),
                column::expression(function_type(std::get<0>(vars.args()))))),
   ...);
  return syst;
}

template <typename Syst, typename Expr>
void ana::dataflow::_vary(Syst &syst, const std::string &name,
                          ana::column::expression<Expr> const &expr) {
  syst.set_variation(name, this->_equate(expr));
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

template <typename Def, typename... Args>
auto ana::dataflow::_define(Args &&...args) {
  return delayed<ana::column::template evaluator_t<Def>>(
      *this,
      lockstep::get_node(
          [&args...](dataset::processor *proc) {
            return proc->template define<Def>(std::forward<Args>(args)...);
          },
          this->m_processors));
}

template <typename Def>
auto ana::dataflow::_define(ana::column::definition<Def> const &defn) {
  return defn._define(*this);
}

template <typename Sel, typename F>
auto ana::dataflow::_select(F callable)
    -> delayed<selection::template custom_applicator_t<F>> {
  return delayed<selection::template custom_applicator_t<F>>(
      *this, lockstep::get_node(
                 [callable](dataset::processor *proc) {
                   return proc->template select<Sel>(nullptr, callable);
                 },
                 this->m_processors));
}

template <typename Sel, typename F>
auto ana::dataflow::_select(lazy<selection> const &prev, F callable)
    -> delayed<selection::template custom_applicator_t<F>> {
  return delayed<selection::template custom_applicator_t<F>>(
      *this, lockstep::get_node(
                 [callable](dataset::processor *proc, selection const *prev) {
                   return proc->template select<Sel>(prev, callable);
                 },
                 this->m_processors, prev));
}