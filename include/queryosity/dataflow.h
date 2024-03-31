#pragma once

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
   * @warning A dataset should *not* be loaded in more than once. Doing so
   * incurs an I/O overhead at best, and a potential thread-unsafe data race at
   * worst(as an entry will be read out multiple times concurrently).
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
  auto read(dataset::input<DS> in, dataset::column<Vals> const &...cols);

  /**
   * @brief Define a constant column.
   * @tparam Val Column data type.
   * @param[in] cnst Constant value.
   */
  template <typename Val>
  auto define(column::constant<Val> const &cnst) -> lazy<column::fixed<Val>>;

  /**
   * @brief Define a column using an expression.
   * @tparam Fn Callable type.
   * @tparam Cols Input column types.
   * @param[in] expr C++ function, functor, lambda, or any other callable.
   * @param[in] cols Input columns.
   * @return Lazy column.
   */
  template <typename Fn, typename... Cols>
  auto
  define(column::expression<Fn> const &expr) -> lazy<column::equation_t<Fn>>;

  /**
   * @brief Define a column using an expression.
   * @tparam Fn Callable type.
   * @tparam Cols Input column types.
   * @param[in] expr C++ function, functor, lambda, or any other callable.
   * @param[in] cols Input columns.
   * @return Lazy column.
   */
  template <typename Fn, typename... Cols>
  auto define(column::expression<Fn> const &expr,
              lazy<Cols> const &...cols) -> lazy<column::equation_t<Fn>>;

  /**
   * @brief Define a varied column using an expression.
   * @tparam Fn Callable type.
   * @tparam Cols Input column types.
   * @param[in] expr C++ function, functor, lambda, or any other callable.
   * @param[in] cols (Varied) input columns.
   * @return Varied lazy column.
   */
  template <typename Fn, typename... Cols>
  auto define(column::expression<Fn> const &expr, Cols const &...cols) ->
      typename lazy<column::equation_t<Fn>>::varied;

  /**
   * @brief Define a custom column.
   * @tparam Def `column::definition<Out(Ins...)>` implementation.
   * @tparam Cols Input column types.
   * @param[in] defn Constructor arguments for `Def`.
   * @param[in] cols Input columns.
   * @return Lazy column.
   */
  template <typename Def, typename... Cols>
  auto define(column::definition<Def> const &defn,
              lazy<Cols> const &...cols) -> lazy<Def>;

  /**
   * @brief Define a custom column.
   * @tparam Def `column::definition<Out(Ins...)>` implementation.
   * @tparam Cols Input column types.
   * @param[in] defn Constructor arguments for `Def`.
   * @param[in] cols (Varied) input columns.
   * @return Varied lazy column.
   */
  template <typename Def, typename... Cols>
  auto define(column::definition<Def> const &defn, Cols const &...cols) ->
      typename lazy<Def>::varied;

  /**
   * @brief Select all entries.
   * @return Lazy selection with cut passing for all entries and weight equal to
   * unity.
   */
  auto all() -> lazy<selection::node>;

  /**
   * @brief Initiate a cutflow.
   * @tparam Col Column type.
   * @param[in] column Input column value used as cut decision.
   * @return Lazy selection.
   */
  template <typename Col>
  auto filter(lazy<Col> const &column) -> lazy<selection::node>;

  /**
   * @brief Initiate a cutflow.
   * @tparam Col Column type.
   * @param[in] column Input column value used as weight decision.
   * @return Lazy selection.
   */
  template <typename Col>
  auto weight(lazy<Col> const &column) -> lazy<selection::node>;

  /**
   * @brief Initiate a cutflow.
   * @tparam Col Lazy varied column.
   * @param[in] column Input column value used as cut decision.
   * @return Lazy varied selection.
   */
  template <typename Col> auto filter(Col const &col);

  /**
   * @brief Initiate a cutflow.
   * @tparam Col Lazy varied column.
   * @param[in] column Input column value used as weight decision.
   * @return Lazy varied selection.
   */
  template <typename Col> auto weight(Col const &col);

  /**
   * @brief Initiate a cutflow.
   * @tparam Fn C++ Callable object.
   * @tparam Cols Column types.
   * @param[in] Input (varied) columns used to evaluate cut decision.
   * @return Lazy (varied) selection.
   */
  template <typename Fn, typename... Cols>
  auto filter(column::constant<Fn> const &expr) -> lazy<selection::node>;

  /**
   * @brief Initiate a cutflow.
   * @tparam Fn C++ Callable object.
   * @tparam Cols Column types.
   * @param[in] Input (varied) columns used to evaluate cut decision.
   * @return Lazy (varied) selection.
   */
  template <typename Val>
  auto weight(column::constant<Val> const &expr) -> lazy<selection::node>;

  /**
   * @brief Initiate a cutflow.
   * @tparam Fn C++ Callable object.
   * @tparam Cols Column types.
   * @param[in] Input (varied) columns used to evaluate cut decision.
   * @return Lazy (varied) selection.
   */
  template <typename Fn, typename... Cols>
  auto filter(column::expression<Fn> const &expr, Cols const &...cols);

  /**
   * @brief Initiate a cutflow.
   * @tparam Fn C++ Callable object.
   * @tparam Cols Column types.
   * @param[in] Input (varied) columns used to evaluate weight decision.
   * @return Lazy (varied) selection.
   */
  template <typename Fn, typename... Cols>
  auto weight(column::expression<Fn> const &expr, Cols const &...cols);

  /**
   * @brief Plan a query.
   * @tparam Qry Concrete queryosity::query::definition implementation.
   * @param[in] plan Query plan (constructor arguments).
   * @return queryosity::todo query booker.
   */
  template <typename Qry>
  auto make(query::plan<Qry> const &plan) -> todo<query::booker<Qry>>;

  template <typename Val>
  auto vary(column::constant<Val> const &cnst, std::map<std::string, Val> vars);

  template <typename Fn>
  auto
  vary(column::expression<Fn> const &expr,
       std::map<std::string,
                typename column::expression<Fn>::function_type> const &vars);

  template <typename Defn, typename... Vars>
  auto vary(column::definition<Defn> const &defn, Vars const &...vars);

  /* "public" API for Python layer */

  template <typename To, typename Col>
  auto _convert(lazy<Col> const &col)
      -> lazy<column::conversion<To, column::value_t<Col>>>;

  template <typename Val>
  auto _assign(Val const &val) -> lazy<column::fixed<Val>>;

  template <typename Def, typename... Args> auto _define(Args &&...args);
  template <typename Def>
  auto
  _define(column::definition<Def> const &defn) -> todo<column::evaluator<Def>>;

  template <typename Fn> auto _equate(Fn fn);
  template <typename Fn>
  auto _equate(column::expression<Fn> const &expr)
      -> todo<column::evaluator<column::equation_t<Fn>>>;

  template <typename Sel, typename Col>
  auto _select(lazy<Col> const &col) -> lazy<selection::node>;

  template <typename Sel, typename Col>
  auto _select(lazy<selection::node> const &prev,
               lazy<Col> const &col) -> lazy<selection::node>;

  template <typename Qry, typename... Args>
  auto _make(Args &&...args) -> todo<query::booker<Qry>>;

protected:
  template <typename Kwd> void accept_kwarg(Kwd &&kwarg);

  void analyze();
  void reset();

  template <typename DS, typename Val>
  auto _read(dataset::reader<DS> &ds,
             const std::string &name) -> lazy<read_column_t<DS, Val>>;

  template <typename Def, typename... Cols>
  auto _evaluate(todo<column::evaluator<Def>> const &calc,
                 lazy<Cols> const &...columns) -> lazy<Def>;

  template <typename Qry>
  auto _book(todo<query::booker<Qry>> const &bkr,
             lazy<selection::node> const &sel) -> lazy<Qry>;
  template <typename Qry, typename... Sels>
  auto _book(todo<query::booker<Qry>> const &bkr, lazy<Sels> const &...sels)
      -> std::array<lazy<Qry>, sizeof...(Sels)>;

  template <typename Syst, typename Val>
  void _vary(Syst &syst, const std::string &name,
             column::constant<Val> const &cnst);

  template <typename Syst, typename Fn>
  void _vary(Syst &syst, const std::string &name,
             column::expression<Fn> const &expr);

  template <typename Syst, typename Defn>
  void _vary(Syst &syst, const std::string &name,
             column::definition<Defn> const &defn);

protected:
  dataset::processor m_processor;
  dataset::weight m_weight;
  long long m_nrows;

  std::vector<std::unique_ptr<dataset::source>> m_sources;
  std::vector<unsigned int> m_dslots;

  std::unique_ptr<lazy<selection::node>> m_all;

  bool m_analyzed;
};

class dataflow::node {

public:
  friend class dataflow;

public:
  template <typename Fn, typename... Nodes>
  static auto invoke(Fn fn, Nodes const &...nodes)
      -> std::enable_if_t<
          !std::is_void_v<
              std::invoke_result_t<Fn, typename Nodes::action_type *...>>,
          std::vector<
              std::invoke_result_t<Fn, typename Nodes::action_type *...>>>;

  template <typename Fn, typename... Nodes>
  static auto invoke(Fn fn, Nodes const &...nodes)
      -> std::enable_if_t<std::is_void_v<std::invoke_result_t<
                              Fn, typename Nodes::action_type *...>>,
                          void>;

public:
  node(dataflow &df);
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
      m_all(nullptr), m_analyzed(false) {}

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
  constexpr bool is_mt_v = std::is_same_v<Kwd, dataset::processor>;
  constexpr bool is_weight_v = std::is_same_v<Kwd, dataset::weight>;
  constexpr bool is_nrows_v = std::is_same_v<Kwd, dataset::head>;
  if constexpr (is_mt_v) {
    m_processor = std::move(kwarg);
  } else if (is_weight_v) {
    m_weight = kwarg;
  } else if (is_nrows_v) {
    m_nrows = kwarg;
  } else {
    static_assert(is_mt_v || is_weight_v || is_nrows_v,
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
    queryosity::dataset::column<Vals> const &...cols) {
  auto ds = this->load<DS>(std::move(in));
  return ds.read(cols...);
}

template <typename Val>
auto queryosity::dataflow::define(column::constant<Val> const &cnst)
    -> lazy<column::fixed<Val>> {
  return cnst._assign(*this);
}

template <typename Def, typename... Cols>
auto queryosity::dataflow::define(column::definition<Def> const &defn,
                                  lazy<Cols> const &...cols) -> lazy<Def> {
  return this->_define(defn).template evaluate(cols...);
}

template <typename Def, typename... Cols>
auto queryosity::dataflow::define(column::definition<Def> const &defn,
                                  Cols const &...cols) ->
    typename lazy<Def>::varied {
  return this->_define(defn).template evaluate(cols...);
}

template <typename Fn, typename... Cols>
auto queryosity::dataflow::define(column::expression<Fn> const &expr)
    -> lazy<column::equation_t<Fn>> {
  return this->_equate(expr).template evaluate();
}

template <typename Fn, typename... Cols>
auto queryosity::dataflow::define(column::expression<Fn> const &expr,
                                  lazy<Cols> const &...cols)
    -> lazy<column::equation_t<Fn>> {
  return this->_equate(expr).template evaluate(cols...);
}

template <typename Fn, typename... Cols>
auto queryosity::dataflow::define(column::expression<Fn> const &expr,
                                  Cols const &...cols) ->
    typename lazy<column::equation_t<Fn>>::varied {
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

auto queryosity::dataflow::all() -> lazy<selection::node> {
  if (!m_all) {
    m_all = std::make_unique<lazy<selection::node>>(
        this->filter(column::constant(true)));
  }
  return *m_all;
}

template <typename Fn, typename... Cols>
auto queryosity::dataflow::filter(column::constant<Fn> const &cnst)
    -> lazy<selection::node> {
  return this->filter(this->define(cnst));
}

template <typename Fn, typename... Cols>
auto queryosity::dataflow::filter(column::expression<Fn> const &expr,
                                  Cols const &...cols) {
  return this->filter(this->define(expr, cols...));
}

template <typename Val>
auto queryosity::dataflow::weight(column::constant<Val> const &cnst)
    -> lazy<selection::node> {
  return this->weight(this->define(cnst));
}

template <typename Fn, typename... Cols>
auto queryosity::dataflow::weight(column::expression<Fn> const &expr,
                                  Cols const &...cols) {
  return this->weight(this->define(expr, cols...));
}

template <typename Qry, typename... Args>
auto queryosity::dataflow::_make(Args &&...args) -> todo<query::booker<Qry>> {
  return todo<query::booker<Qry>>(*this, ensemble::invoke(
                                             [&args...](dataset::player *plyr) {
                                               return plyr->template make<Qry>(
                                                   std::forward<Args>(args)...);
                                             },
                                             m_processor.get_slots()));
}

template <typename Qry>
auto queryosity::dataflow::make(queryosity::query::plan<Qry> const &cntr)
    -> todo<query::booker<Qry>> {
  return cntr._make(*this);
}

template <typename Def, typename... Cols>
auto queryosity::dataflow::_evaluate(todo<column::evaluator<Def>> const &calc,
                                     lazy<Cols> const &...columns)
    -> lazy<Def> {
  auto act = ensemble::invoke(
      [](dataset::player *plyr, column::evaluator<Def> *calc,
         Cols const *...cols) {
        return plyr->template evaluate(*calc, *cols...);
      },
      m_processor.get_slots(), calc.get_slots(), columns.get_slots()...);
  auto lzy = lazy<Def>(*this, act);
  return lzy;
}

template <typename Qry>
auto queryosity::dataflow::_book(todo<query::booker<Qry>> const &bkr,
                                 lazy<selection::node> const &sel)
    -> lazy<Qry> {
  // new query booked: dataset will need to be analyzed
  this->reset();
  auto act = ensemble::invoke(
      [](dataset::player *plyr, query::booker<Qry> *bkr,
         const selection::node *sel) { return plyr->book(*bkr, *sel); },
      m_processor.get_slots(), bkr.get_slots(), sel.get_slots());
  auto lzy = lazy<Qry>(*this, act);
  return lzy;
}

template <typename Qry, typename... Sels>
auto queryosity::dataflow::_book(todo<query::booker<Qry>> const &bkr,
                                 lazy<Sels> const &...sels)
    -> std::array<lazy<Qry>, sizeof...(Sels)> {
  return std::array<lazy<Qry>, sizeof...(Sels)>{this->_book(bkr, sels)...};
}

inline void queryosity::dataflow::analyze() {
  if (m_analyzed)
    return;

  m_processor.process(m_sources, m_weight, m_nrows);
  m_analyzed = true;
}

inline void queryosity::dataflow::reset() { m_analyzed = false; }

template <typename Val>
auto queryosity::dataflow::vary(column::constant<Val> const &cnst,
                                std::map<std::string, Val> vars) {
  auto nom = this->define(cnst);
  using varied_type = typename decltype(nom)::varied;
  varied_type syst(std::move(nom));
  for (auto const &var : vars) {
    this->_vary(syst, var.first, column::constant<Val>(var.second));
  }
  return syst;
}

template <typename Fn>
auto queryosity::dataflow::vary(
    column::expression<Fn> const &expr,
    std::map<std::string, typename column::expression<Fn>::function_type> const
        &vars) {
  auto nom = this->_equate(expr);
  using varied_type = typename decltype(nom)::varied;
  using function_type = typename column::expression<Fn>::function_type;
  varied_type syst(std::move(nom));
  for (auto const &var : vars) {
    this->_vary(syst, var.first, column::expression<function_type>(var.second));
  }
  return syst;
}

template <typename Defn, typename... Vars>
auto queryosity::dataflow::vary(column::definition<Defn> const &defn,
                                Vars const &...vars) {
  auto nom = this->_define(defn);
  using varied_type = typename decltype(nom)::varied;
  varied_type syst(std::move(nom));
  ((this->_vary(syst, vars.name(),
                vars.template _vary_arg<column::definition<Defn>>())),
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
auto queryosity::dataflow::_assign(Val const &val) -> lazy<column::fixed<Val>> {
  auto act = ensemble::invoke(
      [&val](dataset::player *plyr) { return plyr->template assign<Val>(val); },
      m_processor.get_slots());
  auto lzy = lazy<column::fixed<Val>>(*this, act);
  return lzy;
}

template <typename To, typename Col>
auto queryosity::dataflow::_convert(lazy<Col> const &col)
    -> lazy<column::conversion<To, column::value_t<Col>>> {
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
  return todo<column::evaluator<Def>>(*this,
                                      ensemble::invoke(
                                          [&args...](dataset::player *plyr) {
                                            return plyr->template define<Def>(
                                                std::forward<Args>(args)...);
                                          },
                                          m_processor.get_slots()));
}

template <typename Def>
auto queryosity::dataflow::_define(column::definition<Def> const &defn)
    -> todo<column::evaluator<Def>> {
  return defn._define(*this);
}

template <typename Fn> auto queryosity::dataflow::_equate(Fn fn) {
  return todo<column::evaluator<typename column::equation_t<Fn>>>(
      *this,
      ensemble::invoke(
          [fn](dataset::player *plyr) { return plyr->template equate(fn); },
          m_processor.get_slots()));
}

template <typename Fn>
auto queryosity::dataflow::_equate(column::expression<Fn> const &expr)
    -> todo<column::evaluator<column::equation_t<Fn>>> {
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
void queryosity::dataflow::_vary(Syst &syst, const std::string &name,
                                 column::constant<Val> const &cnst) {
  syst.set_variation(name, this->define(cnst));
}

template <typename Syst, typename Fn>
void queryosity::dataflow::_vary(Syst &syst, const std::string &name,
                                 column::expression<Fn> const &expr) {
  syst.set_variation(name, this->_equate(expr));
}

template <typename Syst, typename Defn>
void queryosity::dataflow::_vary(Syst &syst, const std::string &name,
                                 column::definition<Defn> const &defn) {
  syst.set_variation(name, this->_define(defn));
}

inline queryosity::dataflow::node::node(dataflow &df) : m_df(&df) {}

template <typename Fn, typename... Nodes>
auto queryosity::dataflow::node::invoke(Fn fn, Nodes const &...nodes)
    -> std::enable_if_t<
        !std::is_void_v<
            std::invoke_result_t<Fn, typename Nodes::action_type *...>>,
        std::vector<
            std::invoke_result_t<Fn, typename Nodes::action_type *...>>> {
  return ensemble::invoke(fn, nodes.get_slots()...);
}

template <typename Fn, typename... Nodes>
auto queryosity::dataflow::node::invoke(Fn fn, Nodes const &...nodes)
    -> std::enable_if_t<std::is_void_v<std::invoke_result_t<
                            Fn, typename Nodes::action_type *...>>,
                        void> {
  ensemble::invoke(fn, nodes.get_slots()...);
}