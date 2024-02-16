#pragma once

/** @file */

#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "column.h"
#include "counter.h"
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

  template <typename Val>
  auto define(column::constant<Val> const &cnst) -> lazy<column::fixed<Val>>;

  template <typename Def, typename... Cols>
  auto define(column::definition<Def> const &defn, Cols const &...cols)
      -> lazy<Def>;

  template <typename Expr, typename... Cols>
  auto define(column::expression<Expr> const &expr, Cols const &...cols)
      -> lazy<column::equation_t<ana::column::expression<Expr>>>;

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

  template <typename Cntr>
  auto agg(counter::output<Cntr> const &cntr) -> delayed<counter::booker<Cntr>>;

  template <typename Val, typename... Vars>
  auto vary(column::constant<Val> const &nom, Vars const &...vars);

  template <typename Expr, typename... Vars>
  auto vary(column::expression<Expr> const &expr, Vars const &...vars);

  template <typename Expr, typename... Vars>
  auto vary(column::definition<Expr> const &defn, Vars const &...vars);

  /* public, but not really... */

  template <typename Val>
  auto _assign(Val const &val) -> lazy<column::fixed<Val>>;

  template <typename Def, typename... Args> auto _define(Args &&...args);
  template <typename Def> auto _define(column::definition<Def> const &defn);

  template <typename F> auto _equate(F fn);
  template <typename Expr> auto _equate(column::expression<Expr> const &expr);

  template <typename Sel, typename F>
  auto _select(F fn) -> delayed<selection::template custom_applicator_t<F>>;

  template <typename Sel, typename F>
  auto _select(lazy<selection> const &prev, F fn)
      -> delayed<selection::template custom_applicator_t<F>>;

  template <typename Cntr, typename... Args>
  auto _aggregate(Args &&...args) -> delayed<counter::booker<Cntr>>;

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
  auto _apply(delayed<selection::applicator<Eqn>> const &calc,
              lazy<Cols> const &...columns) -> lazy<selection>;

  template <typename Cntr>
  auto _book(delayed<counter::booker<Cntr>> const &bkr,
             lazy<selection> const &sel) -> lazy<Cntr>;
  template <typename Cntr, typename... Sels>
  auto _book(delayed<counter::booker<Cntr>> const &bkr,
             lazy<Sels> const &...sels)
      -> std::array<lazy<Cntr>, sizeof...(Sels)>;

  template <typename Syst, typename Val>
  void _vary(Syst &syst, const std::string &name,
             column::constant<Val> const &cnst);

  template <typename Syst, typename Expr>
  void _vary(Syst &syst, const std::string &name,
             column::expression<Expr> const &expr);

  void add_operation(lockstep::node<operation> act);

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

#include "column_constant.h"
#include "column_expression.h"
#include "counter_output.h"

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

template <typename Val>
auto ana::dataflow::define(ana::column::constant<Val> const &cnst)
    -> lazy<column::fixed<Val>> {
  return cnst._assign(*this);
}

template <typename Def, typename... Cols>
auto ana::dataflow::define(ana::column::definition<Def> const &defn,
                           Cols const &...cols) -> lazy<Def> {
  return this->_define(defn).template evaluate(cols...);
}

template <typename Expr, typename... Cols>
auto ana::dataflow::define(ana::column::expression<Expr> const &expr,
                           Cols const &...cols)
    -> lazy<column::equation_t<ana::column::expression<Expr>>> {
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

template <typename Cntr, typename... Args>
auto ana::dataflow::_aggregate(Args &&...args)
    -> delayed<counter::booker<Cntr>> {
  return delayed<counter::booker<Cntr>>(
      *this, lockstep::get_node(
                 [&args...](dataset::processor *proc) {
                   return proc->template agg<Cntr>(std::forward<Args>(args)...);
                 },
                 this->m_processors));
}

template <typename Cntr>
auto ana::dataflow::agg(ana::counter::output<Cntr> const &cntr)
    -> delayed<counter::booker<Cntr>> {
  return cntr._aggregate(*this);
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
auto ana::dataflow::_apply(delayed<selection::applicator<Eqn>> const &calc,
                           lazy<Cols> const &...columns) -> lazy<selection> {
  auto act = lockstep::get_node(
      [](dataset::processor *proc, selection::applicator<Eqn> *calc,
         Cols *...cols) { return proc->template _apply(*calc, *cols...); },
      this->m_processors, calc, columns...);
  auto lzy = lazy<selection>(*this, act);
  this->add_operation(std::move(act));
  return lzy;
}

template <typename Cntr>
auto ana::dataflow::_book(delayed<counter::booker<Cntr>> const &bkr,
                          lazy<selection> const &sel) -> lazy<Cntr> {
  // any time a new counter is booked, means the dataflow must run: so reset
  // its status
  this->reset();
  auto act = lockstep::get_node(
      [](dataset::processor *proc, counter::booker<Cntr> *bkr,
         const selection *sel) { return proc->book(*bkr, *sel); },
      this->m_processors, bkr, sel);
  auto lzy = lazy<Cntr>(*this, act);
  this->add_operation(std::move(act));
  return lzy;
}

template <typename Cntr, typename... Sels>
auto ana::dataflow::_book(delayed<counter::booker<Cntr>> const &bkr,
                          lazy<Sels> const &...sels)
    -> std::array<lazy<Cntr>, sizeof...(Sels)> {

  return std::array<lazy<Cntr>, sizeof...(Sels)>{this->_book(bkr, sels)...};
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

  // clear counters so they are not run more than once
  this->m_processors.call_all_slots(
      [](dataset::processor *proc) { proc->clear_counters(); });
}

inline void ana::dataflow::reset() { m_analyzed = false; }

template <typename Val, typename... Vars>
auto ana::dataflow::vary(ana::column::constant<Val> const &cnst,
                         Vars const &...vars) {
  auto nom = this->define(cnst);
  using varied_type = typename decltype(nom)::varied;
  varied_type syst(nom);
  ((this->_vary(syst, vars.name(),
                column::constant<Val>(std::get<0>(vars.args())))),
   ...);
  return syst;
}

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

inline void ana::dataflow::add_operation(lockstep::node<operation> operation) {
  m_operations.emplace_back(std::move(operation.m_model));
  for (unsigned int i = 0; i < operation.concurrency(); ++i) {
    m_operations.emplace_back(std::move(operation.m_slots[i]));
  }
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
auto ana::dataflow::_assign(Val const &val) -> lazy<ana::column::fixed<Val>> {
  auto act = lockstep::get_node(
      [&val](dataset::processor *proc) {
        return proc->template assign<Val>(val);
      },
      this->m_processors);
  auto lzy = lazy<column::fixed<Val>>(*this, act);
  this->add_operation(std::move(act));
  return lzy;
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

template <typename F> auto ana::dataflow::_equate(F fn) {
  return delayed<ana::column::template evaluator_t<F>>(
      *this,
      lockstep::get_node(
          [fn](dataset::processor *proc) { return proc->template equate(fn); },
          this->m_processors));
}

template <typename Expr>
auto ana::dataflow::_equate(ana::column::expression<Expr> const &expr) {
  return expr._equate(*this);
}

template <typename Sel, typename F>
auto ana::dataflow::_select(F fn)
    -> delayed<selection::template custom_applicator_t<F>> {
  return delayed<selection::template custom_applicator_t<F>>(
      *this, lockstep::get_node(
                 [fn](dataset::processor *proc) {
                   return proc->template select<Sel>(nullptr, fn);
                 },
                 this->m_processors));
}

template <typename Sel, typename F>
auto ana::dataflow::_select(lazy<selection> const &prev, F fn)
    -> delayed<selection::template custom_applicator_t<F>> {
  return delayed<selection::template custom_applicator_t<F>>(
      *this, lockstep::get_node(
                 [fn](dataset::processor *proc, selection const *prev) {
                   return proc->template select<Sel>(prev, fn);
                 },
                 this->m_processors, prev));
}

template <typename Syst, typename Val>
void ana::dataflow::_vary(Syst &syst, const std::string &name,
                          ana::column::constant<Val> const &cnst) {
  syst.set_variation(name, this->define(cnst));
}

template <typename Syst, typename Expr>
void ana::dataflow::_vary(Syst &syst, const std::string &name,
                          ana::column::expression<Expr> const &expr) {
  syst.set_variation(name, this->_equate(expr));
}