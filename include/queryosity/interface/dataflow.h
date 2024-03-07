#pragma once

/** @file */

#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "column.h"
#include "dataset.h"
#include "dataset_partition.h"
#include "dataset_player.h"
#include "multithread.h"
#include "query.h"
#include "selection.h"
#include "systematic.h"

namespace queryosity {

template <typename T> class lazy;

template <typename U> class todo;

/**
 * Main data analysis and query graph interface.
 */
class dataflow {

public:
  template <typename> friend class dataset::loaded;

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

  template <typename Kwd> dataflow(Kwd kwarg);
  template <typename Kwd1, typename Kwd2> dataflow(Kwd1 kwarg1, Kwd2 kwarg2);

  /**
   * Constructor with (up to) three keyword arguments, which can be one of the
   * following:
   *
   *  - `queryosity::multithread::enable(unsigned int)`
   *  - `queryosity::dataset::head(unsigned int)`
   *  - `queryosity::dataset::weight(float)`
   *
   */
  template <typename Kwd1, typename Kwd2, typename Kwd3>
  dataflow(Kwd1 kwarg1, Kwd2 kwarg2, Kwd3 kwarg3);

  dataflow(dataflow const &) = delete;
  dataflow &operator=(dataflow const &) = delete;

  dataflow(dataflow &&) = default;
  dataflow &operator=(dataflow &&) = default;

  /**
   * Open a dataset input.
   * @tparam DS Dataset input.
   * @tparam Args... Dataset input constructor arguments
   * @return Opened dataset input
   */
  template <typename DS>
  auto load(dataset::input<DS> &&in) -> dataset::loaded<DS>;

  template <typename DS, typename Val>
  auto read(dataset::input<DS> in, dataset::column<Val> const &col);

  template <typename DS, typename... Vals>
  auto read(dataset::input<DS> in, dataset::columns<Vals...> const &cols);

  template <typename Val>
  auto define(column::constant<Val> const &cnst) -> lazy<column::fixed<Val>>;

  template <typename Def, typename... Cols>
  auto define(column::definition<Def> const &defn, lazy<Cols> const &...cols)
      -> lazy<Def>;

  template <typename Expr, typename... Cols>
  auto define(column::expression<Expr> const &expr, lazy<Cols> const &...cols)
      -> lazy<column::equation_t<queryosity::column::expression<Expr>>>;

  template <typename Col>
  auto filter(lazy<Col> const &col) -> lazy<selection::node>;
  template <typename Col>
  auto weight(lazy<Col> const &col) -> lazy<selection::node>;

  template <typename Col> auto filter(Col const &col);
  template <typename Col> auto weight(Col const &col);

  template <typename Expr, typename... Cols>
  auto filter(queryosity::column::expression<Expr> const &expr,
              Cols const &...cols);

  template <typename Expr, typename... Cols>
  auto weight(queryosity::column::expression<Expr> const &expr,
              Cols const &...cols);

  template <typename Cntr>
  auto make(query::plan<Cntr> const &cntr) -> todo<query::book<Cntr>>;

  template <typename Val, typename... Vars>
  auto vary(column::constant<Val> const &nom, Vars const &...vars);

  template <typename Expr, typename... Vars>
  auto vary(column::expression<Expr> const &expr, Vars const &...vars);

  template <typename Defn, typename... Vars>
  auto vary(column::definition<Defn> const &defn, Vars const &...vars);

  template <typename Lzy, typename... Vars>
  auto vary(systematic::nominal<Lzy> const &nom, Vars const &...vars);

  /* public, but not really... */

  template <typename Val>
  auto _assign(Val const &val) -> lazy<column::fixed<Val>>;

  template <typename To, typename Col>
  auto _convert(lazy<Col> const &col)
      -> lazy<column::conversion<To, column::value_t<Col>>>;

  template <typename Def, typename... Args> auto _define(Args &&...args);
  template <typename Def> auto _define(column::definition<Def> const &defn);

  template <typename Fn> auto _equate(Fn fn);
  template <typename Expr> auto _equate(column::expression<Expr> const &expr);

  template <typename Sel, typename Col>
  auto _select(lazy<Col> const &col) -> lazy<selection::node>;

  template <typename Sel, typename Col>
  auto _select(lazy<selection::node> const &prev, lazy<Col> const &col)
      -> lazy<selection::node>;

  template <typename Cntr, typename... Args>
  auto _aggregate(Args &&...args) -> todo<query::book<Cntr>>;

protected:
  template <typename Kwd> void accept_kwarg(Kwd const &kwarg);

  void analyze();
  void reset();

  template <typename DS, typename Val>
  auto _read(dataset::reader<DS> &ds, const std::string &name)
      -> lazy<read_column_t<DS, Val>>;

  template <typename Def, typename... Cols>
  auto _evaluate(todo<column::evaluate<Def>> const &calc,
                 lazy<Cols> const &...columns) -> lazy<Def>;

  template <typename Cntr>
  auto _book(todo<query::book<Cntr>> const &bkr,
             lazy<selection::node> const &sel) -> lazy<Cntr>;
  template <typename Cntr, typename... Sels>
  auto _book(todo<query::book<Cntr>> const &bkr, lazy<Sels> const &...sels)
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

} // namespace queryosity

#include "dataset_input.h"
#include "dataset_loaded.h"
#include "dataset_player.h"

#include "lazy.h"
#include "lazy_varied.h"
#include "todo.h"

#include "column_constant.h"
#include "column_expression.h"
#include "query_plan.h"

#include "systematic_nominal.h"
#include "systematic_resolver.h"
#include "systematic_variation.h"

inline queryosity::dataflow::dataflow()
    : m_mt(queryosity::multithread::disable()), m_nrows(-1), m_weight(1.0),
      m_ds(nullptr), m_analyzed(false) {}

template <typename Kwd> queryosity::dataflow::dataflow(Kwd kwarg) : dataflow() {
  this->accept_kwarg<Kwd>(kwarg);
}

template <typename Kwd1, typename Kwd2>
queryosity::dataflow::dataflow(Kwd1 kwarg1, Kwd2 kwarg2) : dataflow() {
  static_assert(!std::is_same_v<Kwd1, Kwd2>, "repeated keyword arguments.");
  this->accept_kwarg<Kwd1>(kwarg1);
  this->accept_kwarg<Kwd2>(kwarg2);
}

template <typename Kwd>
void queryosity::dataflow::accept_kwarg(Kwd const &kwarg) {
  constexpr bool is_mt = std::is_same_v<Kwd, multithread::core>;
  constexpr bool is_weight = std::is_same_v<Kwd, dataset::weight>;
  constexpr bool is_nrows = std::is_same_v<Kwd, dataset::head>;
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
queryosity::dataflow::dataflow(Kwd1 kwarg1, Kwd2 kwarg2, Kwd3 kwarg3)
    : dataflow() {
  static_assert(!std::is_same_v<Kwd1, Kwd2>, "repeated keyword arguments.");
  static_assert(!std::is_same_v<Kwd1, Kwd3>, "repeated keyword arguments.");
  static_assert(!std::is_same_v<Kwd2, Kwd3>, "repeated keyword arguments.");
  this->accept_kwarg<Kwd1>(kwarg1);
  this->accept_kwarg<Kwd2>(kwarg2);
  this->accept_kwarg<Kwd3>(kwarg3);
}

template <typename DS>
auto queryosity::dataflow::load(queryosity::dataset::input<DS> &&in)
    -> queryosity::dataset::loaded<DS> {

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

  return dataset::loaded<DS>(*this, *ds_rdr);
}

template <typename DS, typename Val>
auto queryosity::dataflow::read(queryosity::dataset::input<DS> in,
                                queryosity::dataset::column<Val> const &col) {
  auto ds = this->load<DS>(std::move(in));
  return ds.read(col);
}

template <typename DS, typename... Vals>
auto queryosity::dataflow::read(
    queryosity::dataset::input<DS> in,
    queryosity::dataset::columns<Vals...> const &cols) {
  auto ds = this->load<DS>(std::move(in));
  return ds.read(cols);
}

template <typename Val>
auto queryosity::dataflow::define(queryosity::column::constant<Val> const &cnst)
    -> lazy<column::fixed<Val>> {
  return cnst._assign(*this);
}

template <typename Def, typename... Cols>
auto queryosity::dataflow::define(
    queryosity::column::definition<Def> const &defn, lazy<Cols> const &...cols)
    -> lazy<Def> {
  return this->_define(defn).template evaluate(cols...);
}

template <typename Expr, typename... Cols>
auto queryosity::dataflow::define(
    queryosity::column::expression<Expr> const &expr, lazy<Cols> const &...cols)
    -> lazy<column::equation_t<queryosity::column::expression<Expr>>> {
  return this->_equate(expr).template evaluate(cols...);
}

template <typename Col>
auto queryosity::dataflow::filter(lazy<Col> const &col)
    -> lazy<selection::node> {
  return this->_select<selection::cut>(col);
}

template <typename Col>
auto queryosity::dataflow::weight(lazy<Col> const &col)
    -> lazy<selection::node> {
  return this->_select<selection::weight>(col);
}

template <typename Col> auto queryosity::dataflow::filter(Col const &col) {
  using varied_type = typename lazy<selection::node>::varied;
  varied_type syst(this->filter(col.nominal()));
  for (auto const &var_name : col.list_variation_names()) {
    syst.set_variation(var_name, this->filter(col.variation(var_name)));
  }
  return syst;
}

template <typename Col> auto queryosity::dataflow::weight(Col const &col) {
  using varied_type = typename lazy<selection::node>::varied;
  varied_type syst(this->weight(col.nominal()));
  for (auto const &var_name : col.list_variation_names()) {
    syst.set_variation(var_name, this->weight(col.variation(var_name)));
  }
  return syst;
}

template <typename Expr, typename... Cols>
auto queryosity::dataflow::filter(
    queryosity::column::expression<Expr> const &expr, Cols const &...cols) {
  auto dec = this->define(expr, cols...);
  return this->filter(dec);
}

template <typename Expr, typename... Cols>
auto queryosity::dataflow::weight(
    queryosity::column::expression<Expr> const &expr, Cols const &...cols) {
  auto dec = this->define(expr, cols...);
  return this->weight(dec);
}

template <typename Cntr, typename... Args>
auto queryosity::dataflow::_aggregate(Args &&...args)
    -> todo<query::book<Cntr>> {
  return todo<query::book<Cntr>>(*this, concurrent::invoke(
                                            [&args...](dataset::player *plyr) {
                                              return plyr->template get<Cntr>(
                                                  std::forward<Args>(args)...);
                                            },
                                            m_dplyrs));
}

template <typename Cntr>
auto queryosity::dataflow::make(queryosity::query::plan<Cntr> const &cntr)
    -> todo<query::book<Cntr>> {
  return cntr._aggregate(*this);
}

template <typename Def, typename... Cols>
auto queryosity::dataflow::_evaluate(todo<column::evaluate<Def>> const &calc,
                                     lazy<Cols> const &...columns)
    -> lazy<Def> {
  auto act = concurrent::invoke(
      [](dataset::player *plyr, column::evaluate<Def> *calc,
         Cols const *...cols) {
        return plyr->template evaluate(*calc, *cols...);
      },
      m_dplyrs, calc.get_slots(), columns.get_slots()...);
  auto lzy = lazy<Def>(*this, act);
  this->add_action(std::move(act));
  return lzy;
}

template <typename Cntr>
auto queryosity::dataflow::_book(todo<query::book<Cntr>> const &bkr,
                                 lazy<selection::node> const &sel)
    -> lazy<Cntr> {
  // new query booked: dataset will need to be analyzed
  this->reset();
  auto act = concurrent::invoke(
      [](dataset::player *plyr, query::book<Cntr> *bkr,
         const selection::node *sel) { return plyr->book(*bkr, *sel); },
      m_dplyrs, bkr.get_slots(), sel.get_slots());
  auto lzy = lazy<Cntr>(*this, act);
  this->add_action(std::move(act));
  return lzy;
}

template <typename Cntr, typename... Sels>
auto queryosity::dataflow::_book(todo<query::book<Cntr>> const &bkr,
                                 lazy<Sels> const &...sels)
    -> std::array<lazy<Cntr>, sizeof...(Sels)> {
  return std::array<lazy<Cntr>, sizeof...(Sels)>{this->_book(bkr, sels)...};
}

inline void queryosity::dataflow::analyze() {

  // 0. do not analyze if already done
  if (m_analyzed)
    return;

  // 1. partition the dataset
  auto ds_parts = m_ds->partition();
  // truncate entries to row limit
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

inline void queryosity::dataflow::reset() { m_analyzed = false; }

template <typename Val, typename... Vars>
auto queryosity::dataflow::vary(queryosity::column::constant<Val> const &cnst,
                                Vars const &...vars) {
  auto nom = this->define(cnst);
  using varied_type = typename decltype(nom)::varied;
  varied_type syst(std::move(nom));
  ((this->_vary(syst, vars.name(),
                column::constant<Val>(std::get<0>(vars.args())))),
   ...);
  return syst;
}

template <typename Expr, typename... Vars>
auto queryosity::dataflow::vary(
    queryosity::column::expression<Expr> const &expr, Vars const &...vars) {
  auto nom = this->_equate(expr);
  using varied_type = typename decltype(nom)::varied;
  using function_type = typename column::expression<Expr>::function_type;
  varied_type syst(std::move(nom));
  ((this->_vary(syst, vars.name(),
                column::expression(function_type(std::get<0>(vars.args()))))),
   ...);
  return syst;
}

template <typename Lzy, typename... Vars>
auto queryosity::dataflow::vary(systematic::nominal<Lzy> const &nom,
                                Vars const &...vars) {
  using action_type = typename Lzy::action_type;
  using value_type = column::template value_t<action_type>;
  using nominal_type = lazy<column::valued<value_type>>;
  using varied_type = typename nominal_type::varied;
  const auto identity =
      column::expression([](value_type const &x) { return x; });
  varied_type syst(nom.get().template to<value_type>());
  (syst.set_variation(
       vars.name(),
       this->define(identity, vars.get()).template to<value_type>()),
   ...);
  return syst;
}

template <typename Action,
          std::enable_if_t<std::is_base_of_v<queryosity::action, Action>, bool>>
void queryosity::dataflow::add_action(
    std::vector<std::unique_ptr<Action>> slots) {
  m_actions.reserve(m_actions.size() + slots.size());
  for (unsigned int i = 0; i < slots.size(); ++i) {
    m_actions.push_back(std::move(slots[i]));
  }
}

template <typename DS, typename Val>
auto queryosity::dataflow::_read(dataset::reader<DS> &ds,
                                 const std::string &name)
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
auto queryosity::dataflow::_assign(Val const &val)
    -> lazy<queryosity::column::fixed<Val>> {
  auto act = concurrent::invoke(
      [&val](dataset::player *plyr) { return plyr->template assign<Val>(val); },
      m_dplyrs);
  auto lzy = lazy<column::fixed<Val>>(*this, act);
  this->add_action(std::move(act));
  return lzy;
}

template <typename To, typename Col>
auto queryosity::dataflow::_convert(lazy<Col> const &col) -> lazy<
    queryosity::column::conversion<To, queryosity::column::value_t<Col>>> {
  auto act = concurrent::invoke(
      [](dataset::player *plyr, Col const *from) {
        return plyr->template convert<To>(from);
      },
      m_dplyrs, col);
  auto lzy = lazy<column::conversion<To, column::value_t<Col>>>(*this, act);
  this->add_action(std::move(act));
  return lzy;
}

template <typename Def, typename... Args>
auto queryosity::dataflow::_define(Args &&...args) {
  return todo<queryosity::column::template evaluate_t<Def>>(
      *this,
      concurrent::invoke(
          [&args...](dataset::player *plyr) {
            return plyr->template define<Def>(std::forward<Args>(args)...);
          },
          m_dplyrs));
}

template <typename Def>
auto queryosity::dataflow::_define(
    queryosity::column::definition<Def> const &defn) {
  return defn._define(*this);
}

template <typename Fn> auto queryosity::dataflow::_equate(Fn fn) {
  return todo<queryosity::column::template evaluate_t<Fn>>(
      *this,
      concurrent::invoke(
          [fn](dataset::player *plyr) { return plyr->template equate(fn); },
          m_dplyrs));
}

template <typename Expr>
auto queryosity::dataflow::_equate(
    queryosity::column::expression<Expr> const &expr) {
  return expr._equate(*this);
}

template <typename Sel, typename Col>
auto queryosity::dataflow::_select(lazy<Col> const &dec)
    -> lazy<selection::node> {
  auto act = concurrent::invoke(
      [](dataset::player *plyr, Col *col) {
        return plyr->template select<Sel>(nullptr, *col);
      },
      m_dplyrs, dec.get_slots());
  auto lzy = lazy<selection::node>(*this, act);
  this->add_action(std::move(act));
  return lzy;
}

template <typename Sel, typename Col>
auto queryosity::dataflow::_select(lazy<selection::node> const &prev,
                                   lazy<Col> const &dec)
    -> lazy<selection::node> {
  auto act = concurrent::invoke(
      [](dataset::player *plyr, selection::node *prev, Col *col) {
        return plyr->template select<Sel>(prev, *col);
      },
      m_dplyrs, prev.get_slots(), dec.get_slots());
  auto lzy = lazy<selection::node>(*this, act);
  this->add_action(std::move(act));
  return lzy;
}

template <typename Syst, typename Val>
void queryosity::dataflow::_vary(
    Syst &syst, const std::string &name,
    queryosity::column::constant<Val> const &cnst) {
  syst.set_variation(name, this->define(cnst));
}

template <typename Syst, typename Expr>
void queryosity::dataflow::_vary(
    Syst &syst, const std::string &name,
    queryosity::column::expression<Expr> const &expr) {
  syst.set_variation(name, this->_equate(expr));
}