#pragma once

#include "dataflow.h"
#include "dataflow_lazy.h"
#include "dataflow_systematic.h"
#include "multithread.h"

namespace ana {

template <typename DS>
template <typename Bld>
class dataflow<DS>::delayed
    : public dataflow<DS>::template systematic<delayed<Bld>>,
      public concurrent<Bld> {

public:
  class varied;

public:
  delayed(dataflow<DS> &dataflow, concurrent<Bld> &&action)
      : systematic<delayed<Bld>>::systematic(dataflow),
        concurrent<Bld>::concurrent(std::move(action)) {}

  virtual ~delayed() = default;

  template <typename V>
  delayed(delayed<V> &&other)
      : systematic<delayed<Bld>>::systematic(*other.m_df),
        concurrent<Bld>::concurrent(std::move(other)) {}

  template <typename V> delayed &operator=(delayed<V> &&other) {
    this->m_df = other.m_df;
    concurrent<Bld>::operator=(std::move(other));
    return *this;
  }

  virtual void set_variation(const std::string &var_name,
                             delayed &&var) override;

  virtual delayed const &get_nominal() const override;
  virtual delayed const &
  get_variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> list_variation_names() const override;

  /**
   * @brief Apply a systematic variation to an `equation` column.
   * @param var_name Name of the systematic variation.
   * @param args... Constructor arguments for `definition`.
   * @return Varied definition.
   * @details Creates a `varied` action whose `.get_nominal()` is the original
   * lazy one, and `get_variation(var_name)` is the newly-constructed one.
   */
  template <typename... Args, typename V = Bld,
            std::enable_if_t<ana::column::template is_evaluator_v<V> &&
                                 !ana::column::template is_equation_v<
                                     ana::column::template evaluated_t<V>>,
                             bool> = false>
  auto vary(const std::string &var_name, Args &&...args) ->
      typename delayed<V>::varied;

  /**
   * @brief Apply a systematic variation to `equation` column.
   * @param var_name Name of the systematic variation.
   * @param callable C++ function, lambda expression, or any other callable.
   * **Note**: the function return type and signature must be convertible to the
   * original's.
   * @return Varied equation.
   * @details Creates a `varied` action whose `.get_nominal()` is the original
   * lazy one, and `get_variation(var_name)` is the newly-constructed one.
   */
  template <typename F, typename V = Bld,
            std::enable_if_t<ana::column::template is_evaluator_v<V> &&
                                 ana::column::template is_equation_v<
                                     ana::column::template evaluated_t<V>>,
                             bool> = false>
  auto vary(const std::string &var_name, F callable) ->
      typename delayed<V>::varied;

  /**
   * @brief Evaluate the column out of existing ones.
   * @param columns Input columns.
   * @return Evaluated column.
   */
  template <
      typename... Nodes, typename V = Bld,
      std::enable_if_t<ana::column::template is_evaluator_v<V>, bool> = false>
  auto evaluate(Nodes &&...columns) const
      -> decltype(std::declval<delayed<V>>().evaluate_column(
          std::forward<Nodes>(columns)...)) {
    return this->evaluate_column(std::forward<Nodes>(columns)...);
  }

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<
                ana::column::template is_evaluator_v<V> &&
                    ana::dataflow<DS>::template has_no_variation_v<Nodes...>,
                bool> = false>
  auto evaluate_column(Nodes const &...columns) const
      -> lazy<column::template evaluated_t<V>> {
    // nominal
    return this->m_df->evaluate_column(*this, columns...);
  }

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<
                ana::column::template is_evaluator_v<V> &&
                    ana::dataflow<DS>::template has_variation_v<Nodes...>,
                bool> = false>
  auto evaluate_column(Nodes const &...columns) const ->
      typename lazy<column::template evaluated_t<V>>::varied {

    using syst_type = typename lazy<column::template evaluated_t<V>>::varied;

    auto nom = this->m_df->evaluate_column(*this, columns.get_nominal()...);
    auto syst = syst_type(std::move(nom));

    for (auto const &var_name : list_all_variation_names(columns...)) {
      auto var = this->m_df->evaluate_column(
          *this, columns.get_variation(var_name)...);
      syst.set_variation(var_name, std::move(var));
    }

    return syst;
  }

  /**
   * @brief Apply the selection's expression based on input columns.
   * @param columns Input columns.
   * @return `lazy/varied<selection>` Applied selection.
   */
  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<ana::selection::template is_applicator_v<V>,
                             bool> = false>
  auto apply(Nodes &&...columns) const
      -> decltype(std::declval<delayed<V>>().apply_selection(
          std::forward<Nodes>(columns)...)) {
    return this->apply_selection(std::forward<Nodes>(columns)...);
  }

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<
                selection::template is_applicator_v<V> &&
                    ana::dataflow<DS>::template has_no_variation_v<Nodes...>,
                bool> = false>
  auto apply_selection(Nodes const &...columns) const -> lazy<selection> {
    // nominal
    return this->m_df->apply_selection(*this, columns...);
  }

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<
                selection::template is_applicator_v<V> &&
                    ana::dataflow<DS>::template has_variation_v<Nodes...>,
                bool> = false>
  auto apply_selection(Nodes const &...columns) const ->
      typename lazy<selection>::varied {
    // variations
    using syst_type = typename lazy<selection>::varied;
    auto syst = syst_type(
        this->get_nominal().apply_selection(columns.get_nominal()...));
    auto var_names = list_all_variation_names(columns...);
    for (auto const &var_name : var_names) {
      syst.set_variation(var_name,
                         this->get_variation(var_name).apply_selection(
                             columns.get_variation(var_name)...));
    }
    return syst;
  }

  /**
   * @brief Fill the counter with input columns.
   * @param columns Input columns
   * @return The counter to be filled with the input columns.
   */
  template <
      typename... Nodes, typename V = Bld,
      std::enable_if_t<ana::counter::template is_booker_v<V>, bool> = false>
  auto fill(Nodes &&...columns) const
      -> decltype(std::declval<delayed<V>>().fill_counter(
          std::declval<Nodes>()...)) {
    return this->fill_counter(std::forward<Nodes>(columns)...);
  }

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<
                ana::counter::template is_booker_v<V> &&
                    ana::dataflow<DS>::template has_no_variation_v<Nodes...>,
                bool> = false>
  auto fill_counter(Nodes const &...columns) const -> delayed<V> {
    // nominal
    return delayed<V>(
        *this->m_df, this->get_concurrent_result(
                         [](V &fillable, typename Nodes::action_type &...cols) {
                           return fillable.book_fill(cols...);
                         },
                         columns...));
  }

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<ana::counter::template is_booker_v<V> &&
                                 has_variation_v<Nodes...>,
                             bool> = false>
  auto fill_counter(Nodes const &...columns) const -> varied {
    auto syst = varied(std::move(this->fill_counter(columns.get_nominal())...));
    for (auto const &var_name : list_all_variation_names(columns...)) {
      syst.set_variation(var_name, this->fill_counter(std::move(
                                       columns.get_variation(var_name))...));
    }
    return syst;
  }

  /**
   * @brief Book the counter at a selection.
   * @param selection Selection to be counted.
   * @return The counter booked at the selection.
   */
  template <typename Node> auto at(Node &&selection) const {
    return this->select_counter(std::forward<Node>(selection));
  }

  template <typename Node, typename V = Bld,
            std::enable_if_t<ana::counter::template is_booker_v<V> &&
                                 ana::dataflow<DS>::template is_nominal_v<Node>,
                             bool> = false>
  auto select_counter(Node const &sel) const -> lazy<counter::booked_t<V>> {
    // nominal
    return this->m_df->select_counter(*this, sel);
  }

  template <typename Node, typename V = Bld,
            std::enable_if_t<ana::counter::template is_booker_v<V> &&
                                 ana::dataflow<DS>::template is_varied_v<Node>,
                             bool> = false>
  auto select_counter(Node const &sel) const ->
      typename lazy<counter::booked_t<V>>::varied {
    using syst_type = typename lazy<counter::booked_t<V>>::varied;
    auto syst = syst_type(this->m_df->select_counter(*this, sel.get_nominal()));
    for (auto const &var_name : list_all_variation_names(sel)) {
      syst.set_variation(var_name, this->m_df->select_counter(
                                       *this, sel.get_variation(var_name)));
    }
    return syst;
  }

  /**
   * @brief Book multiple counters, one at each selection.
   * @param selections The selections.
   * @return The counters booked at selections.
   */
  template <typename... Nodes> auto at(Nodes &&...nodes) const {
    static_assert(counter::template is_booker_v<Bld>, "not a counter (booker)");
    return this->select_counters(std::forward<Nodes>(nodes)...);
  }

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<
                ana::counter::template is_booker_v<V> &&
                    ana::dataflow<DS>::template has_no_variation_v<Nodes...>,
                bool> = false>
  auto select_counters(Nodes const &...sels) const
      -> delayed<counter::bookkeeper<counter::booked_t<V>>> {
    // nominal
    return this->m_df->select_counters(*this, sels...);
  }

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<ana::counter::template is_booker_v<V> &&
                                 has_variation_v<Nodes...>,
                             bool> = false>
  auto select_counters(Nodes const &...sels) const ->
      typename delayed<counter::bookkeeper<counter::booked_t<V>>>::varied {
    // variations
    using syst_type =
        typename delayed<counter::bookkeeper<counter::booked_t<V>>>::varied;
    auto syst =
        syst_type(this->m_df->select_counters(*this, sels.get_nominal()...));
    for (auto const &var_name : list_all_variation_names(sels...)) {
      syst.set_variation(var_name, this->m_df->select_counters(
                                       *this, sels.get_variation(var_name)...));
    }
    return syst;
  }

  /**
   * @return The list of booked selection paths.
   */
  template <
      typename V = Bld,
      std::enable_if_t<ana::counter::template is_bookkeeper_v<V>, bool> = false>
  auto list_selection_paths() const -> std::set<std::string> {
    return this->get_model_value(
        [](Bld const &bkpr) { return bkpr.list_selection_paths(); });
  }

  /**
   * @brief Get the counter booked at a selection path.
   * @param selection_path Path of the selection
   * @return counter The counter booked at the selection path.
   */
  template <
      typename V = Bld,
      std::enable_if_t<ana::counter::template is_bookkeeper_v<V>, bool> = false>
  auto get_counter(const std::string &sel_path) const
      -> lazy<counter::booked_t<V>> {
    return lazy<counter::booked_t<V>>(
        *this->m_df, this->get_lockstep_view([sel_path = sel_path](V &bkpr) {
          return bkpr.get_counter(sel_path);
        }));
  }

  /**
   * @brief Retrieve the result of a counter.
   * @details Triggers processing of the dataset if that the result is not
   * already available.
   * @return The result of the implemented counter.
   */
  template <
      typename V = Bld,
      std::enable_if_t<ana::counter::template has_output_v<V>, bool> = false>
  auto get_result() const -> decltype(std::declval<V>().get_result()) {
    this->m_df->analyze();
    this->merge_results();
    return this->get_model()->get_result();
  }

  /**
   * @brief Evaluate/apply a column/selection, respectively.
   * @param columns The input columns.
   * @details A chained function call is equivalent to `evaluate` and `apply`
   * for column and selection respectively.
   * @return The evaluated/applied column/selection.
   */
  template <typename... Args, typename V = Bld,
            std::enable_if_t<column::template is_evaluator_v<V> ||
                                 selection::template is_applicator_v<V>,
                             bool> = false>
  auto operator()(Args &&...columns) const
      -> decltype(std::declval<delayed<V>>().evaluate_or_apply(
          std::forward<Args>(std::declval<Args &&>())...)) {
    return this->evaluate_or_apply(std::forward<Args>(columns)...);
  }

  template <typename... Args, typename V = Bld,
            std::enable_if_t<column::template is_evaluator_v<V>, bool> = false>
  auto evaluate_or_apply(Args &&...columns) const
      -> decltype(std::declval<delayed<V>>().evaluate(
          std::forward<Args>(std::declval<Args &&>())...)) {
    return this->evaluate(std::forward<Args>(columns)...);
  }

  template <
      typename... Args, typename V = Bld,
      std::enable_if_t<selection::template is_applicator_v<V>, bool> = false>
  auto evaluate_or_apply(Args &&...columns) const
      -> decltype(std::declval<delayed<V>>().apply(
          std::forward<Args>(std::declval<Args &&>())...)) {
    return this->apply(std::forward<Args>(columns)...);
  }

  template <
      typename... Args, typename V = Bld,
      std::enable_if_t<counter::template is_bookkeeper_v<V>, bool> = false>
  auto operator[](const std::string &sel_path) const
      -> lazy<counter::booked_t<V>> {
    return this->get_counter(sel_path);
  }
};

} // namespace ana

template <typename DS>
template <typename Bld>
void ana::dataflow<DS>::delayed<Bld>::set_variation(const std::string &,
                                                    delayed<Bld> &&) {
  // should never be called
  throw std::logic_error("cannot set variation to a lazy action");
}

template <typename DS>
template <typename Bld>
auto ana::dataflow<DS>::delayed<Bld>::get_nominal() const -> delayed const & {
  // this is nominal
  return *this;
}

template <typename DS>
template <typename Bld>
auto ana::dataflow<DS>::delayed<Bld>::get_variation(const std::string &) const
    -> delayed const & {
  // propagation of variations must occur "transparently"
  return *this;
}

template <typename DS>
template <typename Bld>
std::set<std::string>
ana::dataflow<DS>::delayed<Bld>::list_variation_names() const {
  // no variations to list
  return std::set<std::string>();
}

template <typename DS>
template <typename Bld>
bool ana::dataflow<DS>::delayed<Bld>::has_variation(const std::string &) const {
  // always false
  return false;
}

template <typename DS>
template <typename Bld>
template <typename F, typename V,
          std::enable_if_t<ana::column::template is_evaluator_v<V> &&
                               ana::column::template is_equation_v<
                                   ana::column::template evaluated_t<V>>,
                           bool>>
auto ana::dataflow<DS>::delayed<Bld>::vary(const std::string &var_name,
                                           F callable) ->
    typename delayed<V>::varied {

  auto syst = varied(std::move(*this));
  syst.set_variation(var_name, std::move(syst.m_df->vary_equation(
                                   syst.get_nominal(), callable)));

  return std::move(syst);
}

template <typename DS>
template <typename Bld>
template <typename... Args, typename V,
          std::enable_if_t<ana::column::template is_evaluator_v<V> &&
                               !ana::column::template is_equation_v<
                                   ana::column::template evaluated_t<V>>,
                           bool>>
auto ana::dataflow<DS>::delayed<Bld>::vary(const std::string &var_name,
                                           Args &&...args) ->
    typename delayed<V>::varied {
  auto syst = varied(std::move(*this));
  syst.set_variation(var_name,
                     std::move(syst.m_df->vary_definition(
                         syst.get_nominal(), std::forward<Args>(args)...)));
}