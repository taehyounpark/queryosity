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
#include "dataset_processor.h"
#include "multithread.h"
#include "query.h"
#include "selection.h"
#include "systematic.h"

namespace queryosity {

template <typename T> class lazy;

template <typename U> class todo;

/**
 * @ingroup api
 * @brief Main dataflow interface.
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

  template <typename Kwd> dataflow(Kwd &&kwarg);
  template <typename Kwd1, typename Kwd2>
  dataflow(Kwd1 &&kwarg1, Kwd2 &&kwarg2);

  /**
   * @brief Constructor with (up to) three keyword arguments.
   * @details Each keyword argument should be one of the following:
   *
   *  - `queryosity::multithread::enable(unsigned int)`
   *  - `queryosity::multithread::disable()`
   *  - `queryosity::dataset::head(unsigned int)`
   *  - `queryosity::dataset::weight(float)`
   *
   */
  template <typename Kwd1, typename Kwd2, typename Kwd3>
  dataflow(Kwd1 &&kwarg1, Kwd2 &&kwarg2, Kwd3 &&kwarg3);

  dataflow(dataflow const &) = delete;
  dataflow &operator=(dataflow const &) = delete;

  dataflow(dataflow &&) = default;
  dataflow &operator=(dataflow &&) = default;

  /**
   * @brief Load a dataset input.
   * @tparam DS `dataset::reader<Self>` implementation.
   * @tparam Args... Constructor arguments.
   * @return Loaded dataset.
   */
  template <typename DS>
  auto load(dataset::input<DS> &&in) -> dataset::loaded<DS>;

  /**
   * @brief Read a column from an input dataset.
   * @attention A dataset should be loaded-in *once*. Use this method only if
   * you are interested in the requested column, as other columns will not be
   * readable.
   * @tparam DS `dataset::reader<Self>` implementation.
   * @tparam Val Column data type.
   * @param[in] Column name.
   * @return Column read from the loaded dataset.
   */
  template <typename DS, typename Val>
  auto read(dataset::input<DS> in, dataset::column<Val> const &col);

  /**
   * @brief Read columns from an input dataset.
   * @attention A dataset should be loaded-in *once*. Use this method only if
   * you are interested in the requested columns, as other columns will not be
   * readable.
   * @tparam DS `dataset::reader<Self>` implementation.
   * @tparam Vals Column data types.
   * @param[in] cols Column names.
   * @return Columns read from the loaded dataset.
   */
  template <typename DS, typename... Vals>
  auto read(dataset::input<DS> in, dataset::columns<Vals...> const &cols);

  /**
   * @brief Define a constant column.
   * @tparam Val Column data type.
   * @param[in] cnst Constant value.
   */
  template <typename Val>
  auto define(column::constant<Val> const &cnst) -> lazy<column::fixed<Val>>;

  /**
   * @brief Define a column using an expression.
   * @tparam Expr Callable type.
   * @tparam Cols Input column types.
   * @param[in] expr C++ function, functor, lambda, or any other callable.
   * @param[in] cols Input columns.
   * @return Lazy column.
   */
  template <typename Expr, typename... Cols>
  auto define(column::expression<Expr> const &expr, lazy<Cols> const &...cols)
      -> lazy<column::equation_t<queryosity::column::expression<Expr>>>;

  /**
   * @brief Define a custom column.
   * @tparam Def `column::definition<Out(Ins...)>` implementation.
   * @tparam Cols Input column types.
   * @param[in] defn Constructor arguments for `Def`.
   * @param[in] cols Input columns.
   * @return Lazy column.
   */
  template <typename Def, typename... Cols>
  auto define(column::definition<Def> const &defn, lazy<Cols> const &...cols)
      -> lazy<Def>;

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
  auto _make(Args &&...args) -> todo<query::book<Cntr>>;

protected:
  template <typename Kwd> void accept_kwarg(Kwd &&kwarg);

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

protected:
  dataset::processor m_processor;
  dataset::weight m_weight;
  long long m_nrows;

  std::vector<std::unique_ptr<dataset::source>> m_sources;
  std::vector<unsigned int> m_dslots;

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
    : m_processor(multithread::disable()), m_weight(1.0), m_nrows(-1),
      m_analyzed(false) {}

template <typename Kwd>
queryosity::dataflow::dataflow(Kwd &&kwarg) : dataflow() {
  this->accept_kwarg(std::forward<Kwd>(kwarg));
}

template <typename Kwd1, typename Kwd2>
queryosity::dataflow::dataflow(Kwd1 &&kwarg1, Kwd2 &&kwarg2) : dataflow() {
  static_assert(!std::is_same_v<Kwd1, Kwd2>, "repeated keyword arguments.");
  this->accept_kwarg(std::forward<Kwd1>(kwarg1));
  this->accept_kwarg(std::forward<Kwd2>(kwarg2));
}

template <typename Kwd1, typename Kwd2, typename Kwd3>
queryosity::dataflow::dataflow(Kwd1 &&kwarg1, Kwd2 &&kwarg2, Kwd3 &&kwarg3)
    : dataflow() {
  static_assert(!std::is_same_v<Kwd1, Kwd2>, "repeated keyword arguments.");
  static_assert(!std::is_same_v<Kwd1, Kwd3>, "repeated keyword arguments.");
  static_assert(!std::is_same_v<Kwd2, Kwd3>, "repeated keyword arguments.");
  this->accept_kwarg(std::forward<Kwd1>(kwarg1));
  this->accept_kwarg(std::forward<Kwd2>(kwarg2));
  this->accept_kwarg(std::forward<Kwd3>(kwarg3));
}

template <typename Kwd> void queryosity::dataflow::accept_kwarg(Kwd &&kwarg) {
  constexpr bool is_mt = std::is_same_v<Kwd, dataset::processor>;
  constexpr bool is_weight = std::is_same_v<Kwd, dataset::weight>;
  constexpr bool is_nrows = std::is_same_v<Kwd, dataset::head>;
  if constexpr (is_mt) {
    m_processor = std::move(kwarg);
  } else if (is_weight) {
    m_weight = kwarg;
  } else if (is_nrows) {
    m_nrows = kwarg;
  } else {
    static_assert(is_mt || is_weight || is_nrows,
                  "unrecognized keyword argument");
  }
}

template <typename DS>
auto queryosity::dataflow::load(queryosity::dataset::input<DS> &&in)
    -> queryosity::dataset::loaded<DS> {

  auto ds = in.ds.get();

  m_sources.emplace_back(std::move(in.ds));
  m_sources.back()->parallelize(m_processor.concurrency());

  return dataset::loaded<DS>(*this, *ds);
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
  for (auto const &var_name : col.get_variation_names()) {
    syst.set_variation(var_name, this->filter(col.variation(var_name)));
  }
  return syst;
}

template <typename Col> auto queryosity::dataflow::weight(Col const &col) {
  using varied_type = typename lazy<selection::node>::varied;
  varied_type syst(this->weight(col.nominal()));
  for (auto const &var_name : col.get_variation_names()) {
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
auto queryosity::dataflow::_make(Args &&...args) -> todo<query::book<Cntr>> {
  return todo<query::book<Cntr>>(*this, ensemble::invoke(
                                            [&args...](dataset::player *plyr) {
                                              return plyr->template make<Cntr>(
                                                  std::forward<Args>(args)...);
                                            },
                                            m_processor.get_slots()));
}

template <typename Cntr>
auto queryosity::dataflow::make(queryosity::query::plan<Cntr> const &cntr)
    -> todo<query::book<Cntr>> {
  return cntr._make(*this);
}

template <typename Def, typename... Cols>
auto queryosity::dataflow::_evaluate(todo<column::evaluate<Def>> const &calc,
                                     lazy<Cols> const &...columns)
    -> lazy<Def> {
  auto act = ensemble::invoke(
      [](dataset::player *plyr, column::evaluate<Def> *calc,
         Cols const *...cols) {
        return plyr->template evaluate(*calc, *cols...);
      },
      m_processor.get_slots(), calc.get_slots(), columns.get_slots()...);
  auto lzy = lazy<Def>(*this, act);
  return lzy;
}

template <typename Cntr>
auto queryosity::dataflow::_book(todo<query::book<Cntr>> const &bkr,
                                 lazy<selection::node> const &sel)
    -> lazy<Cntr> {
  // new query booked: dataset will need to be analyzed
  this->reset();
  auto act = ensemble::invoke(
      [](dataset::player *plyr, query::book<Cntr> *bkr,
         const selection::node *sel) { return plyr->book(*bkr, *sel); },
      m_processor.get_slots(), bkr.get_slots(), sel.get_slots());
  auto lzy = lazy<Cntr>(*this, act);
  return lzy;
}

template <typename Cntr, typename... Sels>
auto queryosity::dataflow::_book(todo<query::book<Cntr>> const &bkr,
                                 lazy<Sels> const &...sels)
    -> std::array<lazy<Cntr>, sizeof...(Sels)> {
  return std::array<lazy<Cntr>, sizeof...(Sels)>{this->_book(bkr, sels)...};
}

inline void queryosity::dataflow::analyze() {
  if (m_analyzed)
    return;

  m_processor.process(m_sources, m_weight, m_nrows);
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

template <typename DS, typename Val>
auto queryosity::dataflow::_read(dataset::reader<DS> &ds,
                                 const std::string &column_name)
    -> lazy<read_column_t<DS, Val>> {
  auto act = m_processor.read<DS, Val>(ds, column_name);
  auto lzy = lazy<read_column_t<DS, Val>>(*this, act);
  return lzy;
}

template <typename Val>
auto queryosity::dataflow::_assign(Val const &val)
    -> lazy<queryosity::column::fixed<Val>> {
  auto act = ensemble::invoke(
      [&val](dataset::player *plyr) { return plyr->template assign<Val>(val); },
      m_processor.get_slots());
  auto lzy = lazy<column::fixed<Val>>(*this, act);
  return lzy;
}

template <typename To, typename Col>
auto queryosity::dataflow::_convert(lazy<Col> const &col) -> lazy<
    queryosity::column::conversion<To, queryosity::column::value_t<Col>>> {
  auto act = ensemble::invoke(
      [](dataset::player *plyr, Col const *from) {
        return plyr->template convert<To>(*from);
      },
      m_processor.get_slots(), col.get_slots());
  auto lzy = lazy<column::conversion<To, column::value_t<Col>>>(*this, act);
  return lzy;
}

template <typename Def, typename... Args>
auto queryosity::dataflow::_define(Args &&...args) {
  return todo<queryosity::column::template evaluate_t<Def>>(
      *this,
      ensemble::invoke(
          [&args...](dataset::player *plyr) {
            return plyr->template define<Def>(std::forward<Args>(args)...);
          },
          m_processor.get_slots()));
}

template <typename Def>
auto queryosity::dataflow::_define(
    queryosity::column::definition<Def> const &defn) {
  return defn._define(*this);
}

template <typename Fn> auto queryosity::dataflow::_equate(Fn fn) {
  return todo<queryosity::column::template evaluate_t<Fn>>(
      *this,
      ensemble::invoke(
          [fn](dataset::player *plyr) { return plyr->template equate(fn); },
          m_processor.get_slots()));
}

template <typename Expr>
auto queryosity::dataflow::_equate(
    queryosity::column::expression<Expr> const &expr) {
  return expr._equate(*this);
}

template <typename Sel, typename Col>
auto queryosity::dataflow::_select(lazy<Col> const &dec)
    -> lazy<selection::node> {
  auto act = ensemble::invoke(
      [](dataset::player *plyr, Col *col) {
        return plyr->template select<Sel>(nullptr, *col);
      },
      m_processor.get_slots(), dec.get_slots());
  auto lzy = lazy<selection::node>(*this, act);
  return lzy;
}

template <typename Sel, typename Col>
auto queryosity::dataflow::_select(lazy<selection::node> const &prev,
                                   lazy<Col> const &dec)
    -> lazy<selection::node> {
  auto act = ensemble::invoke(
      [](dataset::player *plyr, selection::node *prev, Col *col) {
        return plyr->template select<Sel>(prev, *col);
      },
      m_processor.get_slots(), prev.get_slots(), dec.get_slots());
  auto lzy = lazy<selection::node>(*this, act);
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