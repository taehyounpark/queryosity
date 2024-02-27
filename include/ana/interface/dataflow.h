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
#include "multithread.h"
#include "selection.h"
#include "systematic.h"

namespace ana {

template <typename T> class lazy;

template <typename U> class todo;

class dataflow {

public:
  template <typename> friend class dataset::opened;

  template <typename> friend class lazy;
  template <typename> friend class todo;

public:
  class node;

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
  auto agg(counter::output<Cntr> const &cntr) -> todo<counter::booker<Cntr>>;

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
  auto _select(F fn) -> todo<selection::template custom_applicator_t<F>>;

  template <typename Sel, typename F>
  auto _select(lazy<selection> const &prev, F fn)
      -> todo<selection::template custom_applicator_t<F>>;

  template <typename Cntr, typename... Args>
  auto _aggregate(Args &&...args) -> todo<counter::booker<Cntr>>;

protected:
  template <typename Kwd> void accept_kwarg(Kwd const &kwarg);

  void analyze();
  void reset();

  template <typename DS, typename Val>
  auto _read(dataset::reader<DS> &ds, const std::string &name)
      -> lazy<read_column_t<DS, Val>>;

  template <typename Def, typename... Cols>
  auto evaluate_column(todo<column::evaluator<Def>> const &calc,
                       lazy<Cols> const &...columns) -> lazy<Def>;

  template <typename Eqn, typename... Cols>
  auto _apply(todo<selection::applicator<Eqn>> const &calc,
              lazy<Cols> const &...columns) -> lazy<selection>;

  template <typename Cntr>
  auto _book(todo<counter::booker<Cntr>> const &bkr, lazy<selection> const &sel)
      -> lazy<Cntr>;
  template <typename Cntr, typename... Sels>
  auto _book(todo<counter::booker<Cntr>> const &bkr, lazy<Sels> const &...sels)
      -> std::array<lazy<Cntr>, sizeof...(Sels)>;

  template <typename Syst, typename Val>
  void _vary(Syst &syst, const std::string &name,
             column::constant<Val> const &cnst);

  template <typename Syst, typename Expr>
  void _vary(Syst &syst, const std::string &name,
             column::expression<Expr> const &expr);

  template <typename Action,
            std::enable_if_t<std::is_base_of_v<action, Action>, bool> = false>
  void add_action(std::vector<std::unique_ptr<Action>> slots);

protected:
  multithread::core m_mt;
  long long m_nrows;
  dataset::weight m_weight;

  std::unique_ptr<dataset::source> m_ds;
  std::vector<unsigned int> m_dslots;
  std::vector<std::unique_ptr<dataset::player>> m_dplyrs_owned;
  std::vector<dataset::player *> m_dplyrs;

  std::vector<std::unique_ptr<action>> m_actions;
  bool m_analyzed;
};

class dataflow::node {

public:
  friend class dataflow;

public:
  node(dataflow &df) : m_df(&df) {}
  virtual ~node() = default;

protected:
  dataflow *m_df;
};

} // namespace ana

#include "dataset_input.h"
#include "dataset_opened.h"
#include "dataset_player.h"

#include "lazy.h"
#include "lazy_varied.h"
#include "todo.h"

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
    m_mt = kwarg;
  } else if (is_weight) {
    m_weight = kwarg;
  } else if (is_nrows) {
    m_nrows = kwarg.nrows;
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
auto ana::dataflow::open(ana::dataset::input<DS> &&in)
    -> ana::dataset::opened<DS> {

  if (m_ds) {
    throw std::runtime_error(
        "opening multiple datasets is not (yet) supported.");
  }
  // !!! must get raw address before moving the unique_ptr
  auto ds_rdr = in.ds.get();
  m_ds = std::move(in.ds);

  auto nslots = m_mt.concurrency();
  m_ds->parallelize(nslots);
  m_dplyrs_owned.resize(nslots);
  m_dplyrs.resize(nslots);
  for (unsigned int islot = 0; islot < nslots; ++islot) {
    m_dplyrs_owned[islot] =
        std::make_unique<dataset::player>(*ds_rdr, m_weight);
    m_dplyrs[islot] = m_dplyrs_owned[islot].get();
  }
  // same for slot numbers
  m_dslots = std::vector<unsigned int>(nslots);
  for (unsigned int i = 0; i < m_dslots.size(); ++i) {
    m_dslots[i] = i;
  }

  return dataset::opened<DS>(*this, *ds_rdr);
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
auto ana::dataflow::_aggregate(Args &&...args) -> todo<counter::booker<Cntr>> {
  return todo<counter::booker<Cntr>>(
      *this, concurrent::invoke(
                 [&args...](dataset::player *plyr) {
                   return plyr->template agg<Cntr>(std::forward<Args>(args)...);
                 },
                 m_dplyrs));
}

template <typename Cntr>
auto ana::dataflow::agg(ana::counter::output<Cntr> const &cntr)
    -> todo<counter::booker<Cntr>> {
  return cntr._aggregate(*this);
}

template <typename Def, typename... Cols>
auto ana::dataflow::evaluate_column(todo<column::evaluator<Def>> const &calc,
                                    lazy<Cols> const &...columns) -> lazy<Def> {
  auto act = concurrent::invoke(
      [](dataset::player *plyr, column::evaluator<Def> *calc,
         Cols const *...cols) {
        return plyr->template evaluate_column(*calc, *cols...);
      },
      m_dplyrs, calc.get_slots(), columns.get_slots()...);
  auto lzy = lazy<Def>(*this, act);
  this->add_action(std::move(act));
  return lzy;
}

template <typename Eqn, typename... Cols>
auto ana::dataflow::_apply(todo<selection::applicator<Eqn>> const &calc,
                           lazy<Cols> const &...columns) -> lazy<selection> {
  auto act = concurrent::invoke(
      [](dataset::player *plyr, selection::applicator<Eqn> *calc,
         Cols *...cols) { return plyr->template _apply(*calc, *cols...); },
      m_dplyrs, calc.get_slots(), columns.get_slots()...);
  auto lzy = lazy<selection>(*this, act);
  this->add_action(std::move(act));
  return lzy;
}

template <typename Cntr>
auto ana::dataflow::_book(todo<counter::booker<Cntr>> const &bkr,
                          lazy<selection> const &sel) -> lazy<Cntr> {
  // new counter booked: dataset will need to be analyzed
  this->reset();
  auto act = concurrent::invoke(
      [](dataset::player *plyr, counter::booker<Cntr> *bkr,
         const selection *sel) { return plyr->book(*bkr, *sel); },
      m_dplyrs, bkr.get_slots(), sel.get_slots());
  auto lzy = lazy<Cntr>(*this, act);
  this->add_action(std::move(act));
  return lzy;
}

template <typename Cntr, typename... Sels>
auto ana::dataflow::_book(todo<counter::booker<Cntr>> const &bkr,
                          lazy<Sels> const &...sels)
    -> std::array<lazy<Cntr>, sizeof...(Sels)> {
  return std::array<lazy<Cntr>, sizeof...(Sels)>{this->_book(bkr, sels)...};
}

inline void ana::dataflow::analyze() {

  // 0. do not analyze if already done
  if (m_analyzed)
    return;

  // 1. partition the dataset
  auto ds_parts = m_ds->partition();
  // truncate entries to limit
  ds_parts = dataset::partition::truncate(ds_parts, m_nrows);
  // merge ds_parts to concurrency limit
  ds_parts = dataset::partition::merge(ds_parts, m_mt.concurrency());

  // dataset players should match partition
  m_dplyrs.resize(ds_parts.size());
  m_dslots.resize(ds_parts.size());

  // 3. event loop is starting
  m_ds->initialize();

  // 4. enter event loop
  m_mt.run(
      [](dataset::player *plyr, unsigned int slot,
         std::pair<unsigned long long, unsigned long long> part) {
        plyr->play(slot, part.first, part.second);
      },
      m_dplyrs, m_dslots, ds_parts);

  // 5. event loop has finished
  m_ds->finalize();

  // done
  m_analyzed = true;
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

template <typename Action,
          std::enable_if_t<std::is_base_of_v<ana::action, Action>, bool>>
void ana::dataflow::add_action(std::vector<std::unique_ptr<Action>> slots) {
  m_actions.reserve(m_actions.size() + slots.size());
  for (unsigned int i = 0; i < slots.size(); ++i) {
    m_actions.push_back(std::move(slots[i]));
  }
}

template <typename DS, typename Val>
auto ana::dataflow::_read(dataset::reader<DS> &ds, const std::string &name)
    -> lazy<read_column_t<DS, Val>> {
  auto act = concurrent::invoke(
      [name, &ds](dataset::player *plyr, unsigned int slot) {
        return plyr->template read<DS, Val>(ds, slot, name);
      },
      m_dplyrs, m_dslots);
  auto lzy = lazy<read_column_t<DS, Val>>(*this, act);
  this->add_action(std::move(act));
  return lzy;
}

template <typename Val>
auto ana::dataflow::_assign(Val const &val) -> lazy<ana::column::fixed<Val>> {
  auto act = concurrent::invoke(
      [&val](dataset::player *plyr) { return plyr->template assign<Val>(val); },
      m_dplyrs);
  auto lzy = lazy<column::fixed<Val>>(*this, act);
  this->add_action(std::move(act));
  return lzy;
}

template <typename Def, typename... Args>
auto ana::dataflow::_define(Args &&...args) {
  return todo<ana::column::template evaluator_t<Def>>(
      *this,
      concurrent::invoke(
          [&args...](dataset::player *plyr) {
            return plyr->template define<Def>(std::forward<Args>(args)...);
          },
          m_dplyrs));
}

template <typename Def>
auto ana::dataflow::_define(ana::column::definition<Def> const &defn) {
  return defn._define(*this);
}

template <typename F> auto ana::dataflow::_equate(F fn) {
  return todo<ana::column::template evaluator_t<F>>(
      *this,
      concurrent::invoke(
          [fn](dataset::player *plyr) { return plyr->template equate(fn); },
          m_dplyrs));
}

template <typename Expr>
auto ana::dataflow::_equate(ana::column::expression<Expr> const &expr) {
  return expr._equate(*this);
}

template <typename Sel, typename F>
auto ana::dataflow::_select(F fn)
    -> todo<selection::template custom_applicator_t<F>> {
  return todo<selection::template custom_applicator_t<F>>(
      *this, concurrent::invoke(
                 [fn](dataset::player *plyr) {
                   return plyr->template select<Sel>(nullptr, fn);
                 },
                 m_dplyrs));
}

template <typename Sel, typename F>
auto ana::dataflow::_select(lazy<selection> const &prev, F fn)
    -> todo<selection::template custom_applicator_t<F>> {
  return todo<selection::template custom_applicator_t<F>>(
      *this, concurrent::invoke(
                 [fn](dataset::player *plyr, selection const *prev) {
                   return plyr->template select<Sel>(prev, fn);
                 },
                 m_dplyrs, prev.get_slots()));
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