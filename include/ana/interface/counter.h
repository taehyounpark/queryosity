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

  template <typename T> class output;

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

  virtual void initialize(const dataset::range &part) override;
  virtual void execute(const dataset::range &part,
                       unsigned long long entry) override;

  virtual void count(double w) = 0;

protected:
  bool m_raw;
  double m_scale;
  const selection *m_selection;

public:
  template <typename T>
  static constexpr std::true_type check_implemented(const counter::output<T> &);
  static constexpr std::false_type check_implemented(...);

  template <typename Out, typename... Vals>
  static constexpr std::true_type
  check_fillable(const typename counter::logic<Out(Vals...)> &);
  static constexpr std::false_type check_fillable(...);

  template <typename T> struct is_booker : std::false_type {};
  template <typename T>
  struct is_booker<counter::booker<T>> : std::true_type {};

  template <typename T>
  static constexpr bool has_output_v =
      decltype(check_implemented(std::declval<T>()))::value;

  template <typename T>
  static constexpr bool is_fillable_v =
      decltype(check_fillable(std::declval<T>()))::value;

  template <typename T> static constexpr bool is_booker_v = is_booker<T>::value;

  template <typename Bkr> using counter_t = typename Bkr::counter_type;
};

/**
 * @brief Minimal counter with an output result.
 * @details This ABC should be used for counting operations that do not require
 * any input columns, e.g. a cutflow of selections.
 */
template <typename T> class counter::output : public counter {

public:
  using result_type = T;

public:
  output();
  virtual ~output() = default;

  /**
   * @brief Create and return the result of the counter.
   * @return The result.
   * @detail The output from each concurrent slot, which is returned by value,
   * are collected into a list to be merged into one.
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

  virtual void finalize(const dataset::range &) final override;

  T const &get_result() const;

  T const &operator->() const { return this->get_result(); }

  bool is_merged() const;
  void merge_results(std::vector<T> results);

protected:
  void set_merged(bool merged = true);

protected:
  T m_result;
  bool m_merged;
};

/**
 * @brief Counter output to be filled with columns using arbitrary logic.
 * @tparam T Output result type.
 * @tparam Obs... Input column data types.
 */
template <typename T, typename... Obs>
class counter::logic<T(Obs...)> : public counter::output<T> {

public:
  using vartup_type = std::tuple<ana::variable<Obs>...>;

public:
  logic() = default;
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
  virtual void fill(ana::observable<Obs>... observables, double w) = 0;
  virtual void count(double w) final override;

  template <typename... Vals> void enter_columns(term<Vals> const &...cols);

protected:
  std::vector<vartup_type> m_fills;
};

template <typename T> class counter::booker {

public:
  using counter_type = T;

public:
  booker() = default;
  template <typename... Args> booker(Args... args);
  ~booker() = default;

  template <typename... Vals>
  auto book_fill(term<Vals> const &...cols) const -> std::unique_ptr<booker<T>>;

  auto select_counter(const selection &sel) const -> std::unique_ptr<T>;
  template <typename... Sels>
  auto select_counters(Sels const &...sels) const -> std::unique_ptr<booker<T>>;

  std::set<std::string> list_selection_paths() const;
  T *get_counter(const std::string &path) const;

protected:
  std::unique_ptr<T> make_counter();
  template <typename... Vals> void fill_counter(term<Vals> const &...cols);
  void book_selection(std::unique_ptr<T> cnt, const selection &sel);
  auto fresh() const -> std::unique_ptr<booker<T>>;

protected:
  std::function<std::unique_ptr<T>()> m_make_counter_call;
  std::vector<std::function<void(T &)>> m_fill_counter_calls;
  std::set<std::string> m_booked_selection_paths;
  std::unordered_map<std::string, std::unique_ptr<T>> m_booked_counter_map;
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

inline void ana::counter::initialize(const ana::dataset::range &part) {
  if (!m_selection)
    throw std::runtime_error("no booked selection");
}

inline void ana::counter::execute(const ana::dataset::range &,
                                  unsigned long long) {
  if (m_selection->passed_cut())
    this->count(m_raw ? 1.0 : m_scale * m_selection->get_weight());
}

template <typename T> ana::counter::output<T>::output() : m_merged(false) {}

template <typename T> bool ana::counter::output<T>::is_merged() const {
  return m_merged;
}

template <typename T> void ana::counter::output<T>::set_merged(bool merged) {
  m_merged = merged;
}

template <typename T>
void ana::counter::output<T>::finalize(const ana::dataset::range &) {
  m_result = this->result();
}

template <typename T> T const &ana::counter::output<T>::get_result() const {
  return m_result;
}

template <typename T>
void ana::counter::output<T>::merge_results(std::vector<T> results) {
  if (!results.size()) {
    throw std::logic_error("merging requires at least one result");
  }
  m_result = this->merge(results);
  this->set_merged(true);
}

template <typename T, typename... Obs>
template <typename... Vals>
void ana::counter::logic<T(Obs...)>::enter_columns(term<Vals> const &...cols) {
  static_assert(sizeof...(Obs) == sizeof...(Vals),
                "dimension mis-match between filled variables & columns.");
  m_fills.emplace_back(cols...);
}

template <typename T, typename... Obs>
void ana::counter::logic<T(Obs...)>::count(double w) {
  for (unsigned int ifill = 0; ifill < m_fills.size(); ++ifill) {
    std::apply(
        [this, w](const variable<Obs> &...obs) { this->fill(obs..., w); },
        m_fills[ifill]);
  }
}

template <typename T>
template <typename... Args>
ana::counter::booker<T>::booker(Args... args)
    : m_make_counter_call(std::bind(
          [](Args... args) { return std::make_unique<T>(args...); }, args...)) {
}

template <typename T>
template <typename... Vals>
auto ana::counter::booker<T>::book_fill(term<Vals> const &...columns) const
    -> std::unique_ptr<booker<T>> {
  // use a fresh one with its current fills
  auto filled = this->fresh();
  // add fills
  filled->fill_counter(columns...);
  // return new booker
  return std::move(filled);
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
auto ana::counter::booker<T>::select_counter(const selection &sel) const
    -> std::unique_ptr<T> {
  // call constructor
  auto cnt = m_make_counter_call();
  // fill columns (if set)
  for (const auto &fill_counter_call : m_fill_counter_calls) {
    fill_counter_call(*cnt);
  }
  // book cnt at the selection
  cnt->set_selection(sel);
  // return
  return std::move(cnt);
}

template <typename T>
template <typename... Sels>
auto ana::counter::booker<T>::select_counters(const Sels &...sels) const
    -> std::unique_ptr<booker<T>> {
  // make a fresh one with fill calls, but no selections
  auto selected = this->fresh();
  // book selections
  (selected->book_selection(selected->select_counter(sels), sels), ...);
  // return a new booker with the selections added
  return std::move(selected);
}

template <typename T>
void ana::counter::booker<T>::book_selection(std::unique_ptr<T> cnt,
                                             const selection &sel) {
  // check if booking makes sense
  if (m_booked_counter_map.find(sel.get_path()) != m_booked_counter_map.end()) {
    throw std::logic_error("counter already booked at selection");
  }
  m_booked_selection_paths.insert(sel.get_path());
  m_booked_counter_map.insert(std::make_pair(sel.get_path(), std::move(cnt)));
}

template <typename T>
std::set<std::string> ana::counter::booker<T>::list_selection_paths() const {
  return m_booked_selection_paths;
}

template <typename T>
T *ana::counter::booker<T>::get_counter(const std::string &sel_path) const {
  if (m_booked_counter_map.find(sel_path) == m_booked_counter_map.end()) {
    throw std::out_of_range("counter not booked at selection path");
  }
  return m_booked_counter_map.at(sel_path).get();
}

template <typename T>
auto ana::counter::booker<T>::fresh() const -> std::unique_ptr<booker<T>> {
  auto out = std::make_unique<booker<T>>();
  out->m_make_counter_call = this->m_make_counter_call;
  out->m_fill_counter_calls = this->m_fill_counter_calls;
  return std::move(out);
}
