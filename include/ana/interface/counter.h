#pragma once

#include <functional>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "action.h"

namespace ana {

template <typename T> class term;

template <typename T> class cell;

template <typename T> class variable;

template <typename T> class observable;

class selection;

class counter : public action {

public:
  class experiment;

  template <typename T> class implementation;

  template <typename T> class logic;

  template <typename T> class booker;

  template <typename T> class summary;

public:
  counter();
  virtual ~counter() = default;

  void apply_scale(double scale);
  void use_weight(bool use = true);

  void set_selection(const selection &selection);
  const selection *get_selection() const;

  virtual void initialize() override;
  virtual void execute() override;

  virtual void count(double w) = 0;

protected:
  bool m_raw;
  double m_scale;
  const selection *m_selection;

public:
  template <typename T>
  static constexpr std::true_type
  check_implemented(const counter::implementation<T> &);
  static constexpr std::false_type check_implemented(...);

  template <typename Out, typename... Vals>
  static constexpr std::true_type check_fillable(
      const typename counter::implementation<Out>::template fillable<Vals...>
          &);
  static constexpr std::false_type check_fillable(...);

  template <typename T> struct is_booker : std::false_type {};
  template <typename T>
  struct is_booker<counter::booker<T>> : std::true_type {};

  template <typename T>
  static constexpr bool is_implemented_v =
      decltype(check_implemented(std::declval<T>()))::value;

  template <typename T>
  static constexpr bool is_fillable_v =
      decltype(check_fillable(std::declval<T>()))::value;

  template <typename T> static constexpr bool is_booker_v = is_booker<T>::value;
};

/**
 * @brief ABC of a minimal counter with an output result.
 * @details This ABC should be used for counting operations that do not require
 * any input columns, e.g. a cutflow of selections.
 */
template <typename T> class counter::implementation : public counter {

public:
  template <typename... Obs> class fillable;

  using result_type = T;

public:
  implementation();
  virtual ~implementation() = default;

  /**
   * @brief Create the result of the counter.
   * @return The result.
   */
  virtual T result() const = 0;

  /**
   * @brief Merge the results from concurrent slots into one.
   * @param results Incoming results.
   * @return The merged result.
   */
  virtual T merge(std::vector<T> results) const = 0;

  /**
   * @brief Count an entry for which the booked selection has passed with its
   * weight.
   * @param weight The value of the weight at booked selection for the passed
   * entry.
   */
  using counter::count;

  /**
   * @details Set the result of the counter.
   */
  virtual void finalize() override;

  /**
   * @brief Get the result of the counter
   * @return The result of the counter.
   * @detail The result is returned by `const &` to avoid unncessary copies, but
   * also prevent post-processing modifications. Should the analyzer wish for
   * the latter, the result should be copied by value.
   */
  T const &get_result() const;

  /**
   * @brief Get the result of the counter
   * @return The result of the counter.
   * @detail Shorthand indirection operator to access the result's `const`
   * methods.
   */
  T const &operator->() const { return this->get_result(); }

  bool is_merged() const;
  void merge_results(std::vector<T> results);

protected:
  void set_merged(bool merged = true);

protected:
  T m_result;
  bool m_merged;
};

template <typename T>
template <typename... Obs>
class counter::implementation<T>::fillable : public counter::implementation<T> {

public:
  using obstup_type = std::tuple<ana::variable<Obs>...>;

public:
  fillable();
  virtual ~fillable() = default;

  template <typename... Vals> void enter_columns(term<Vals> const &...cols);

  virtual void count(double w) override;
  virtual void fill(ana::observable<Obs>... observables, double w) = 0;

protected:
  std::vector<obstup_type> m_fills;
};

/**
 * @brief ABC of a counter to be filled with columns.
 * @details Analyzers should inherit from this ABC to implement the most general
 * arbitrary logic of a counting operation.
 */
template <typename T, typename... Obs>
class counter::logic<T(Obs...)>
    : public counter::implementation<T>::template fillable<Obs...> {

public:
  logic();
  virtual ~logic() = default;

  /**
   * @brief Perform the counting operation for an entry.
   * @param observables The `observable` of each input column.
   * @param weight The weight value of the booked selection for the passed
   * entry.
   * @details This operation is performed N times for a passed entry, where N is
   * the number of `fill` calls made to its `lazy` action, each with its the set
   * of input columns as provided then.
   */
  using counter::implementation<T>::template fillable<Obs...>::fill;
};

template <typename T> class counter::booker {

public:
  using counter_type = T;

public:
  template <typename... Args> booker(Args... args);

  // use default copy-constructor
  booker(booker const &) = default;
  booker &operator=(booker const &) = default;

  ~booker() = default;

  template <typename... Vals>
  auto book_fill(term<Vals> const &...cols) const -> std::shared_ptr<booker<T>>;

  auto book_selection(const selection &sel) const -> std::shared_ptr<T>;
  template <typename... Sels>
  auto book_selections(Sels const &...sels) const -> std::shared_ptr<booker<T>>;

  std::set<std::string> list_selection_paths() const;
  std::shared_ptr<T> get_counter(const std::string &path) const;

protected:
  template <typename... Vals> void fill_counter(term<Vals> const &...cols);
  void make_counter(const selection &sel);

protected:
  std::function<std::shared_ptr<T>()> m_make_counter_call;
  std::vector<std::function<void(T &)>> m_fill_counter_calls;
  std::set<std::string> m_booked_selection_paths;
  std::unordered_map<std::string, std::shared_ptr<T>> m_booked_counter_map;
};

template <typename T> class counter::summary {

public:
  // version for lazy<counter>
  template <typename Res>
  void record(const std::string &selection_path,
              std::decay_t<Res> counter_result) {
    static_cast<T *>(this)->record(selection_path, counter_result);
  }

  // version for varied<counter>
  template <typename Res>
  void record(const std::string &variation_name,
              const std::string &selection_path,
              std::decay_t<Res> counter_result) {
    static_cast<T *>(this)->record(variation_name, selection_path,
                                   counter_result);
  }

  template <typename Dest> void output(Dest &destination) {
    static_cast<T *>(this)->output(destination);
  }
};

} // namespace ana

#include "column.h"
#include "selection.h"

inline ana::counter::counter()
    : m_selection(nullptr), m_scale(1.0), m_raw(false) {}

inline void ana::counter::set_selection(const selection &selection) {
  m_selection = &selection;
}

inline const ana::selection *ana::counter::get_selection() const {
  return m_selection;
}

inline void ana::counter::apply_scale(double scale) { m_scale *= scale; }

inline void ana::counter::use_weight(bool use) { m_raw = !use; }

inline void ana::counter::initialize() {
  if (!m_selection)
    throw std::runtime_error("no booked selection");
}

inline void ana::counter::execute() {
  if (m_selection->passed_cut())
    this->count(m_raw ? 1.0 : m_scale * m_selection->get_weight());
}

template <typename T>
ana::counter::implementation<T>::implementation()
    : counter(), m_merged(false) {}

template <typename T> bool ana::counter::implementation<T>::is_merged() const {
  return m_merged;
}

template <typename T>
void ana::counter::implementation<T>::set_merged(bool merged) {
  m_merged = merged;
}

template <typename T> void ana::counter::implementation<T>::finalize() {
  m_result = this->result();
}

template <typename T>
T const &ana::counter::implementation<T>::get_result() const {
  return m_result;
}

template <typename T>
void ana::counter::implementation<T>::merge_results(std::vector<T> results) {
  if (!results.size()) {
    throw std::logic_error("merging requires at least one result");
  }
  m_result = this->merge(results);
  this->set_merged(true);
}

template <typename T>
template <typename... Obs>
ana::counter::implementation<T>::fillable<Obs...>::fillable()
    : counter::implementation<T>() {}

template <typename T>
template <typename... Obs>
template <typename... Vals>
void ana::counter::implementation<T>::fillable<Obs...>::enter_columns(
    term<Vals> const &...cols) {
  static_assert(sizeof...(Obs) == sizeof...(Vals),
                "dimension mis-match between filled variables & columns.");
  m_fills.emplace_back(cols...);
}

template <typename T>
template <typename... Obs>
void ana::counter::implementation<T>::fillable<Obs...>::count(double w) {
  for (unsigned int ifill = 0; ifill < m_fills.size(); ++ifill) {
    std::apply(
        [this, w](const variable<Obs> &...obs) { this->fill(obs..., w); },
        m_fills[ifill]);
  }
}

template <typename T, typename... Obs>
ana::counter::logic<T(Obs...)>::logic()
    : counter::implementation<T>::template fillable<Obs...>() {}

template <typename T>
template <typename... Args>
ana::counter::booker<T>::booker(Args... args)
    : m_make_counter_call(std::bind(
          [](Args... args) { return std::make_shared<T>(args...); }, args...)) {
}

template <typename T>
template <typename... Vals>
auto ana::counter::booker<T>::book_fill(term<Vals> const &...columns) const
    -> std::shared_ptr<booker<T>> {
  // use a snapshot of its current calls
  auto filled = std::make_shared<booker<T>>(*this);
  filled->fill_counter(columns...);
  return filled;
}

template <typename T>
template <typename... Vals>
void ana::counter::booker<T>::fill_counter(term<Vals> const &...columns) {
  // use a snapshot of its current calls
  m_fill_counter_calls.push_back(std::bind(
      [](T &cnt, term<Vals> const &...cols) { cnt.enter_columns(cols...); },
      std::placeholders::_1, std::ref(columns)...));
}

template <typename T>
template <typename... Sels>
auto ana::counter::booker<T>::book_selections(const Sels &...sels) const
    -> std::shared_ptr<booker<T>> {
  // use a snapshot of its current calls
  auto counted = std::make_shared<booker<T>>(*this);
  (counted->make_counter(sels), ...);
  // return a new booker with the selections added
  return counted;
}

template <typename T>
auto ana::counter::booker<T>::book_selection(const selection &sel) const
    -> std::shared_ptr<T> {
  // use a snapshot of its current calls
  auto counted = std::make_shared<booker<T>>(*this);
  counted->make_counter(sel);
  // return a new booker with the selection added
  return counted->get_counter(sel.get_path());
}

template <typename T>
void ana::counter::booker<T>::make_counter(const selection &sel) {
  // check if booking makes sense
  if (m_booked_counter_map.find(sel.get_path()) != m_booked_counter_map.end()) {
    throw std::logic_error("counter already booked at selection");
  }

  // call constructor
  auto cnt = m_make_counter_call();

  // map selection path & counter
  m_booked_selection_paths.insert(sel.get_path());
  m_booked_counter_map.insert(std::make_pair(sel.get_path(), cnt));

  // fill columns (if set)
  for (const auto &fill_counter_call : m_fill_counter_calls) {
    fill_counter_call(*cnt);
  }

  // book cnt at the selection
  cnt->set_selection(sel);
}

template <typename T>
std::set<std::string> ana::counter::booker<T>::list_selection_paths() const {
  return m_booked_selection_paths;
}

template <typename T>
std::shared_ptr<T>
ana::counter::booker<T>::get_counter(const std::string &sel_path) const {
  if (m_booked_counter_map.find(sel_path) == m_booked_counter_map.end()) {
    throw std::out_of_range("counter not booked at selection path");
  }
  return m_booked_counter_map.at(sel_path);
}