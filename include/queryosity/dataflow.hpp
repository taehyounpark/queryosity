#pragma once

#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "column.hpp"
#include "dataset.hpp"
#include "dataset_partition.hpp"
#include "dataset_player.hpp"
#include "dataset_processor.hpp"
#include "multithread.hpp"
#include "query.hpp"
#include "selection.hpp"
#include "systematic.hpp"

namespace queryosity {

template <typename T> class lazy;

template <typename U> class todo;

template <typename T> class varied;

/**
 * @ingroup api
 * @brief Main dataflow interface.
 */
class dataflow {

public:
  template <typename> class input;
  class node;

public:
  template <typename> friend class input;
  template <typename> friend class lazy;
  template <typename> friend class todo;
  template <typename> friend class varied;
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
  auto load(dataset::input<DS> &&in) -> dataflow::input<DS>;

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
  auto define(column::constant<Val> const &cnst) -> lazy<column::valued<Val>>;

  /**
   * @brief Define a column using an expression.
   * @tparam Fn Callable type.
   * @param[in] expr C++ function, functor, lambda, or any other callable.
   * @return Evaluator.
   */
  template <typename Fn>
  auto define(column::expression<Fn> const &expr)
      -> todo<column::evaluator<column::equation_t<Fn>>>;

  /**
   * @brief Define a column using an expression.
   * @tparam Def Custom definition.
   * @param[in] defn Definition type and constructor arguments.
   * @return Evaluator.
   */
  template <typename Def>
  auto
  define(column::definition<Def> const &defn) -> todo<column::evaluator<Def>>;

  /**
   * @brief Initiate a cutflow.
   * @tparam Col Column type.
   * @param[in] column Input column used as cut decision.
   * @return Lazy selection.
   */
  template <typename Col>
  auto filter(lazy<Col> const &column) -> lazy<selection::node>;

  /**
   * @brief Initiate a cutflow.
   * @tparam Col Column type.
   * @param[in] column Input column used as weight decision.
   * @return Lazy selection.
   */
  template <typename Col>
  auto weight(lazy<Col> const &column) -> lazy<selection::node>;

  /**
   * @brief Initiate a cutflow.
   * @tparam Col Lazy varied column.
   * @param[in] column Input column used as cut decision.
   * @return Lazy varied selection.
   */
  template <typename Lzy> auto filter(varied<Lzy> const &col);

  /**
   * @brief Initiate a cutflow.
   * @tparam Col Lazy varied column.
   * @param[in] column Input column used as weight decision.
   * @return Lazy varied selection.
   */
  template <typename Lzy> auto weight(varied<Lzy> const &col);

  /**
   * @brief Initiate a cutflow.
   * @tparam Fn C++ Callable object.
   * @tparam Cols Column types.
   * @param[in] Input (varied) columns used to evaluate cut decision.
   * @return Lazy (varied) selection.
   */
  template <typename Fn, typename... Cols>
  auto filter(column::constant<Fn> const &cnst) -> lazy<selection::node>;

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
  template <typename Fn>
  auto filter(column::expression<Fn> const &expr)
      -> todo<selection::applicator<selection::cut, column::equation_t<Fn>>>;

  /**
   * @brief Initiate a cutflow.
   * @tparam Fn C++ Callable object.
   * @tparam Cols Column types.
   * @param[in] Input (varied) columns used to evaluate weight decision.
   * @return Lazy (varied) selection.
   */
  template <typename Fn>
  auto weight(column::expression<Fn> const &expr)
      -> todo<selection::applicator<selection::weight, column::equation_t<Fn>>>;

  /**
   * @brief Initiate a cutflow.
   * @tparam Def Column definition type.
   * @tparam Cols Column types.
   * @param[in] Input (varied) columns used to evaluate cut decision.
   * @return Lazy selection.
   */
  template <typename Def>
  auto filter(column::definition<Def> const &defn)
      -> todo<selection::applicator<selection::cut, Def>>;

  /**
   * @brief Initiate a cutflow.
   * @tparam Def Column definition type.
   * @tparam Cols Column types.
   * @param[in] Input (varied) columns used to evaluate weight decision.
   * @return Lazy (varied) selection.
   */
  template <typename Def>
  auto weight(column::definition<Def> const &defn)
      -> todo<selection::applicator<selection::weight, Def>>;

  /**
   * @brief Plan a query.
   * @tparam Qry Concrete queryosity::query::definition implementation.
   * @param[in] output Query output (constructor arguments).
   * @return queryosity::todo query booker.
   */
  template <typename Qry>
  auto get(query::output<Qry> const &output) -> todo<query::booker<Qry>>;

  // /**
  //  * @brief Get a column series.
  //  * @tparam Col (Varied) lazy column.
  //  * @param[in] col Column as series constructor argument.
  //  * @return (Varied) lazy column series query.
  //  */
  template <typename Col> auto get(column::series<Col> const &col);

  // /**
  //  * @brief Get selection yield.
  //  * @tparam Sels (Varied) lazy selection(s).
  //  * @param[in] sel Selection(s) as yield constructor argument(s).
  //  * @return (Varied) lazy selection yield query(ies).
  //  */
  template <typename... Sels> auto get(selection::yield<Sels...> const &sels);

  /**
   * @brief Vary a column constant.
   * @tparam Val Constant value type.
   * @param[in] cnst Column constant.
   * @param[in] vars Map of variation to value.
   * @return Varied lazy column.
   */
  template <typename Val>
  auto vary(column::constant<Val> const &cnst, std::map<std::string, Val> vars)
      -> varied<lazy<column::valued<Val>>>;

  /**
   * @brief Vary a column expression.
   * @tparam Fn Expression function type.
   * @param[in] expr Column expression.
   * @param[in] vars Map of variation to expression.
   * @return Varied todo column evaluator.
   */
  template <typename Fn>
  auto
  vary(column::expression<Fn> const &expr,
       std::map<std::string,
                typename column::expression<Fn>::function_type> const &vars)
      -> varied<todo<column::evaluator<column::equation_t<Fn>>>>;

  /**
   * @brief Vary a column definition.
   * @tparam Def Definition type.
   * @param[in] defn Column definition.
   * @param[in] vars Map of variation to definition.
   * @return Varied todo column evaluator.
   */
  template <typename Def>
  auto vary(column::definition<Def> const &defn,
            std::map<std::string, column::definition<Def>> const &vars)
      -> varied<todo<column::evaluator<Def>>>;

  /**
   * @brief Vary a column.
   * @tparam Col Column type.
   * @param[in] nom Nominal lazy column.
   * @param[in] vars Map of variation to lazy column.
   * @return Varied lazy column.
   */
  template <typename Col>
  auto vary(column::nominal<Col> const &nom,
            std::map<std::string, column::variation<column::value_t<Col>>> const
                &vars) -> varied<lazy<column::valued<column::value_t<Col>>>>;

  /* "public" API for Python layer */

  template <typename To, typename Col>
  auto _convert(lazy<Col> const &col)
      -> lazy<column::conversion<To, column::value_t<Col>>>;

  template <typename Val>
  auto _assign(Val const &val) -> lazy<column::valued<Val>>;

  template <typename Def>
  auto
  _define(column::definition<Def> const &defn) -> todo<column::evaluator<Def>>;

  template <typename Fn> auto _equate(Fn fn);
  template <typename Fn>
  auto _equate(column::expression<Fn> const &expr)
      -> todo<column::evaluator<column::equation_t<Fn>>>;

  template <typename Sel, typename Fn> auto _select(Fn fn);
  template <typename Sel, typename Fn>
  auto _select(lazy<selection::node> const &prev, Fn fn);

  template <typename Sel, typename Fn>
  auto _select(column::expression<Fn> const &expr)
      -> todo<selection::applicator<Sel, column::equation_t<Fn>>>;
  template <typename Sel, typename Fn>
  auto _select(lazy<selection::node> const &prev,
               column::expression<Fn> const &expr)
      -> todo<selection::applicator<Sel, column::equation_t<Fn>>>;

  template <typename Sel, typename Def>
  auto _select(column::definition<Def> const &defn)
      -> todo<selection::applicator<Sel, Def>>;

  template <typename Sel, typename Def>
  auto _select(lazy<selection::node> const &prev,
               column::definition<Def> const &defn)
      -> todo<selection::applicator<Sel, Def>>;

  template <typename Qry, typename... Args>
  auto _make(Args &&...args) -> todo<query::booker<Qry>>;

  template <typename Def, typename... Cols>
  auto _evaluate(todo<column::evaluator<Def>> const &calc,
                 lazy<Cols> const &...columns) -> lazy<Def>;

protected:
  template <typename Kwd> void accept_kwarg(Kwd &&kwarg);

  void analyze();
  void reset();

public:
  template <typename DS, typename Val>
  auto _read(dataset::reader<DS> &ds,
             const std::string &name) -> lazy<read_column_t<DS, Val>>;

  template <typename Sel, typename Col>
  auto _apply(lazy<Col> const &col) -> lazy<selection::node>;

  template <typename Sel, typename Col>
  auto _apply(lazy<selection::node> const &prev,
              lazy<Col> const &col) -> lazy<selection::node>;

  template <typename Sel, typename Def, typename... Cols>
  auto _apply(todo<selection::applicator<Sel, Def>> const &calc,
              lazy<Cols> const &...columns) -> lazy<selection::node>;

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

  template <typename Syst, typename Def>
  void _vary(Syst &syst, const std::string &name,
             column::definition<Def> const &defn);

protected:
  dataset::processor m_processor;  //!
  dataset::weight m_weight;  //!
  long long m_nrows;

  std::vector<std::unique_ptr<dataset::source>> m_sources;  //!

  mutable bool m_analyzed;
};

class dataflow::node {

public:
  friend class dataflow;
  template <typename> friend class varied;

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
  node();
  node(dataflow &df);
  virtual ~node() = default;

protected:
  dataflow *m_df;
};

} // namespace queryosity

#include "dataflow_input.hpp"
#include "dataset_input.hpp"
#include "dataset_player.hpp"

#include "lazy.hpp"
#include "lazy_varied.hpp"
#include "todo.hpp"

#include "column_constant.hpp"
#include "column_expression.hpp"
#include "column_nominal.hpp"
#include "column_variation.hpp"
#include "query_output.hpp"

#include "systematic_resolver.hpp"

inline queryosity::dataflow::dataflow()
    : m_processor(multithread::disable()), m_weight(1.0), m_nrows(-1),
      m_analyzed(false) {}

template <typename Kwd>
queryosity::dataflow::dataflow(Kwd &&kwarg) : dataflow() {
  this->accept_kwarg(std::forward<Kwd>(kwarg));
}

template <typename Kwd1, typename Kwd2>
queryosity::dataflow::dataflow(Kwd1 &&kwarg1, Kwd2 &&kwarg2) : dataflow() {
  static_assert(!std::is_same_v<Kwd1, Kwd2>,
                "each keyword argument must be unique");
  this->accept_kwarg(std::forward<Kwd1>(kwarg1));
  this->accept_kwarg(std::forward<Kwd2>(kwarg2));
}

template <typename Kwd1, typename Kwd2, typename Kwd3>
queryosity::dataflow::dataflow(Kwd1 &&kwarg1, Kwd2 &&kwarg2, Kwd3 &&kwarg3)
    : dataflow() {
  static_assert(!std::is_same_v<Kwd1, Kwd2>,
                "each keyword argument must be unique");
  static_assert(!std::is_same_v<Kwd1, Kwd3>,
                "each keyword argument must be unique");
  static_assert(!std::is_same_v<Kwd2, Kwd3>,
                "each keyword argument must be unique");
  this->accept_kwarg(std::forward<Kwd1>(kwarg1));
  this->accept_kwarg(std::forward<Kwd2>(kwarg2));
  this->accept_kwarg(std::forward<Kwd3>(kwarg3));
}

template <typename Kwd> void queryosity::dataflow::accept_kwarg(Kwd &&kwarg) {
  constexpr bool is_mt_v = std::is_same_v<Kwd, dataset::processor>;
  constexpr bool is_weight_v = std::is_same_v<Kwd, dataset::weight>;
  constexpr bool is_nrows_v = std::is_same_v<Kwd, dataset::head>;
  if constexpr (is_mt_v) {
    m_processor = std::forward<Kwd>(kwarg);
  } else if constexpr (is_weight_v) {
    m_weight = std::forward<Kwd>(kwarg);
  } else if constexpr (is_nrows_v) {
    m_nrows = std::forward<Kwd>(kwarg);
  } else {
    static_assert(is_mt_v || is_weight_v || is_nrows_v,
                  "unrecognized keyword argument");
  }
}

template <typename DS>
auto queryosity::dataflow::load(queryosity::dataset::input<DS> &&in)
    -> queryosity::dataflow::input<DS> {

  auto ds = in.ds.get();

  m_sources.emplace_back(std::move(in.ds));
  m_sources.back()->parallelize(m_processor.concurrency());

  return dataflow::input<DS>(*this, *ds);
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
    -> lazy<column::valued<Val>> {
  return cnst._assign(*this);
}

template <typename Fn>
auto queryosity::dataflow::define(column::expression<Fn> const &expr)
    -> todo<column::evaluator<column::equation_t<Fn>>> {
  return this->_equate(expr);
}

template <typename Def>
auto queryosity::dataflow::define(column::definition<Def> const &defn)
    -> todo<column::evaluator<Def>> {
  return this->_define(defn);
}

template <typename Col>
auto queryosity::dataflow::filter(lazy<Col> const &col)
    -> lazy<selection::node> {
  return this->_apply<selection::cut>(col);
}

template <typename Col>
auto queryosity::dataflow::weight(lazy<Col> const &col)
    -> lazy<selection::node> {
  return this->_apply<selection::weight>(col);
}

template <typename Lzy>
auto queryosity::dataflow::filter(varied<Lzy> const &col) {
  using varied_type = varied<lazy<selection::node>>;
  varied_type syst(this->filter(col.nominal()));
  for (auto const &var_name : col.get_variation_names()) {
    syst.set_variation(var_name, this->filter(col.variation(var_name)));
  }
  return syst;
}

template <typename Lzy>
auto queryosity::dataflow::weight(varied<Lzy> const &col) {
  using varied_type = varied<lazy<selection::node>>;
  varied_type syst(this->weight(col.nominal()));
  for (auto const &var_name : col.get_variation_names()) {
    syst.set_variation(var_name, this->weight(col.variation(var_name)));
  }
  return syst;
}

template <typename Fn, typename... Cols>
auto queryosity::dataflow::filter(column::constant<Fn> const &cnst)
    -> lazy<selection::node> {
  return this->filter(this->define(cnst));
}

template <typename Val>
auto queryosity::dataflow::weight(column::constant<Val> const &cnst)
    -> lazy<selection::node> {
  return this->weight(this->define(cnst));
}

template <typename Fn>
auto queryosity::dataflow::filter(column::expression<Fn> const &expr)
    -> todo<selection::applicator<selection::cut, column::equation_t<Fn>>> {
  return this->_select<selection::cut>(expr);
}

template <typename Fn>
auto queryosity::dataflow::weight(column::expression<Fn> const &expr)
    -> todo<selection::applicator<selection::weight, column::equation_t<Fn>>> {
  return this->_select<selection::weight>(expr);
}

template <typename Def>
auto queryosity::dataflow::filter(column::definition<Def> const &defn)
    -> todo<selection::applicator<selection::cut, Def>> {
  return this->_select<selection::cut>(defn);
}

template <typename Def>
auto queryosity::dataflow::weight(column::definition<Def> const &defn)
    -> todo<selection::applicator<selection::weight, Def>> {
  return this->_select<selection::weight>(defn);
}

template <typename Qry, typename... Args>
auto queryosity::dataflow::_make(Args &&...args) -> todo<query::booker<Qry>> {
  return todo<query::booker<Qry>>(*this, ensemble::invoke(
                                             [&args...](dataset::player *plyr) {
                                               return plyr->make<Qry>(
                                                   std::forward<Args>(args)...);
                                             },
                                             m_processor.get_slots()));
}

template <typename Qry>
auto queryosity::dataflow::get(queryosity::query::output<Qry> const &qry)
    -> todo<query::booker<Qry>> {
  return qry.make(*this);
}

template <typename Col>
auto queryosity::dataflow::get(queryosity::column::series<Col> const &col) {
  return col.make(*this);
}

template <typename... Sels>
auto queryosity::dataflow::get(selection::yield<Sels...> const &sels) {
  return sels.make(*this);
}

template <typename Def, typename... Cols>
auto queryosity::dataflow::_evaluate(todo<column::evaluator<Def>> const &calc,
                                     lazy<Cols> const &...columns)
    -> lazy<Def> {
  auto act = ensemble::invoke(
      [](dataset::player *plyr, column::evaluator<Def> const *calc,
         Cols const *...cols) {
        return plyr->evaluate(*calc, *cols...);
      },
      m_processor.get_slots(), calc.get_slots(), columns.get_slots()...);
  auto lzy = lazy<Def>(*this, act);
  return lzy;
}

template <typename Sel, typename Def, typename... Cols>
auto queryosity::dataflow::_apply(
    todo<selection::applicator<Sel, Def>> const &appl,
    lazy<Cols> const &...columns) -> lazy<selection::node> {
  auto act = ensemble::invoke(
      [](dataset::player *plyr, selection::applicator<Sel, Def> const *appl,
         Cols const *...cols) { return plyr->apply(*appl, *cols...); },
      m_processor.get_slots(), appl.get_slots(), columns.get_slots()...);
  auto lzy = lazy<selection::node>(*this, act);
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
         selection::node const *sel) { return plyr->book(*bkr, *sel); },
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
  m_analyzed = true;

  m_processor.process(m_sources, m_weight, m_nrows);
}

inline void queryosity::dataflow::reset() { m_analyzed = false; }

template <typename Val>
auto queryosity::dataflow::vary(column::constant<Val> const &cnst,
                                std::map<std::string, Val> vars)
    -> varied<lazy<column::valued<Val>>> {
  auto nom = this->define(cnst);
  using varied_type = varied<lazy<column::valued<Val>>>;
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
        &vars) -> varied<todo<column::evaluator<column::equation_t<Fn>>>> {
  auto nom = this->_equate(expr);
  using varied_type = varied<decltype(nom)>;
  using function_type = typename column::expression<Fn>::function_type;
  varied_type syst(std::move(nom));
  for (auto const &var : vars) {
    this->_vary(syst, var.first, column::expression<function_type>(var.second));
  }
  return syst;
}

template <typename Def>
auto queryosity::dataflow::vary(
    column::definition<Def> const &defn,
    std::map<std::string, column::definition<Def>> const &vars)
    -> varied<todo<column::evaluator<Def>>> {
  auto nom = this->_define(defn);
  using varied_type = varied<decltype(nom)>;
  varied_type syst(std::move(nom));
  for (auto const &var : vars) {
    this->_vary(syst, var.first, var.second);
  }
  return syst;
}

template <typename Col>
auto queryosity::dataflow::vary(
    column::nominal<Col> const &nom,
    std::map<std::string, column::variation<column::value_t<Col>>> const &vars)
    -> varied<lazy<column::valued<column::value_t<Col>>>> {
  using varied_type = varied<lazy<column::valued<column::value_t<Col>>>>;
  auto sys = varied_type(std::move(nom.get()));
  for (auto const &var : vars) {
    sys.set_variation(var.first, std::move(var.second.get()));
  }
  return sys;
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
    -> lazy<column::valued<Val>> {
  auto act = ensemble::invoke(
      [&val](dataset::player *plyr) { return plyr->assign<Val>(val); },
      m_processor.get_slots());
  auto lzy = lazy<column::valued<Val>>(*this, act);
  return lzy;
}

template <typename To, typename Col>
auto queryosity::dataflow::_convert(lazy<Col> const &col)
    -> lazy<column::conversion<To, column::value_t<Col>>> {
  auto act = ensemble::invoke(
      [](dataset::player *plyr, Col const *from) {
        return plyr->convert<To>(*from);
      },
      m_processor.get_slots(), col.get_slots());
  auto lzy = lazy<column::conversion<To, column::value_t<Col>>>(*this, act);
  return lzy;
}

template <typename Def>
auto queryosity::dataflow::_define(column::definition<Def> const &defn)
    -> todo<column::evaluator<Def>> {
  return todo<column::evaluator<Def>>(
      *this, ensemble::invoke(
                 [&defn](dataset::player *plyr) { return defn._define(*plyr); },
                 m_processor.get_slots()));
}

template <typename Fn> auto queryosity::dataflow::_equate(Fn fn) {
  return todo<column::evaluator<typename column::equation_t<Fn>>>(
      *this,
      ensemble::invoke(
          [fn](dataset::player *plyr) { return plyr->equate(fn); },
          m_processor.get_slots()));
}

template <typename Sel, typename Fn> auto queryosity::dataflow::_select(Fn fn) {
  return todo<selection::applicator<Sel, typename column::equation_t<Fn>>>(
      *this, ensemble::invoke(
                 [fn](dataset::player *plyr) {
                   return plyr->select<Sel>(nullptr, fn);
                 },
                 m_processor.get_slots()));
}

template <typename Fn>
auto queryosity::dataflow::_equate(column::expression<Fn> const &expr)
    -> todo<column::evaluator<column::equation_t<Fn>>> {
  return expr._equate(*this);
}

template <typename Sel, typename Fn>
auto queryosity::dataflow::_select(lazy<selection::node> const &prev, Fn fn) {
  return todo<selection::applicator<Sel, typename column::equation_t<Fn>>>(
      *this, ensemble::invoke(
                 [fn](dataset::player *plyr, selection::node const *prev) {
                   return plyr->select<Sel>(prev, fn);
                 },
                 m_processor.get_slots(), prev.get_slots()));
}

template <typename Sel, typename Fn>
auto queryosity::dataflow::_select(column::expression<Fn> const &expr)
    -> todo<selection::applicator<Sel, column::equation_t<Fn>>> {
  return expr.template _select<Sel>(*this);
}

template <typename Sel, typename Fn>
auto queryosity::dataflow::_select(lazy<selection::node> const &prev,
                                   column::expression<Fn> const &expr)
    -> todo<selection::applicator<Sel, column::equation_t<Fn>>> {
  return expr.template _select<Sel>(*this, prev);
}

template <typename Sel, typename Def>
auto queryosity::dataflow::_select(column::definition<Def> const &defn)
    -> todo<selection::applicator<Sel, Def>> {
  return todo<selection::applicator<Sel, Def>>(
      *this, ensemble::invoke(
                 [&defn](dataset::player *plyr) {
                   return defn.template _select<Sel>(*plyr);
                 },
                 m_processor.get_slots()));
}

template <typename Sel, typename Def>
auto queryosity::dataflow::_select(lazy<selection::node> const &prev,
                                   column::definition<Def> const &defn)
    -> todo<selection::applicator<Sel, Def>> {
  return todo<selection::applicator<Sel, Def>>(
      *this, ensemble::invoke(
                 [&defn](dataset::player *plyr, selection::node const *prev) {
                   return defn.template _select<Sel>(*plyr, *prev);
                 },
                 m_processor.get_slots(), prev.get_slots()));
}

template <typename Sel, typename Col>
auto queryosity::dataflow::_apply(lazy<Col> const &dec)
    -> lazy<selection::node> {
  auto act = ensemble::invoke(
      [](dataset::player *plyr, Col *col) {
        return plyr->apply<Sel>(nullptr, *col);
      },
      m_processor.get_slots(), dec.get_slots());
  auto lzy = lazy<selection::node>(*this, act);
  return lzy;
}

template <typename Sel, typename Col>
auto queryosity::dataflow::_apply(lazy<selection::node> const &prev,
                                  lazy<Col> const &dec)
    -> lazy<selection::node> {
  auto act = ensemble::invoke(
      [](dataset::player *plyr, selection::node const *prev, Col *col) {
        return plyr->apply<Sel>(prev, *col);
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

template <typename Syst, typename Def>
void queryosity::dataflow::_vary(Syst &syst, const std::string &name,
                                 column::definition<Def> const &defn) {
  syst.set_variation(name, this->_define(defn));
}

inline queryosity::dataflow::node::node() : m_df(nullptr) {}

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