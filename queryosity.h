#pragma once

/**
 * @defgroup ext Extensions
 * @defgroup api API
 * @defgroup abc ABCs
 */

#include <algorithm>
#include <cassert>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>
#include <type_traits>
#include <vector>

namespace queryosity {

namespace ensemble {

template <typename T> class slotted {
public:
  slotted() = default;
  virtual ~slotted() = default;

public:
  virtual std::vector<T *> const &get_slots() const = 0;
  T *get_slot(unsigned int i) const;

  operator std::vector<T *> const &() const { return this->get_slots(); }

  unsigned int size() const;
};

template <typename T, typename... Args>
unsigned int check(std::vector<T> const &first,
                   std::vector<Args> const &...args);

template <typename Fn, typename... Args>
auto invoke(Fn const &fn, std::vector<Args> const &...args)
    -> std::enable_if_t<
        !std::is_void_v<typename std::invoke_result_t<Fn, Args...>>,
        std::vector<typename std::invoke_result_t<Fn, Args...>>>;

template <typename Fn, typename... Args>
auto invoke(Fn const &fn, std::vector<Args> const &...args)
    -> std::enable_if_t<
        std::is_void_v<typename std::invoke_result_t<Fn, Args...>>, void>;

} // namespace ensemble

namespace multithread {

class core {

public:
  core(int suggestion);

  core(const core &) = default;
  core &operator=(const core &) = default;

  virtual ~core() = default;

  /**
   * @brief Run the function on the underlying slots, multi-threading if
   * enabled.
   * @param[in] fn Function to be called.
   * @param[in] args (Optional) arguments applied per-slot to function.
   * @details The methods are called on each slot, and the model is left
   * untouched.
   */
  template <typename Fn, typename... Args>
  void run(Fn const &fn, std::vector<Args> const &...args) const;

  bool is_enabled() const { return m_enabled; }
  unsigned int concurrency() const { return m_concurrency; }

protected:
  bool m_enabled;
  unsigned int m_concurrency;
};

} // namespace multithread

} // namespace queryosity

inline queryosity::multithread::core::core(int suggestion)
    : m_enabled(suggestion) {
  if (!suggestion) // single-threaded
    m_concurrency = 1;
  else if (suggestion < 0) // maximum thread count
    m_concurrency = std::thread::hardware_concurrency();
  else // (up to maximum) requested thread count
    m_concurrency =
        std::min<unsigned int>(std::thread::hardware_concurrency(), suggestion);
}

template <typename Fn, typename... Args>
void queryosity::multithread::core::run(
    Fn const &fn, std::vector<Args> const &...args) const {
  auto nslots = ensemble::check(args...);

  if (this->is_enabled()) {
    // enabled
    std::vector<std::thread> pool;
    pool.reserve(nslots);
    for (size_t islot = 0; islot < nslots; ++islot) {
      pool.emplace_back(fn, args.at(islot)...);
    }
    for (auto &&thread : pool) {
      thread.join();
    }
  } else {
    // disabled
    for (size_t islot = 0; islot < nslots; ++islot) {
      fn(args.at(islot)...);
    }
  }
}

template <typename T, typename... Args>
inline unsigned int
queryosity::ensemble::check(std::vector<T> const &first,
                            std::vector<Args> const &...args) {
  assert(((first.size() == args.size()) && ...));
  (args.size(), ...); // suppress GCC unused parameter warnings
  return first.size();
}

template <typename Fn, typename... Args>
inline auto queryosity::ensemble::invoke(Fn const &fn,
                                         std::vector<Args> const &...args)
    -> std::enable_if_t<
        !std::is_void_v<typename std::invoke_result_t<Fn, Args...>>,
        std::vector<typename std::invoke_result_t<Fn, Args...>>> {
  auto nslots = check(args...);
  using slot_t = typename std::invoke_result_t<Fn, Args...>;
  typename std::vector<slot_t> invoked;
  invoked.reserve(nslots);
  for (size_t i = 0; i < nslots; ++i) {
    invoked.push_back(std::move((fn(args.at(i)...))));
  }
  return invoked;
}

template <typename Fn, typename... Args>
inline auto queryosity::ensemble::invoke(Fn const &fn,
                                         std::vector<Args> const &...args)
    -> std::enable_if_t<
        std::is_void_v<typename std::invoke_result_t<Fn, Args...>>, void> {
  auto nslots = check(args...);
  for (size_t i = 0; i < nslots; ++i) {
    fn(args.at(i)...);
  }
}

template <typename T>
T *queryosity::ensemble::slotted<T>::get_slot(unsigned int islot) const {
  return this->get_slots().at(islot);
}

template <typename T>
unsigned int queryosity::ensemble::slotted<T>::size() const {
  return this->get_slots().size();
}

#include <set>
#include <string>
#include <type_traits>

namespace queryosity {

template <typename T> class lazy;

namespace systematic {

template <typename... Nodes>
auto get_variation_names(Nodes const &...nodes) -> std::set<std::string>;

template <typename Node> class resolver;

} // namespace systematic

} // namespace queryosity

template <typename... Nodes>
auto queryosity::systematic::get_variation_names(Nodes const &...nodes)
    -> std::set<std::string> {
  std::set<std::string> variation_names;
  (variation_names.merge(nodes.get_variation_names()), ...);
  return variation_names;
}

namespace queryosity {

class action {

public:
  action() = default;
  virtual ~action() = default;

  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) = 0;
  virtual void execute(unsigned int slot, unsigned long long entry) = 0;
  virtual void finalize(unsigned int slot) = 0;

protected:
};

} // namespace queryosity

#include <functional>
#include <memory>
#include <type_traits>

namespace queryosity {

/**
 * @brief Compute quantities of interest.
 */
namespace column {

class node : public action {
  public:
    node() = default;
    virtual ~node() = default;
};

//---------------------------------------------------
// view can actually report on the concrete data type
//---------------------------------------------------
template <typename T> class view {

public:
  using value_type = T;

public:
  template <typename U> class converted_from;

  template <typename U> class interface_of;

public:
  view() = default;
  virtual ~view() = default;

  virtual T const &value() const = 0;
  virtual T const *field() const;
};

//------------------------------------
// conversion between compatible types
//------------------------------------
template <typename To>
template <typename From>
class view<To>::converted_from : public view<To> {

public:
  converted_from(view<From> const &from);
  virtual ~converted_from() = default;

public:
  virtual const To &value() const override;

private:
  view<From> const *m_from;
  mutable To m_converted_from;
};

//------------------------------------------
// interface between inherited -> base type
//------------------------------------------
template <typename To>
template <typename From>
class view<To>::interface_of : public view<To> {

public:
  interface_of(view<From> const &from);
  virtual ~interface_of() = default;

public:
  virtual const To &value() const override;

private:
  view<From> const *m_impl;
};

template <typename T> class valued : public column::node, public view<T> {

public:
  using value_type = typename view<T>::value_type;

public:
  valued() = default;
  virtual ~valued() = default;

  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) override;
  virtual void execute(unsigned int slot, unsigned long long entry) override;
  virtual void finalize(unsigned int slot) override;
};

// costly to move around
template <typename T> class variable {

public:
  variable() = default;
  template <typename U> variable(view<U> const &val);
  ~variable() = default;

  variable(variable &&) = default;
  variable &operator=(variable &&) = default;

  T const &value() const;
  T const *field() const;

protected:
  std::unique_ptr<const view<T>> m_view;
};

/**
 * @brief Column observable.
 * @tparam Val Column data type.
 */
template <typename Val> class observable {

public:
  /**
   * @brief Constructor out of a variable.
   */
  observable(variable<Val> const &obs);
  ~observable() = default;

  /**
   * @brief Compute and retrieve the value of the column.
   * @return Value of the column.
   * @brief The column is *not* computed until the method is called at least
   * once during the dataflow entry processing. Once computed, it is cached for
   * future retrievals.
   */
  Val const &value() const;
  Val const *field() const;

  /**
   * @brief Shortcut for `value()`.
   */
  Val const &operator*() const;

  /**
   * @brief Indirection semantic for non-trivial data types.
   */
  Val const *operator->() const;

protected:
  const variable<Val> &m_var;
};

template <typename To, typename From>
std::unique_ptr<view<To>> view_as(view<From> const &from);

class computation;

template <typename> class reader;

template <typename> class fixed;

template <typename> class calculation;

template <typename> class definition;

template <typename, typename U> class conversion;

template <typename> class equation;

template <typename> class composition;

template <typename> class evaluator;

template <typename> struct constant;

template <typename> struct expression;

template <typename> struct series;

template <typename> struct nominal;

template <typename> struct variation;

template <typename T>
constexpr std::true_type check_reader(typename column::reader<T> const &);
constexpr std::false_type check_reader(...);

template <typename T>
constexpr std::true_type check_fixed(typename column::fixed<T> const &);
constexpr std::false_type check_fixed(...);

template <typename T>
constexpr std::true_type
check_definition(typename column::definition<T> const &);
constexpr std::false_type check_definition(...);

template <typename T>
constexpr std::true_type check_equation(typename column::equation<T> const &);
constexpr std::false_type check_equation(...);

template <typename T>
constexpr std::true_type
check_composition(typename column::composition<T> const &);
constexpr std::false_type check_composition(...);

template <typename T>
constexpr bool is_reader_v =
    decltype(check_reader(std::declval<std::decay_t<T> const &>()))::value;

template <typename T>
constexpr bool is_fixed_v =
    decltype(check_fixed(std::declval<std::decay_t<T> const &>()))::value;

template <typename T>
constexpr bool is_definition_v =
    decltype(check_definition(std::declval<std::decay_t<T> const &>()))::value;

template <typename T>
constexpr bool is_equation_v =
    decltype(check_equation(std::declval<std::decay_t<T> const &>()))::value;

template <typename T>
constexpr bool is_composition_v =
    decltype(check_composition(std::declval<std::decay_t<T> const &>()))::value;

template <typename T> struct is_evaluator : std::false_type {};
template <typename T>
struct is_evaluator<column::evaluator<T>> : std::true_type {};

template <typename T> constexpr bool is_evaluatable_v = is_evaluator<T>::value;

template <typename Fn> struct deduce_equation;

template <typename Ret, typename... Args>
struct deduce_equation<std::function<Ret(Args...)>> {
  using type = column::equation<std::decay_t<Ret>(std::decay_t<Args>...)>;
};

template <typename Fn>
using equation_t = typename deduce_equation<
    typename column::expression<Fn>::function_type>::type;

template <typename T> using evaluated_t = typename T::evaluated_type;

template <typename T>
using value_t = std::decay_t<decltype(std::declval<T>().value())>;

} // namespace column

template <typename T>
constexpr bool is_column_v = std::is_base_of_v<column::node, T>;

} // namespace queryosity

template <typename T>
void queryosity::column::valued<T>::initialize(unsigned int, unsigned long long,
                                               unsigned long long) {}

template <typename T>
void queryosity::column::valued<T>::execute(unsigned int, unsigned long long) {}

template <typename T>
void queryosity::column::valued<T>::finalize(unsigned int) {}

template <typename T> T const *queryosity::column::view<T>::field() const {
  return &this->value();
}

template <typename To>
template <typename From>
queryosity::column::view<To>::converted_from<From>::converted_from(
    view<From> const &from)
    : m_from(&from) {}

template <typename To>
template <typename From>
const To &queryosity::column::view<To>::converted_from<From>::value() const {
  m_converted_from = std::move(m_from->value());
  return m_converted_from;
}

template <typename Base>
template <typename Impl>
queryosity::column::view<Base>::interface_of<Impl>::interface_of(
    view<Impl> const &from)
    : m_impl(&from) {}

template <typename Base>
template <typename Impl>
const Base &queryosity::column::view<Base>::interface_of<Impl>::value() const {
  return m_impl->value();
}

template <typename To, typename From>
std::unique_ptr<queryosity::column::view<To>>
queryosity::column::view_as(view<From> const &from) {
  static_assert(std::is_same_v<From, To> || std::is_base_of_v<To, From> ||
                    std::is_convertible_v<From, To>,
                "incompatible value types");
  if constexpr (std::is_same_v<From, To> || std::is_base_of_v<To, From>) {
    return std::make_unique<
        typename queryosity::column::view<To>::template interface_of<From>>(
        from);
  } else if constexpr (std::is_convertible_v<From, To>) {
    return std::make_unique<
        typename queryosity::column::view<To>::template converted_from<From>>(
        from);
  }
}

// --------
// variable
// --------

template <typename T>
template <typename U>
queryosity::column::variable<T>::variable(view<U> const &val)
    : m_view(view_as<T>(val)) {}

template <typename T> T const &queryosity::column::variable<T>::value() const {
  return m_view->value();
}

template <typename T> T const *queryosity::column::variable<T>::field() const {
  return m_view->field();
}

template <typename T>
queryosity::column::observable<T>::observable(const variable<T> &var)
    : m_var(var) {}

template <typename T>
T const &queryosity::column::observable<T>::operator*() const {
  return m_var.value();
}

template <typename T>
T const *queryosity::column::observable<T>::operator->() const {
  return m_var.field();
}

template <typename T>
T const &queryosity::column::observable<T>::value() const {
  return m_var.value();
}

template <typename T>
T const *queryosity::column::observable<T>::field() const {
  return m_var.field();
}

namespace queryosity {

namespace dataset {

/**
 * @ingroup abc
 * @brief Custom dataset source
 */
class source : public action {
public:
  source() = default;
  virtual ~source() = default;

  /**
   * @brief Inform the dataset of parallelism.
   */
  virtual void parallelize(unsigned int concurrency) = 0;

  /**
   * @brief Initialize dataset processing
   */
  virtual void initialize();

  /**
   * @brief Determine dataset partition for parallel processing.
   * @return Dataset partition
   *
   * @details
   * A non-empty partition **MUST** begin from the `0` and be sorted contiguous
   * order, e.g.:
   * @code{.cpp} {{0,100},{100,200}, ..., {900,1000}} @endcode
   * If a dataset returns an empty partition, it relinquishes its control over
   * the entry loop to another dataset with a non-empty partition.
   * @attention
   * - Non-empty partitions reported from multiple datasets need to be aligned
   * to form a common denominator partition over which the dataset processing is
   * parallelized. As such, they **MUST** have (1) at minimum, the same total
   * number of entries, and (2) ideally, shared sub-range boundaries.
   * - Any dataset reporting an empty partition **MUST** be able to fulfill
   * `dataset::source::execute()` calls for any entry number as requested by the
   * other datasets loaded in the dataflow.
   *
   */
  virtual std::vector<std::pair<unsigned long long, unsigned long long>>
  partition() = 0;

  /**
   * @brief Enter an entry loop.
   * @param[in] slot Thread slot number.
   * @param[in] begin First entry number processed.
   * @param[in] end Loop stops after `end-1`-th entry has been processed.
   */
  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) override;

  /**
   * @brief Process an entry.
   * @param[in] slot Thread slot number.
   * @param[in] entry Entry being processed.
   */
  virtual void execute(unsigned int slot, unsigned long long entry) override;

  /**
   * @brief Exit an entry loop.
   * @param[in] slot Thread slot number.
   * @param[in] entry Entry being processed.
   */
  virtual void finalize(unsigned int slot) override;

  /**
   * @brief Finalize processing the dataset.
   */
  virtual void finalize();
};

/**
 * @ingroup abc
 * @brief Custom dataset reader
 */
template <typename DS> class reader : public source {

public:
  virtual ~reader() = default;

  template <typename Val>
  std::unique_ptr<queryosity::column::reader<Val>>
  read_column(unsigned int slot, const std::string &name);

  /**
   * @brief Read a column.
   * @tparam Val Column value type.
   * @param slot Thread slot number.
   * @param name Column name.
   * @return Column implementation.
   */
  template <typename Val>
  std::unique_ptr<queryosity::column::reader<Val>>
  read(unsigned int slot, const std::string &name);
};

} // namespace dataset

} // namespace queryosity

template <typename T, typename Val>
using read_column_t =
    typename decltype(std::declval<T>().template read_column<Val>(
        std::declval<unsigned int>(),
        std::declval<std::string const &>()))::element_type;

namespace queryosity {

namespace column {

struct range;

/**
 * @ingroup abc
 * @brief Read columns from a dataset.
 * @tparam T column data type.
 */
template <typename T> class reader : public valued<T> {

public:
  using const_reference = T const &;

public:
  reader();
  virtual ~reader() = default;

  /**
   * Read the value of the column at current entry.
   * @param[in] slot Multithreaded slot enumerator.
   * @param[in] entry Dataset global entry enumerator.
   * @return Column value at current entry.
   */
  virtual const_reference read(unsigned int, unsigned long long) const = 0;

  virtual const_reference value() const override;

  virtual void execute(unsigned int, unsigned long long) final override;

protected:
  mutable T const *m_addr;
  mutable bool m_updated;

private:
  unsigned int m_slot;
  unsigned long long m_entry;
};

} // namespace column

} // namespace queryosity

#include <cassert>
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace queryosity {

/**
 * @brief Process a dataset.
 */
namespace dataset {

using part_t = std::pair<unsigned long long, unsigned long long>;

class source;

class player;

class processor;

template <typename T> class reader;

template <typename T> struct input;

template <typename T> class loaded;

template <typename T> class column;

template <typename... Ts> class columns;

struct head {
  head(long long nrows) : nrows(nrows) {}
  long long nrows;
  operator long long() { return nrows; }
};

struct offset {
  offset(unsigned long long pos) : pos(pos) {}
  unsigned long long pos;
  operator unsigned long long() { return pos; }
};

struct weight {
  weight(double value) : value(value) {}
  double value;
  operator double() { return value; }
};

} // namespace dataset

} // namespace queryosity

template <typename T>
queryosity::column::reader<T>::reader()
    : m_addr(nullptr), m_updated(false), m_slot(0), m_entry(0) {}

template <typename T> T const &queryosity::column::reader<T>::value() const {
  if (!this->m_updated) {
    m_addr = &(this->read(this->m_slot, this->m_entry));
    m_updated = true;
  }
  return *m_addr;
}

template <typename T>
void queryosity::column::reader<T>::execute(unsigned int slot,
                                            unsigned long long entry) {
  this->m_slot = slot;
  this->m_entry = entry;
  this->m_updated = false;
}

inline void queryosity::dataset::source::initialize() {}

inline void queryosity::dataset::source::initialize(unsigned int,
                                                    unsigned long long,
                                                    unsigned long long) {}

inline void queryosity::dataset::source::execute(unsigned int,
                                                 unsigned long long) {}

inline void queryosity::dataset::source::finalize(unsigned int) {}

inline void queryosity::dataset::source::finalize() {}

template <typename DS>
template <typename Val>
std::unique_ptr<queryosity::column::reader<Val>>
queryosity::dataset::reader<DS>::read_column(unsigned int slot,
                                             const std::string &name) {
  auto col_rdr = static_cast<DS *>(this)->template read<Val>(slot, name);
  if (!col_rdr)
    throw std::runtime_error("dataset column cannot be read");
  return col_rdr;
}

template <typename DS>
template <typename Val>
std::unique_ptr<queryosity::column::reader<Val>>
queryosity::dataset::reader<DS>::read(unsigned int, const std::string &) {
  return nullptr;
}

#include <memory>
#include <tuple>

namespace queryosity {

/**
 * @brief Calculate a column value for each dataset entry.
 * @tparam Val Column value type.
 * @details A calculation is performed once per-entry (if needed) and its value
 * is stored for multiple accesses by downstream actions within the entry.
 * The type `Val` must be *CopyConstructible* and *CopyAssignable*.
 */
template <typename Val> class column::calculation : public valued<Val> {

public:
  calculation();
  virtual ~calculation() = default;

protected:
  template <typename... Args> calculation(Args &&...args);

public:
  virtual const Val &value() const final override;

  virtual Val calculate() const = 0;

  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) override;
  virtual void execute(unsigned int slot, unsigned long long entry) override;
  virtual void finalize(unsigned int slot) override;

protected:
  void update() const;
  void reset() const;

protected:
  mutable Val m_value;
  mutable bool m_updated;
};

} // namespace queryosity

template <typename Val>
queryosity::column::calculation<Val>::calculation()
    : m_value(), m_updated(false) {}

template <typename Val>
template <typename... Args>
queryosity::column::calculation<Val>::calculation(Args &&...args)
    : m_value(std::forward<Args>(args)...) {}

template <typename Val>
const Val &queryosity::column::calculation<Val>::value() const {
  if (!m_updated)
    this->update();
  return m_value;
}

template <typename Val>
void queryosity::column::calculation<Val>::update() const {
  m_value = std::move(this->calculate());
  m_updated = true;
}

template <typename Val>
void queryosity::column::calculation<Val>::reset() const {
  m_updated = false;
}

template <typename Val>
void queryosity::column::calculation<Val>::initialize(unsigned int,
                                                      unsigned long long,
                                                      unsigned long long) {}

template <typename Val>
void queryosity::column::calculation<Val>::execute(unsigned int,
                                                   unsigned long long) {
  this->reset();
}

template <typename Val>
void queryosity::column::calculation<Val>::finalize(unsigned int) {}

#include <functional>
#include <memory>
#include <type_traits>

namespace queryosity {

namespace column {

template <typename T> class evaluator {

public:
  using evaluated_type = T;

public:
  template <typename... Args> evaluator(Args const &...args);
  virtual ~evaluator() = default;

  template <typename... Vals>
  std::unique_ptr<T> evaluate(view<Vals> const &...cols) const;

protected:
  std::function<std::unique_ptr<T>()> m_make;
};
} // namespace column

} // namespace queryosity

template <typename T>
template <typename... Args>
queryosity::column::evaluator<T>::evaluator(Args const &...args)
    : m_make([args...]() { return std::make_unique<T>(args...); }) {}

template <typename T>
template <typename... Vals>
std::unique_ptr<T>
queryosity::column::evaluator<T>::evaluate(view<Vals> const &...columns) const {
  auto defn = m_make();
  defn->set_arguments(columns...);
  return defn;
}

#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <cassert>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

namespace queryosity {

namespace dataset {

using entry_t = unsigned long long;

using partition_t = std::vector<part_t>;

using slot_t = unsigned int;

namespace partition {

partition_t align(std::vector<partition_t> const &partitions);

partition_t truncate(partition_t const &parts, long long nentries_max);

} // namespace partition

} // namespace dataset

} // namespace queryosity

inline queryosity::dataset::partition_t queryosity::dataset::partition::align(
    std::vector<partition_t> const &partitions) {
  std::map<entry_t, unsigned int> edge_counts;
  const unsigned int num_vectors = partitions.size();

  // Count appearances of each edge
  for (auto const &vec : partitions) {
    std::map<entry_t, bool>
        seen_edges; // Ensure each edge is only counted once per vector
    for (auto const &p : vec) {
      if (seen_edges.find(p.first) == seen_edges.end()) {
        edge_counts[p.first]++;
        seen_edges[p.first] = true;
      }
      if (seen_edges.find(p.second) == seen_edges.end()) {
        edge_counts[p.second]++;
        seen_edges[p.second] = true;
      }
    }
  }

  // Filter edges that appear in all vectors
  std::vector<entry_t> aligned_edges;
  for (auto const &pair : edge_counts) {
    if (pair.second == num_vectors) {
      aligned_edges.push_back(pair.first);
    }
  }

  // Create aligned vector of pairs
  std::vector<std::pair<entry_t, entry_t>> aligned_ranges;
  for (size_t i = 0; i < aligned_edges.size() - 1; ++i) {
    aligned_ranges.emplace_back(aligned_edges[i], aligned_edges[i + 1]);
  }

  return aligned_ranges;
}

inline queryosity::dataset::partition_t
queryosity::dataset::partition::truncate(
    queryosity::dataset::partition_t const &parts, long long nentries_max) {
  if (nentries_max < 0)
    return parts;

  partition_t parts_truncated;

  for (auto const &part : parts) {
    auto part_end = nentries_max >= 0
                        ? std::min(part.first + nentries_max, part.second)
                        : part.second;
    parts_truncated.emplace_back(part.first, part_end);
    nentries_max -= (part_end - part.first);
    if (!nentries_max)
      break;
  }

  return parts_truncated;
}

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace queryosity {

namespace column {

class computation {

public:
  computation() = default;
  virtual ~computation() = default;

public:
  template <typename DS, typename Val>
  auto read(dataset::reader<DS> &ds, unsigned int slot, const std::string &name)
      -> read_column_t<DS, Val> *;

  template <typename Val> auto assign(Val const &val) -> fixed<Val> *;

  template <typename To, typename Col>
  auto convert(Col const &col) -> conversion<To, value_t<Col>> *;

  template <typename Def, typename... Args>
  auto define(Args const &...vars) const
      -> std::unique_ptr<evaluator<Def>>;

  template <typename Ret, typename... Args>
  auto equate(std::function<Ret(Args...)> fn) const -> std::unique_ptr<
      evaluator<equation<std::decay_t<Ret>(std::decay_t<Args>...)>>>;

  template <typename Def, typename... Cols>
  auto evaluate(evaluator<Def> const&calc, Cols const &...cols) -> Def *;

protected:
  template <typename Col> auto add_column(std::unique_ptr<Col> col) -> Col *;

protected:
  std::vector<std::unique_ptr<column::node>> m_columns;
};

}

} // namespace queryosity

#include <memory>
#include <tuple>

namespace queryosity {

template <typename To, typename From>
class column::conversion : public column::definition<To(From)> {
public:
  conversion(view<From> const &from);
  virtual ~conversion() = default;

  virtual To evaluate(observable<From> from) const override;
};

} // namespace queryosity

template <typename To, typename From>
queryosity::column::conversion<To, From>::conversion(view<From> const &from) {
  this->set_arguments(from);
}

template <typename To, typename From>
To queryosity::column::conversion<To, From>::evaluate(
    observable<From> from) const {
  return from.value();
}

#include <functional>

namespace queryosity {

namespace column {

template <typename Out, typename... Ins>
class equation<Out(Ins...)> : public definition<Out(Ins...)> {

public:
  using vartuple_type = typename definition<Out(Ins...)>::vartuple_type;
  using function_type =
      std::function<std::decay_t<Out>(std::decay_t<Ins> const &...)>;

public:
  template <typename Fn> equation(Fn&& fn);
  virtual ~equation() = default;

public:
  virtual Out evaluate(observable<Ins>... args) const final override;

  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) final override;
  virtual void execute(unsigned int slot, unsigned long long entry) final override;
  virtual void finalize(unsigned int slot) final override;

protected:
  function_type m_evaluate;
};

} // namespace column

} // namespace queryosity

template <typename Out, typename... Ins>
template <typename Fn>
queryosity::column::equation<Out(Ins...)>::equation(Fn&& fn) : m_evaluate(std::forward<Fn>(fn)) {}

template <typename Out, typename... Ins>
Out queryosity::column::equation<Out(Ins...)>::evaluate(
    observable<Ins>... args) const {
  return this->m_evaluate(args.value()...);
}

template <typename Out, typename... Ins>
void queryosity::column::equation<Out(Ins...)>::initialize(unsigned int slot, unsigned long long begin,
                                               unsigned long long end) {
  calculation<Out>::initialize(slot, begin, end);
                                               }

template <typename Out, typename... Ins>
void queryosity::column::equation<Out(Ins...)>::execute(unsigned int slot, unsigned long long entry) {
  calculation<Out>::execute(slot, entry);
}

template <typename Out, typename... Ins>
void queryosity::column::equation<Out(Ins...)>::finalize(unsigned int slot) {
  calculation<Out>::finalize(slot);
}

namespace queryosity {

//------------------------------------------------------------------------------
// fixed: value set manually
//------------------------------------------------------------------------------
template <typename Val> class column::fixed : public valued<Val> {

public:
  fixed(Val const &val);
  template <typename... Args> fixed(Args &&...args);
  virtual ~fixed() = default;

  const Val &value() const final override;

  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) final override;
  virtual void execute(unsigned int slot, unsigned long long entry) final override;
  virtual void finalize(unsigned int slot) final override;

protected:
  Val m_value;
};

} // namespace queryosity

template <typename Val>
queryosity::column::fixed<Val>::fixed(Val const &val) : m_value(val) {}

template <typename Val>
template <typename... Args>
queryosity::column::fixed<Val>::fixed(Args &&...args)
    : m_value(std::forward<Args>(args)...) {}

template <typename Val>
const Val &queryosity::column::fixed<Val>::value() const {
  return m_value;
}

template <typename Val>
void queryosity::column::fixed<Val>::initialize(unsigned int slot, unsigned long long begin,
                                               unsigned long long end) {
  valued<Val>::initialize(slot, begin, end);
                                               }

template <typename Val>
void queryosity::column::fixed<Val>::execute(unsigned int slot, unsigned long long entry) {
  valued<Val>::execute(slot, entry);
}

template <typename Val>
void queryosity::column::fixed<Val>::finalize(unsigned int slot) {
  valued<Val>::finalize(slot);
}

template <typename DS, typename Val>
auto queryosity::column::computation::read(dataset::reader<DS> &ds,
                                           unsigned int slot,
                                           const std::string &name)
    -> read_column_t<DS, Val> * {
  auto rdr = ds.template read_column<Val>(slot, name);
  return this->add_column(std::move(rdr));
}

template <typename Val>
auto queryosity::column::computation::assign(Val const &val) -> fixed<Val> * {
  auto cnst = std::make_unique<typename column::fixed<Val>>(val);
  return this->add_column(std::move(cnst));
}

template <typename To, typename Col>
auto queryosity::column::computation::convert(Col const &col)
    -> conversion<To, value_t<Col>> * {
  auto cnv = std::make_unique<conversion<To, value_t<Col>>>(col);
  cnv->set_arguments(col);
  return this->add_column(std::move(cnv));
}

template <typename Def, typename... Args>
auto queryosity::column::computation::define(Args const &...args) const
    -> std::unique_ptr<evaluator<Def>> {
  return std::make_unique<evaluator<Def>>(args...);
}

template <typename Ret, typename... Args>
auto queryosity::column::computation::equate(std::function<Ret(Args...)> fn)
    const -> std::unique_ptr<
      evaluator<equation<std::decay_t<Ret>(std::decay_t<Args>...)>>> {
  return std::make_unique<
      evaluator<equation<std::decay_t<Ret>(std::decay_t<Args>...)>>>(
      fn);
}

template <typename Def, typename... Cols>
auto queryosity::column::computation::evaluate(evaluator<Def> const &calc,
                                               Cols const &...cols) -> Def * {
  auto defn = calc.evaluate(cols...);
  return this->add_column(std::move(defn));
}

template <typename Col>
auto queryosity::column::computation::add_column(std::unique_ptr<Col> col)
    -> Col * {
  auto out = col.get();
  m_columns.push_back(std::move(col));
  return out;
}

#include <memory>
#include <vector>

#include <memory>
#include <string>
#include <vector>

#include <memory>
#include <string>
#include <vector>

namespace queryosity {

/**
 * @brief Apply cuts and weights to entries.
 */
namespace selection {

class cutflow;

class cut;

class weight;

template <typename T, typename U> class applicator;

struct count_t;

class counter;

template <typename... Ts> struct yield;

class node : public column::calculation<double> {

public:
  node(const selection::node *presel, column::variable<double> dec);
  virtual ~node() = default;

public:
  bool is_initial() const noexcept;
  const selection::node *get_previous() const noexcept;

  virtual bool passed_cut() const = 0;
  virtual double get_weight() const = 0;

  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) final override;
  virtual void execute(unsigned int slot, unsigned long long entry) final override;
  virtual void finalize(unsigned int slot) final override;

protected:
  const selection::node *const m_preselection;
  column::variable<double> m_decision;
};

template <typename T> struct is_applicable : std::false_type {};
template <typename T, typename U>
struct is_applicable<selection::applicator<T,U>> : std::true_type {};
template <typename T>
static constexpr bool is_applicable_v = is_applicable<T>::value;

template <typename T>
using applied_t = typename T::selection_type;

} // namespace selection

template <typename T>
constexpr bool is_selection_v = std::is_base_of_v<selection::node, T>;
} // namespace queryosity

#include <functional>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

/**
 * @brief All queryosity namespaces and classes.
 */
namespace queryosity {

namespace column {

template <typename T> class view;

template <typename T> class valued;

template <typename T> class variable;

template <typename T> class observable;

} // namespace column

namespace selection {

class node;

}

/**
 * @brief Perform a query.
 */
namespace query {

class experiment;

template <typename T> class aggregation;

template <typename... Ts> class fillable;

template <typename T> class definition;

template <typename T> class booker;

template <typename T> class series;

template <typename T> class calculation;

template <typename T> struct output;

class node : public action {

public:
  node();
  virtual ~node() = default;

  void apply_scale(double scale);
  void use_weight(bool use = true);

  void set_selection(const selection::node &selection);
  const selection::node *get_selection() const;

  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) override;
  virtual void execute(unsigned int slot, unsigned long long entry) final override;
  virtual void finalize(unsigned int slot) override;

  virtual void count(double w) = 0;

protected:
  double m_scale;
  const selection::node *m_selection;
};

template <typename T>
constexpr std::true_type check_implemented(query::aggregation<T> const &);
constexpr std::false_type check_implemented(...);

template <typename... Vals>
constexpr std::true_type
check_fillable(query::fillable<Vals...> const &);
constexpr std::false_type check_fillable(...);

template <typename T> struct is_bookable : std::false_type {};
template <typename T> struct is_bookable<query::booker<T>> : std::true_type {};

template <typename T>
constexpr bool has_result_v =
    decltype(check_implemented(std::declval<T>()))::value;

template <typename T>
constexpr bool is_fillable_v =
    decltype(check_fillable(std::declval<T>()))::value;

template <typename T> constexpr bool is_bookable_v = is_bookable<T>::value;

template <typename Bkr> using booked_t = typename Bkr::booked_type;

// mixin class to conditionally add a member variable
template <typename Action, typename Enable = void> struct result_of {};

// Specialization for types satisfying is_query
template <typename Action>
struct result_of<Action, std::enable_if_t<query::has_result_v<Action>>> {
  using result_type = decltype(std::declval<Action>().result());
  result_of() : m_merged(false) {}
  virtual ~result_of() = default;

protected:
  result_type m_result;
  bool m_merged;
};

} // namespace query

} // namespace queryosity

inline queryosity::query::node::node() : m_scale(1.0), m_selection(nullptr) {}

inline void
queryosity::query::node::set_selection(const selection::node &selection) {
  m_selection = &selection;
}

inline const queryosity::selection::node *
queryosity::query::node::get_selection() const {
  return m_selection;
}

inline void queryosity::query::node::apply_scale(double scale) {
  m_scale *= scale;
}

inline void queryosity::query::node::initialize(unsigned int,
                                                unsigned long long,
                                                unsigned long long) {
  if (!m_selection)
    throw std::runtime_error("no booked selection");
}

inline void queryosity::query::node::execute(unsigned int, unsigned long long) {
  if (m_selection->passed_cut()) {
    this->count(m_scale * m_selection->get_weight());
  }
}

inline void queryosity::query::node::finalize(unsigned int) {}

inline queryosity::selection::node::node(const selection::node *presel,
                                         column::variable<double> dec)
    : m_preselection(presel), m_decision(std::move(dec)) {}

inline bool queryosity::selection::node::is_initial() const noexcept {
  return m_preselection ? false : true;
}

inline const queryosity::selection::node *
queryosity::selection::node::get_previous() const noexcept {
  return m_preselection;
}

inline void queryosity::selection::node::initialize(unsigned int slot, unsigned long long begin,
                                               unsigned long long end) {
  column::calculation<double>::initialize(slot, begin, end);
                                               }

inline void queryosity::selection::node::execute(unsigned int slot, unsigned long long entry) {
  column::calculation<double>::execute(slot, entry);
}

inline void queryosity::selection::node::finalize(unsigned int slot) {
  column::calculation<double>::finalize(slot);
}

#include <functional>
#include <memory>
#include <type_traits>

namespace queryosity {

template <typename Sel, typename Def>
class selection::applicator : public column::evaluator<Def> {

public:
  using selection_type = Sel;
  using evaluated_type = typename column::evaluator<Def>::evaluated_type;

public:
  template <typename... Args>
  applicator(selection::node const *prev, Args const &...args);
  virtual ~applicator() = default;

  template <typename... Vals>
  std::pair<std::unique_ptr<Sel>, std::unique_ptr<Def>>
  apply(column::view<Vals> const &...columns) const;

protected:
  selection::node const *m_prev;
};

} // namespace queryosity

template <typename Sel, typename Def>
template <typename... Args>
queryosity::selection::applicator<Sel, Def>::applicator(
    selection::node const *prev, Args const &...args)
    : column::evaluator<Def>(args...), m_prev(prev) {}

template <typename Sel, typename Def>
template <typename... Vals>
std::pair<std::unique_ptr<Sel>, std::unique_ptr<Def>>
queryosity::selection::applicator<Sel, Def>::apply(
    column::view<Vals> const &...columns) const {
  auto col = this->evaluate(columns...);
  auto sel = std::make_unique<Sel>(m_prev, column::variable<double>(*col));
  return {std::move(sel), std::move(col)};
}

namespace queryosity {

class selection::cutflow : public column::computation {

public:
  cutflow() = default;
  ~cutflow() = default;

public:
  template <typename Sel, typename Val>
  auto apply(selection::node const *prev, column::valued<Val> const &dec)
      -> selection::node *;

  template <typename Sel, typename Ret, typename... Args>
  auto select(selection::node const *prev, std::function<Ret(Args...)> fn) const
      -> std::unique_ptr<applicator<
          Sel, column::equation<std::decay_t<Ret>(std::decay_t<Args>...)>>>;

  template <typename Sel, typename Def, typename... Cols>
  auto apply(applicator<Sel, Def> const &calc, Cols const &...cols)
      -> selection::node *;

protected:
  template <typename Sel>
  auto add_selection(std::unique_ptr<Sel> selection) -> Sel *;

protected:
  std::vector<std::unique_ptr<selection::node>> m_selections;
};

} // namespace queryosity

namespace queryosity {

class selection::cut : public selection::node {

public:
  cut(const selection::node *presel, column::variable<double> dec);
  virtual ~cut() = default;

public:
  virtual double calculate() const final override;
  virtual bool passed_cut() const final override;
  virtual double get_weight() const final override;
};

} // namespace queryosity

inline queryosity::selection::cut::cut(const selection::node *presel,
                                       column::variable<double> dec)
    : selection::node(presel, std::move(dec)) {}

inline double queryosity::selection::cut::calculate() const {
  return this->passed_cut();
}

inline bool queryosity::selection::cut::passed_cut() const {
  return this->m_preselection
             ? this->m_preselection->passed_cut() && m_decision.value()
             : m_decision.value();
}

inline double queryosity::selection::cut::get_weight() const {
  return this->m_preselection ? this->m_preselection->get_weight() : 1.0;
}

namespace queryosity {

class selection::weight : public selection::node {

public:
  class a_times_b;

public:
  weight(const selection::node *presel, column::variable<double> dec);
  virtual ~weight() = default;

public:
  virtual double calculate() const final override;
  virtual bool passed_cut() const final override;
  virtual double get_weight() const final override;
};

} // namespace queryosity

inline queryosity::selection::weight::weight(const selection::node *presel,
                                             column::variable<double> dec)
    : selection::node(presel, std::move(dec)) {}

inline double queryosity::selection::weight::calculate() const {
  return this->get_weight();
}

inline bool queryosity::selection::weight::passed_cut() const {
  return this->m_preselection ? this->m_preselection->passed_cut() : true;
}

inline double queryosity::selection::weight::get_weight() const {
  return this->m_preselection
             ? this->m_preselection->get_weight() * m_decision.value()
             : m_decision.value();
}

template <typename Sel, typename Val>
auto queryosity::selection::cutflow::apply(selection::node const *prev,
                                           column::valued<Val> const &dec)
    -> selection::node * {
  auto sel = std::make_unique<Sel>(prev, column::variable<double>(dec));
  return this->add_selection(std::move(sel));
}

template <typename Sel, typename Ret, typename... Args>
auto queryosity::selection::cutflow::select(
    selection::node const *prev, std::function<Ret(Args...)> fn) const
    -> std::unique_ptr<applicator<
        Sel, column::equation<std::decay_t<Ret>(std::decay_t<Args>...)>>> {
  return std::make_unique<applicator<
      Sel, column::equation<std::decay_t<Ret>(std::decay_t<Args>...)>>>(prev,
                                                                        fn);
}

template <typename Sel, typename Def, typename... Cols>
auto queryosity::selection::cutflow::apply(
    selection::applicator<Sel, Def> const &calc, Cols const &...cols)
    -> selection::node * {
  auto [sel, col] = calc.apply(cols...);
  this->add_column(std::move(col));
  return this->add_selection(std::move(sel));
}

template <typename Sel>
auto queryosity::selection::cutflow::add_selection(std::unique_ptr<Sel> sel)
    -> Sel * {
  auto out = sel.get();
  m_selections.push_back(std::move(sel));
  return out;
}

#include <functional>
#include <utility>
#include <vector>

namespace queryosity {

template <typename T> class query::booker {

public:
  using booked_type = T;

public:
  template <typename... Args> booker(Args... args);
  ~booker() = default;

  // copyable
  booker(const booker &) = default;
  booker &operator=(const booker &) = default;

  template <typename... Vals>
  auto add_columns(column::valued<Vals> const &...cols) const
      -> std::unique_ptr<booker<T>>;

  auto set_selection(const selection::node &sel) const -> std::unique_ptr<T>;

protected:
  std::unique_ptr<T> make_query();
  template <typename... Vals>
  void fill_query(column::valued<Vals> const &...cols);

protected:
  std::function<std::unique_ptr<T>()> m_make_unique_query;
  std::vector<std::function<void(T &)>> m_add_columns;
};

} // namespace queryosity

template <typename T>
template <typename... Args>
queryosity::query::booker<T>::booker(Args... args)
    : m_make_unique_query(std::bind(
          [](Args... args) { return std::make_unique<T>(args...); }, args...)) {
}

template <typename T>
template <typename... Vals>
auto queryosity::query::booker<T>::add_columns(
    column::valued<Vals> const &...columns) const -> std::unique_ptr<booker<T>> {
  // use a fresh one with its current fills
  auto filled = std::make_unique<booker<T>>(*this);
  // add fills
  filled->fill_query(columns...);
  // return new book
  return filled;
}

template <typename T>
template <typename... Vals>
void queryosity::query::booker<T>::fill_query(
    column::valued<Vals> const &...columns) {
  // use a snapshot of its current calls
  m_add_columns.push_back(std::bind(
      [](T &cnt, column::valued<Vals> const &...cols) {
        cnt.enter_columns(cols...);
      },
      std::placeholders::_1, std::cref(columns)...));
}

template <typename T>
auto queryosity::query::booker<T>::set_selection(const selection::node &sel) const
    -> std::unique_ptr<T> {
  // call constructor
  auto cnt = m_make_unique_query();
  // fill columns (if set)
  for (auto const &fill_query : m_add_columns) {
    fill_query(*cnt);
  }
  // book cnt at the selection
  cnt->set_selection(sel);
  return cnt;
}

namespace queryosity {

class query::experiment : public selection::cutflow {

public:
  experiment() = default;
  ~experiment() = default;

public:
  template <typename Qry, typename... Args>
  std::unique_ptr<query::booker<Qry>> make(Args &&...args);

  template <typename Qry>
  auto book(query::booker<Qry> const &bkr, const selection::node &sel) -> Qry *;

protected:
  template <typename Qry> auto add_query(std::unique_ptr<Qry> qry) -> Qry *;

protected:
  std::vector<query::node *> m_queries;
  std::vector<std::unique_ptr<query::node>> m_queries_history;
};

} // namespace queryosity

template <typename Qry, typename... Args>
std::unique_ptr<queryosity::query::booker<Qry>>
queryosity::query::experiment::make(Args &&...args) {
  auto bkr = std::make_unique<query::booker<Qry>>(std::forward<Args>(args)...);
  return bkr;
}

template <typename Qry>
auto queryosity::query::experiment::book(query::booker<Qry> const &bkr,
                                         const selection::node &sel) -> Qry * {
  auto qry = bkr.set_selection(sel);
  return this->add_query(std::move(qry));
}

template <typename Qry>
auto queryosity::query::experiment::add_query(std::unique_ptr<Qry> qry)
    -> Qry * {
  auto out = qry.get();
  m_queries_history.push_back(std::move(qry));
  m_queries.push_back(m_queries_history.back().get());
  return out;
}

namespace queryosity {

namespace dataset {

class player : public query::experiment {

public:
  player() = default;
  virtual ~player() = default;

public:
  void play(std::vector<std::unique_ptr<source>> const &sources, double scale,
            slot_t slot, std::vector<part_t> const &parts);
};

} // namespace dataset

} // namespace queryosity

inline void queryosity::dataset::player::play(
    std::vector<std::unique_ptr<source>> const &sources, double scale,
    slot_t slot, std::vector<part_t> const &parts) {

  // apply dataset scale in effect for all queries
  for (auto const &qry : m_queries) {
    qry->apply_scale(scale);
  }

  // traverse each part
  for (auto const &part : parts) {
    // initialize
    for (auto const &ds : sources) {
      ds->initialize(slot, part.first, part.second);
    }
    for (auto const &col : m_columns) {
      col->initialize(slot, part.first, part.second);
    }
    for (auto const &sel : m_selections) {
      sel->initialize(slot, part.first, part.second);
    }
    for (auto const &qry : m_queries) {
      qry->initialize(slot, part.first, part.second);
    }
    // execute
    for (auto entry = part.first; entry < part.second; ++entry) {
      for (auto const &ds : sources) {
        ds->execute(slot, entry);
      }
      for (auto const &col : m_columns) {
        col->execute(slot, entry);
      }
      for (auto const &sel : m_selections) {
        sel->execute(slot, entry);
      }
      for (auto const &qry : m_queries) {
        qry->execute(slot, entry);
      }
    }
    // finalize (in reverse order)
    for (auto const &qry : m_queries) {
      qry->finalize(slot);
    }
    for (auto const &sel : m_selections) {
      sel->finalize(slot);
    }
    for (auto const &col : m_columns) {
      col->finalize(slot);
    }
    for (auto const &ds : sources) {
      ds->finalize(slot);
    }
  }

  // clear out queries (should not be re-played)
  m_queries.clear();
}

namespace queryosity {

namespace dataset {

class processor : public multithread::core, public ensemble::slotted<player> {
public:
  processor(int suggestion);
  virtual ~processor() = default;

  processor(const processor &) = delete;
  processor &operator=(const processor &) = delete;

  processor(processor &&) noexcept = default;
  processor &operator=(processor &&) noexcept = default;

  template <typename DS, typename Val>
  auto read(dataset::reader<DS> &ds, const std::string &column_name)
      -> std::vector<read_column_t<DS, Val> *>;

  void downsize(unsigned int nslots);
  void process(std::vector<std::unique_ptr<source>> const &sources,
               double scale, unsigned long long nrows);

  virtual std::vector<player *> const &get_slots() const override;

protected:
  std::vector<unsigned int> m_range_slots;
  std::vector<dataset::player> m_players;
  std::vector<dataset::player *> m_player_ptrs;
};

} // namespace dataset

namespace multithread {

dataset::processor enable(int suggestion = -1);
dataset::processor disable();

} // namespace multithread

} // namespace queryosity

inline queryosity::dataset::processor
queryosity::multithread::enable(int suggestion) {
  return dataset::processor(suggestion);
}

inline queryosity::dataset::processor queryosity::multithread::disable() {
  return dataset::processor(false);
}

inline queryosity::dataset::processor::processor(int suggestion)
    : multithread::core::core(suggestion), m_range_slots(), m_players(), m_player_ptrs() {
  const auto nslots = this->concurrency();
  m_players = std::vector<player>(nslots);
  m_player_ptrs = std::vector<player *>(nslots, nullptr);
  std::transform(m_players.begin(), m_players.end(), m_player_ptrs.begin(),
                 [](player &plyr) -> player * { return &plyr; });
  m_range_slots.clear();
  m_range_slots.reserve(nslots);
  for (unsigned int i = 0; i < nslots; ++i) {
    m_range_slots.push_back(i);
  }
}

template <typename DS, typename Val>
auto queryosity::dataset::processor::read(dataset::reader<DS> &ds,
                                          const std::string &column_name)
    -> std::vector<read_column_t<DS, Val> *> {
  return ensemble::invoke(
      [column_name, &ds](dataset::player *plyr, unsigned int slot) {
        return plyr->template read<DS, Val>(ds, slot, column_name);
      },
      m_player_ptrs, m_range_slots);
}

inline void queryosity::dataset::processor::downsize(unsigned int nslots) {
  if (nslots > this->size()) {
    throw std::logic_error("requested thread count too high");
  };
  while (m_players.size() > nslots) {
    // avoid copy-constructor
    m_players.pop_back();
  }
  m_player_ptrs.resize(nslots);
  m_range_slots.resize(nslots);
}

inline void queryosity::dataset::processor::process(
    std::vector<std::unique_ptr<source>> const &sources, double scale,
    unsigned long long nrows) {

  const auto nslots = this->concurrency();

  // 1. enter event loop
  for (auto const &ds : sources) {
    ds->initialize();
  }

  // 2. partition dataset(s)
  // 2.1 get partition from each dataset source
  std::vector<partition_t> partitions_from_sources;
  for (auto const &ds : sources) {
    auto partition_from_source = ds->partition();
    if (partition_from_source.size())
      partitions_from_sources.push_back(std::move(partition_from_source));
  }
  if (!partitions_from_sources.size()) {
    throw std::runtime_error("no valid dataset partition found");
  }
  // 2.2 find common denominator partition
  const auto partition_aligned =
      dataset::partition::align(partitions_from_sources);
  // 2.3 truncate entries to row limit
  const auto partition_truncated =
      dataset::partition::truncate(partition_aligned, nrows);
  // 2.3 distribute partition amongst threads
  std::vector<partition_t> partitions_for_slots(nslots);
  auto nparts_remaining = partition_truncated.size();
  const auto nparts = nparts_remaining;
  while (nparts_remaining) {
    for (unsigned int islot = 0; islot < nslots; ++islot) {
      partitions_for_slots[islot].push_back(
          std::move(partition_truncated[nparts - (nparts_remaining--)]));
      if (!nparts_remaining)
        break;
    }
  }
  // todo: can intel tbb distribute slots during parallel processing?

  // 3. run event loop
  this->run(
      [&sources, scale](
          dataset::player *plyr, unsigned int slot,
          std::vector<std::pair<unsigned long long, unsigned long long>> const
              &parts) { plyr->play(sources, scale, slot, parts); },
      m_player_ptrs, m_range_slots, partitions_for_slots);

  // 4. exit event loop
  for (auto const &ds : sources) {
    ds->finalize();
  }
}

inline std::vector<queryosity::dataset::player *> const &
queryosity::dataset::processor::get_slots() const {
  return m_player_ptrs;
}

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
  template <typename> friend class dataflow::input;
  template <typename> friend class lazy;
  template <typename> friend class todo;
  template <typename> friend class varied;

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
  auto define(column::definition<Def> const &defn)
      -> todo<column::evaluator<Def>>;

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
   * @brief Plan a query.
   * @tparam Qry Concrete queryosity::query::definition implementation.
   * @param[in] output Query output (constructor arguments).
   * @return queryosity::todo query booker.
   */
  template <typename Qry>
  auto get(query::output<Qry> const &output) -> todo<query::booker<Qry>>;

  /**
   * @brief Get a column series.
   * @tparam Col (Varied) lazy column.
   * @param[in] col Column as series constructor argument.
   * @return (Varied) lazy column series query.
   */
  template <typename Col> auto get(column::series<Col> const &col);

  /**
   * @brief Get selection yield.
   * @tparam Sels (Varied) lazy selection(s).
   * @param[in] sel Selection(s) as yield constructor argument(s).
   * @return (Varied) lazy selection yield query(ies).
   */
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

  template <typename Col>
  auto _cut(lazy<Col> const &column) -> lazy<selection::node>;

  template <typename Lzy>
  auto _cut(varied<Lzy> const &column) -> varied<lazy<selection::node>>;

  template <typename Def, typename... Args> auto _define(Args &&...args);
  template <typename Def>
  auto _define(column::definition<Def> const &defn)
      -> todo<column::evaluator<Def>>;

  template <typename Fn> auto _equate(Fn fn);
  template <typename Fn>
  auto _equate(column::expression<Fn> const &expr)
      -> todo<column::evaluator<column::equation_t<Fn>>>;

  template <typename Sel, typename Fn> auto _select(Fn fn);
  template <typename Sel, typename Fn>
  auto _select(column::expression<Fn> const &expr)
      -> todo<selection::applicator<Sel, column::equation_t<Fn>>>;

  template <typename Sel, typename Fn>
  auto _select(lazy<selection::node> const &prev, Fn fn);
  template <typename Sel, typename Fn>
  auto _select(lazy<selection::node> const &prev,
               column::expression<Fn> const &expr)
      -> todo<selection::applicator<Sel, column::equation_t<Fn>>>;

  template <typename Qry, typename... Args>
  auto _make(Args &&...args) -> todo<query::booker<Qry>>;

protected:
  template <typename Kwd> void accept_kwarg(Kwd &&kwarg);

  void analyze();
  void reset();

  template <typename DS, typename Val>
  auto _read(dataset::reader<DS> &ds, const std::string &name)
      -> lazy<read_column_t<DS, Val>>;

  template <typename Def, typename... Cols>
  auto _evaluate(todo<column::evaluator<Def>> const &calc,
                 lazy<Cols> const &...columns) -> lazy<Def>;

  template <typename Sel, typename Col>
  auto _apply(lazy<Col> const &col) -> lazy<selection::node>;

  template <typename Sel, typename Col>
  auto _apply(lazy<selection::node> const &prev, lazy<Col> const &col)
      -> lazy<selection::node>;

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
  template <typename> friend class varied;

public:
  template <typename Fn, typename... Nodes>
  static auto invoke(Fn fn, Nodes const &...nodes) -> std::enable_if_t<
      !std::is_void_v<
          std::invoke_result_t<Fn, typename Nodes::action_type *...>>,
      std::vector<std::invoke_result_t<Fn, typename Nodes::action_type *...>>>;

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

#include <memory>

namespace queryosity {

namespace dataset {

/**
 * @ingroup api
 * @brief Argument to load a dataset input in the dataflow.
 * @tparam DS Concrete implementation of `queryosity::dataset::reader`.
 */
template <typename DS> struct input {
  /**
   * @brief Argument constructor.
   * @tparam Args Constructor arguments types for @p DS.
   * @param[in] args Constructor arguments for @p DS.
   */
  input() = default;
  template <typename... Args> input(Args &&...args);
  virtual ~input() = default;
  std::unique_ptr<DS> ds; //!
};

} // namespace dataset

} // namespace queryosity

template <typename DS>
template <typename... Args>
queryosity::dataset::input<DS>::input(Args &&...args)
    : ds(std::make_unique<DS>(std::forward<Args>(args)...)) {}

#include <map>

namespace queryosity {

namespace dataset {

template <typename DS> class loaded {

public:
  loaded(dataflow &df, DS &ds);
  ~loaded() = default;

  template <typename Val> auto _read(const std::string &name) {
    return m_df->_read<DS, Val>(*m_ds, name);
  }

  template <typename Val>
  auto read(dataset::column<Val> const &col)
      -> lazy<queryosity::column::valued<Val>>;

  template <typename... Vals> auto read(dataset::column<Vals> const &...cols);

  template <typename Val>
  auto vary(dataset::column<Val> const &col,
            std::map<std::string, std::string> const &vars);

protected:
  dataflow *m_df;
  dataset::reader<DS> *m_ds;
};

} // namespace dataset

} // namespace queryosity

#include <array>
#include <string>
#include <tuple>
#include <utility>

namespace queryosity {

class dataflow;

/**
 * @ingroup api
 * @brief Argument to read a column from a loaded dataset.
 * @tparam Val Column data type.
 */
template <typename Val> class dataset::column {

public:
  /**
   * @brief Argument constructor.
   * @param[in] column_name Name of column.
   */
  column(const std::string &column_name);
  ~column() = default;

  template <typename DS> auto _read(dataflow::input<DS> &ds) const;

protected:
  std::string m_name;
};

} // namespace queryosity

template <typename Val>
queryosity::dataset::column<Val>::column(const std::string &name)
    : m_name(name) {}

template <typename Val>
template <typename DS>
auto queryosity::dataset::column<Val>::_read(
    queryosity::dataflow::input<DS> &ds) const {
  return ds.template _read<Val>(this->m_name);
}

#include <iostream>
#include <memory>
#include <set>
#include <type_traits>
#include <vector>

#include <type_traits>

// check for the existence of a native/custom binary operator
#define CHECK_FOR_BINARY_OP(op_name, op_symbol)                                \
  template <typename T, typename Arg, typename = void>                         \
  struct has_native_##op_name : std::false_type {};                            \
                                                                               \
  template <typename T, typename Arg>                                          \
  struct has_native_##op_name<                                                 \
      T, Arg,                                                                  \
      std::void_t<decltype(std::declval<T>() op_symbol std::declval<Arg>())>>  \
      : std::true_type {};                                                     \
                                                                               \
  template <typename T, typename Arg>                                          \
  auto operator op_symbol(const T &, const Arg &) -> priority_tag<0>;          \
                                                                               \
  template <typename T, typename Arg> struct has_custom_only_##op_name {       \
  private:                                                                     \
    template <typename U, typename V>                                          \
    static auto test(priority_tag<1>)                                          \
        -> decltype(std::declval<U>() op_symbol std::declval<V>(),             \
                    std::true_type());                                         \
                                                                               \
    template <typename, typename>                                              \
    static auto test(priority_tag<0>) -> std::false_type;                      \
                                                                               \
  public:                                                                      \
    static constexpr bool value =                                              \
        decltype(test<T, Arg>(priority_tag<1>()))::value &&                    \
        !has_native_##op_name<T, Arg>::value;                                  \
  };

#define DEFINE_LAZY_BINARY_OP(op_name, op_symbol)                              \
  template <typename Arg, typename V = Action,                                 \
            typename std::enable_if<                                           \
                queryosity::is_column_v<V> &&                                  \
                    queryosity::is_column_v<typename Arg::action_type> &&      \
                    (detail::has_native_##op_name<                             \
                         column::value_t<V>,                                   \
                         column::value_t<typename Arg::action_type>>::value || \
                     detail::has_custom_only_##op_name<                        \
                         column::value_t<V>,                                   \
                         column::value_t<typename Arg::action_type>>::value),  \
                bool>::type = true>                                            \
  auto operator op_symbol(Arg const &arg) const {                              \
    return this->m_df                                                          \
        ->define(queryosity::column::expression(                               \
            [](column::value_t<V> const &me,                                   \
               column::value_t<typename Arg::action_type> const &you) {        \
              return me op_symbol you;                                         \
            }))                                                                \
        .template evaluate(*this, arg);                                        \
  }

#define CHECK_FOR_UNARY_OP(op_name, op_symbol)                                 \
  struct has_no_##op_name {};                                                  \
  template <typename T> has_no_##op_name operator op_symbol(const T &);        \
  template <typename T> struct has_##op_name {                                 \
    enum {                                                                     \
      value = !std::is_same<decltype(op_symbol std::declval<T>()),             \
                            has_no_##op_name>::value                           \
    };                                                                         \
  };                                                                           \
  template <typename T>                                                        \
  static constexpr bool has_##op_name##_v = has_##op_name<T>::value;

#define DEFINE_LAZY_UNARY_OP(op_name, op_symbol)                               \
  template <                                                                   \
      typename V = Action,                                                     \
      std::enable_if_t<queryosity::is_column_v<V> &&                           \
                           detail::has_##op_name##_v<column::value_t<V>>,      \
                       bool> = false>                                          \
  auto operator op_symbol() const {                                            \
    return queryosity::column::expression(                               \
            [](column::value_t<V> const &me) { return (op_symbol me); })._equate(*this->m_df).                                                          \
        .template evaluate(*this);                                             \
    // return this->m_df                                                          \
    //     ->define(queryosity::column::expression(                               \
    //         [](column::value_t<V> const &me) { return (op_symbol me); }))      \
    //     .template evaluate(*this);                                             \
  }

#define CHECK_FOR_INDEX_OP()                                                   \
  template <class T, class Index> struct has_subscript_impl {                  \
    template <class T1, class IndexDeduced = Index,                            \
              class Reference = decltype((                                     \
                  *std::declval<T *>())[std::declval<IndexDeduced>()]),        \
              typename = typename std::enable_if<                              \
                  !std::is_void<Reference>::value>::type>                      \
    static std::true_type test(int);                                           \
    template <class> static std::false_type test(...);                         \
    using type = decltype(test<T>(0));                                         \
  };                                                                           \
  template <class T, class Index>                                              \
  using has_subscript = typename has_subscript_impl<T, Index>::type;           \
  template <class T, class Index>                                              \
  static constexpr bool has_subscript_v = has_subscript<T, Index>::value;

#define DEFINE_LAZY_INDEX_OP()                                                 \
  template <                                                                   \
      typename Arg, typename V = Action,                                       \
      std::enable_if_t<is_column_v<V> &&                                       \
                           detail::has_subscript_v<                            \
                               column::value_t<V>,                             \
                               column::value_t<typename Arg::action_type>>,    \
                       bool> = false>                                          \
  auto operator[](Arg const &arg) const {                                      \
    return this->m_df                                                          \
        ->define(queryosity::column::expression(                               \
            [](column::value_t<V> me,                                          \
               column::value_t<typename Arg::action_type> index) {             \
              return me[index];                                                \
            }))                                                                \
        .template evaluate(*this, arg);                                        \
  }

#define DECLARE_LAZY_VARIED_BINARY_OP(op_symbol)                               \
  template <                                                                   \
      typename Arg, typename V = Act,                                          \
      std::enable_if_t<queryosity::is_column_v<V> &&                           \
                           queryosity::is_column_v<typename Arg::action_type>, \
                       bool> = false>                                          \
  auto operator op_symbol(Arg const &b) const -> varied<                       \
      lazy<typename decltype(std::declval<queryosity::lazy<Act>>().            \
                             operator op_symbol(b.nominal()))::action_type>>;

#define DEFINE_LAZY_VARIED_BINARY_OP(op_symbol)                                \
  template <typename Act>                                                      \
  template <                                                                   \
      typename Arg, typename V,                                                \
      std::enable_if_t<queryosity::is_column_v<V> &&                           \
                           queryosity::is_column_v<typename Arg::action_type>, \
                       bool>>                                                  \
  auto queryosity::varied<queryosity::lazy<Act>>::operator op_symbol(          \
      Arg const &b) const                                                      \
      -> varied<                                                               \
          lazy<typename decltype(std::declval<lazy<Act>>().operator op_symbol( \
              b.nominal()))::action_type>> {                                   \
    auto syst = varied<                                                        \
        lazy<typename decltype(std::declval<lazy<Act>>().operator op_symbol(   \
            b.nominal()))::action_type>>(                                      \
        this->nominal().operator op_symbol(b.nominal()));                      \
    for (auto const &var_name : systematic::get_variation_names(*this, b)) {   \
      syst.set_variation(var_name, variation(var_name).operator op_symbol(     \
                                       b.variation(var_name)));                \
    }                                                                          \
    return syst;                                                               \
  }

#define DECLARE_LAZY_VARIED_UNARY_OP(op_symbol)                                \
  template <typename V = Act,                                                  \
            std::enable_if_t<queryosity::is_column_v<V>, bool> = false>        \
  auto operator op_symbol() const -> varied<                                   \
      queryosity::lazy<typename decltype(std::declval<lazy<V>>().              \
                                         operator op_symbol())::action_type>>;
#define DEFINE_LAZY_VARIED_UNARY_OP(op_name, op_symbol)                        \
  template <typename Act>                                                      \
  template <typename V, std::enable_if_t<queryosity::is_column_v<V>, bool>>    \
  auto queryosity::varied<queryosity::lazy<Act>>::operator op_symbol() const   \
      -> varied<lazy<typename decltype(std::declval<lazy<V>>().                \
                                       operator op_symbol())::action_type>> {  \
    auto syst = varied<queryosity::lazy<                                       \
        typename decltype(std::declval<lazy<V>>().                             \
                          operator op_symbol())::action_type>>(                \
        this->nominal().operator op_symbol());                                 \
    for (auto const &var_name : systematic::get_variation_names(*this)) {      \
      syst.set_variation(var_name, variation(var_name).operator op_symbol());  \
    }                                                                          \
    return syst;                                                               \
  }

namespace queryosity {

template <typename T> class lazy;
template <typename T> class todo;

template <typename U>
static constexpr std::true_type check_lazy(lazy<U> const &);
static constexpr std::false_type check_lazy(...) { return std::false_type{}; }
template <typename U>
static constexpr std::true_type check_todo(todo<U> const &);
static constexpr std::false_type check_todo(...) { return std::false_type{}; }

template <typename V>
static constexpr bool is_nominal_v =
    (decltype(check_lazy(std::declval<V>()))::value ||
     decltype(check_todo(std::declval<V>()))::value);
template <typename V> static constexpr bool is_varied_v = !is_nominal_v<V>;

template <typename... Args>
static constexpr bool has_no_variation_v = (is_nominal_v<Args> && ...);
template <typename... Args>
static constexpr bool has_variation_v = (is_varied_v<Args> || ...);

namespace detail {

// https://quuxplusone.github.io/blog/2021/07/09/priority-tag/
template <int I> struct priority_tag : priority_tag<I - 1> {};
template <> struct priority_tag<0> {};

CHECK_FOR_UNARY_OP(logical_not, !)
CHECK_FOR_UNARY_OP(minus, -)
CHECK_FOR_BINARY_OP(addition, +)
CHECK_FOR_BINARY_OP(subtraction, -)
CHECK_FOR_BINARY_OP(multiplication, *)
CHECK_FOR_BINARY_OP(division, /)
CHECK_FOR_BINARY_OP(remainder, %)
CHECK_FOR_BINARY_OP(greater_than, >)
CHECK_FOR_BINARY_OP(less_than, <)
CHECK_FOR_BINARY_OP(greater_than_or_equal_to, >=)
CHECK_FOR_BINARY_OP(less_than_or_equal_to, <=)
CHECK_FOR_BINARY_OP(equality, ==)
CHECK_FOR_BINARY_OP(inequality, !=)
CHECK_FOR_BINARY_OP(logical_and, &&)
CHECK_FOR_BINARY_OP(logical_or, ||)
CHECK_FOR_BINARY_OP(bitwise_or, &)
CHECK_FOR_BINARY_OP(bitwise_and, |)
CHECK_FOR_INDEX_OP()

} // namespace detail

} // namespace queryosity

#include <set>
#include <string>
#include <type_traits>

namespace queryosity {

class dataflow;

template <typename U> class systematic::resolver {

public:
  using nominal_type = U;

public:
  template <typename> friend class lazy;
  template <typename> friend class todo;
  template <typename> friend class resolver;

public:
  resolver() = default;
  virtual ~resolver() = default;

public:
  virtual void set_variation(const std::string &var_name, U var) = 0;

  virtual U &nominal() = 0;
  virtual U &variation(const std::string &var_name) = 0;

  virtual U const &nominal() const = 0;
  virtual U const &variation(const std::string &var_name) const = 0;

  virtual bool has_variation(const std::string &var_name) const = 0;
  virtual std::set<std::string> get_variation_names() const = 0;
};

} // namespace queryosity

namespace queryosity {

/**
 * @ingroup api
 * @brief Lazy action over dataset.
 * @tparam Action Action to be performed.
 */
template <typename Action>
class lazy : public dataflow::node,
             public ensemble::slotted<Action>,
             public systematic::resolver<lazy<Action>>,
             public query::result_of<Action> {

public:
  using action_type = Action;

public:
  friend class dataflow;
  template <typename> friend class lazy;
  template <typename> friend class varied;
  template <typename> friend struct column::series; // access to dataflow

public:
  lazy(dataflow &df, std::vector<Action *> const &slots)
      : dataflow::node(df), m_slots(slots) {}

  template <typename Derived>
  lazy(dataflow &df, std::vector<Derived *> const &slots);
  template <typename Derived>
  lazy(dataflow &df, std::vector<std::unique_ptr<Derived>> const &slots);

  template <typename Base> operator lazy<Base>() const;

  lazy(const lazy &) = default;
  lazy &operator=(const lazy &) = default;

  lazy(lazy &&) = default;
  lazy &operator=(lazy &&) = default;

  virtual ~lazy() = default;

  virtual std::vector<Action *> const &get_slots() const final override;

  virtual void set_variation(const std::string &var_name,
                             lazy var) final override;

  virtual lazy &nominal() final override;
  virtual lazy &variation(const std::string &var_name) final override;
  virtual lazy const &nominal() const final override;
  virtual lazy const &
  variation(const std::string &var_name) const final override;

  virtual bool has_variation(const std::string &var_name) const final override;
  virtual std::set<std::string> get_variation_names() const final override;

  template <typename To, typename V = Action,
            std::enable_if_t<queryosity::is_column_v<V>, bool> = false>
  auto to() const -> lazy<column::valued<To>>;

  /**
   * @brief Compound a weight to from this selection.
   * @tparam Col Column type.
   * @param[in] column Input lazy column used as weight decision.
   * @details The current selection becomes its prerequisite in the cutflow: all
   * prerequisites must pass in order for a downstream selection to pass.
   * @return Compounded lazy weight.
   */
  template <typename Col> auto filter(lazy<Col> const &column) const;

  /**
   * @brief Compound a weight to from this selection.
   * @tparam Col Column type.
   * @param[in] column Input lazy column used as weight decision.
   * @return Compounded lazy weight.
   */
  template <typename Col> auto weight(lazy<Col> const &column) const;

  /**
   * @brief Compound a varied cut to this selection.
   * @tparam Col Varied lazy column type.
   * @param[in] column Input varied column used as cut decision.
   * @return Varied lazy cut.
   */
  template <typename Col> auto filter(Col const &column) const;

  /**
   * @brief Compound a varied weight to this selection.
   * @tparam Col Varied lazy column type.
   * @param[in] column Input varied column used as weight decision.
   * @return Varied lazy weight.
   */
  template <typename Col> auto weight(Col const &column) const;

  /**
   * @brief Compound a cut to this selection.
   * @tparam Expr Callable type (C++ function, functor, lambda, etc.).
   * @param[in] column Input lazy column used as cut decision.
   * @return Selection evaluator.
   */
  template <typename Expr>
  auto filter(queryosity::column::expression<Expr> const &expr) const;

  /**
   * @brief Compound a weight to from this selection.
   * @tparam Expr Callable type (C++ function, functor, lambda, etc.).
   * @param[in] expr Expression used to evaluate the weight decision.
   * @return Selection evaluator.
   */
  template <typename Expr>
  auto weight(queryosity::column::expression<Expr> const &expr) const;

  /**
   * @brief Book a query at this selection.
   * @tparam Qry (Varied) query booker type.
   * @param[in] qry Query booker.
   * @details The query booker should have already been filled with input
   * columns (if applicable).
   * @return (Varied) lazy query.
   */
  template <typename Qry> auto book(Qry &&qry) const;

  /**
   * @brief Book multiple queries at this selection.
   * @tparam Qrys (Varied) query booker types.
   * @param[in] qrys Query bookers.
   * @details The query bookers should have already been filled with input
   * columns (if applicable).
   * @return (Varied) lazy queries.
   */
  template <typename... Qrys> auto book(Qrys &&...qrys) const;

  /**
   * @brief Get a column series for the entries passing the selection.
   * @tparam Col Lazy column type.
   * @return Lazy query.
   * @attention The weight value does not apply in the population of the
   * series.
   */
  template <typename Col,
            std::enable_if_t<queryosity::is_nominal_v<Col>, bool> = false>
  auto get(column::series<Col> const &col)
      -> lazy<query::series<typename column::series<Col>::value_type>>;

  /**
   * @brief Get a column series for the entries passing the selection.
   * @tparam Col Varied column type.
   * @return Varied lazy query.
   * @attention The weight value does not apply in the population of the
   * series.
   */
  template <typename Col,
            std::enable_if_t<queryosity::is_varied_v<Col>, bool> = false>
  auto get(column::series<Col> const &col)
      -> varied<lazy<query::series<typename column::series<Col>::value_type>>>;

  /**
   * @brief (Process and) retrieve the result of a query.
   * @return Query result.
   * @attention Invoking this turns *all* lazy actions in the dataflow *eager*.
   */
  template <
      typename V = Action,
      std::enable_if_t<queryosity::query::has_result_v<V>, bool> = false>
  auto result() -> decltype(std::declval<V>().result());

  /**
   * @brief Shortcut for `result()`.
   */
  template <
      typename V = Action,
      std::enable_if_t<queryosity::query::has_result_v<V>, bool> = false>
  auto operator->() -> decltype(std::declval<V>().result()) {
    return this->result();
  }

  DEFINE_LAZY_UNARY_OP(logical_not, !)
  DEFINE_LAZY_UNARY_OP(minus, -)
  DEFINE_LAZY_BINARY_OP(equality, ==)
  DEFINE_LAZY_BINARY_OP(inequality, !=)
  DEFINE_LAZY_BINARY_OP(addition, +)
  DEFINE_LAZY_BINARY_OP(subtraction, -)
  DEFINE_LAZY_BINARY_OP(multiplication, *)
  DEFINE_LAZY_BINARY_OP(division, /)
  DEFINE_LAZY_BINARY_OP(logical_and, &&)
  DEFINE_LAZY_BINARY_OP(logical_or, ||)
  DEFINE_LAZY_BINARY_OP(bitwise_and, &)
  DEFINE_LAZY_BINARY_OP(bitwise_or, |)
  DEFINE_LAZY_BINARY_OP(greater_than, >)
  DEFINE_LAZY_BINARY_OP(less_than, <)
  DEFINE_LAZY_BINARY_OP(greater_than_or_equal_to, >=)
  DEFINE_LAZY_BINARY_OP(less_than_or_equal_to, <=)
  DEFINE_LAZY_INDEX_OP()

protected:
  template <
      typename V = Action,
      std::enable_if_t<queryosity::query::has_result_v<V>, bool> = false>
  void merge_results();

protected:
  std::vector<Action *> m_slots;
};

} // namespace queryosity

#include <set>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace queryosity {

template <typename T> class varied;

/**
 * @ingroup api
 * @brief Variations of a lazy action.
 * @tparam Action Action to be performed.
 * @details A varied lazy action encapsulates independent nominal and variation
 * instances of of a lazy action, which are implicitly propagated through all
 * downstream actions that it participates in. In other words, a varied node
 * behaves functionally identical to a nominal-only one.
 */
template <typename Act>
class varied<lazy<Act>> : public dataflow::node,
                          public systematic::resolver<lazy<Act>> {

public:
  using action_type = typename lazy<Act>::action_type;

  template <typename> friend class lazy;
  template <typename> friend class varied;

public:
  varied(lazy<Act> nom);
  virtual ~varied() = default;

  varied(varied const &) = default;
  varied &operator=(varied const &) = default;

  varied(varied &&) = default;
  varied &operator=(varied &&) = default;

  template <typename Derived> varied(varied<lazy<Derived>> const &);
  template <typename Derived> varied &operator=(varied<lazy<Derived>> const &);

  virtual void set_variation(const std::string &var_name,
                             lazy<Act> var) final override;

  virtual lazy<Act> &nominal() final override;
  virtual lazy<Act> &variation(const std::string &var_name) final override;
  virtual lazy<Act> const &nominal() const final override;
  virtual lazy<Act> const &
  variation(const std::string &var_name) const final override;

  virtual bool has_variation(const std::string &var_name) const final override;
  virtual std::set<std::string> get_variation_names() const final override;

  /**
   * @brief Compound a cut to this selection.
   * @Col (Varied) lazy input column type.
   * @parma[in] col (Varied) lazy input column used as cut decision.
   * @return Varied lazy selection.
   */
  template <typename Col, typename V = Act,
            std::enable_if_t<queryosity::is_selection_v<V>, bool> = false>
  auto filter(Col const &col) -> varied<lazy<selection::node>>;

  /**
   * @brief Compound a weight to this selection.
   * @Col (Varied) lazy input column type.
   * @parma[in] col (Varied) lazy input column used as cut decision.
   * @return Varied lazy selection.
   */
  template <typename Col, typename V = Act,
            std::enable_if_t<queryosity::is_selection_v<V>, bool> = false>
  auto weight(Col const &col) -> varied<lazy<selection::node>>;

  template <typename Expr, typename V = Act,
            std::enable_if_t<queryosity::is_selection_v<V>, bool> = false>
  auto filter(column::expression<Expr> const &expr) -> varied<
      todo<selection::applicator<selection::cut, column::equation_t<Expr>>>>;

  template <typename Expr, typename V = Act,
            std::enable_if_t<queryosity::is_selection_v<V>, bool> = false>
  auto weight(column::expression<Expr> const &expr) -> varied<
      todo<selection::applicator<selection::weight, column::equation_t<Expr>>>>;

  template <typename Agg, typename V = Act,
            std::enable_if_t<queryosity::is_selection_v<V>, bool> = false>
  auto book(Agg &&agg);

  template <typename... Aggs, typename V = Act,
            std::enable_if_t<queryosity::is_selection_v<V>, bool> = false>
  auto book(Aggs &&...aggs);

  template <
      typename V = Act,
      std::enable_if_t<queryosity::query::has_result_v<V>, bool> = false>
  auto operator[](const std::string &var_name) -> lazy<V> &;
  template <
      typename V = Act,
      std::enable_if_t<queryosity::query::has_result_v<V>, bool> = false>
  auto operator[](const std::string &var_name) const -> lazy<V> const &;

  DECLARE_LAZY_VARIED_UNARY_OP(-)
  DECLARE_LAZY_VARIED_UNARY_OP(!)

  DECLARE_LAZY_VARIED_BINARY_OP(+)
  DECLARE_LAZY_VARIED_BINARY_OP(-)
  DECLARE_LAZY_VARIED_BINARY_OP(*)
  DECLARE_LAZY_VARIED_BINARY_OP(/)
  DECLARE_LAZY_VARIED_BINARY_OP(<)
  DECLARE_LAZY_VARIED_BINARY_OP(>)
  DECLARE_LAZY_VARIED_BINARY_OP(<=)
  DECLARE_LAZY_VARIED_BINARY_OP(>=)
  DECLARE_LAZY_VARIED_BINARY_OP(==)
  DECLARE_LAZY_VARIED_BINARY_OP(&&)
  DECLARE_LAZY_VARIED_BINARY_OP(||)
  DECLARE_LAZY_VARIED_BINARY_OP([])

protected:
  lazy<Act> m_nom;
  std::unordered_map<std::string, lazy<Act>> m_var_map;
  std::set<std::string> m_var_names;
};

} // namespace queryosity

template <typename Act>
queryosity::varied<queryosity::lazy<Act>>::varied(queryosity::lazy<Act> nom)
    : dataflow::node(*nom.m_df), m_nom(std::move(nom)) {}

template <typename Act>
template <typename Derived>
queryosity::varied<queryosity::lazy<Act>>::varied(
    varied<queryosity::lazy<Derived>> const &other) {
  this->m_df = other.m_df;
  this->m_var_names = other.m_var_names;
  for (auto const &var : other.m_var_map) {
    m_var_map.insert(var);
  }
}

template <typename Act>
template <typename Derived>
queryosity::varied<queryosity::lazy<Act>> &
queryosity::varied<queryosity::lazy<Act>>::operator=(
    varied<queryosity::lazy<Derived>> const &other) {
  this->m_df = other.m_df;
  this->m_var_names = other.m_var_names;
  for (auto const &var : other.m_var_map) {
    m_var_map.insert(var);
  }
  return *this;
}

template <typename Act>
void queryosity::varied<queryosity::lazy<Act>>::set_variation(
    const std::string &var_name, queryosity::lazy<Act> var) {
  m_var_map.insert(std::make_pair(var_name, std::move(var)));
  m_var_names.insert(var_name);
}

template <typename Act>
auto queryosity::varied<queryosity::lazy<Act>>::nominal()
    -> queryosity::lazy<Act> & {
  return this->m_nom;
}

template <typename Act>
auto queryosity::varied<queryosity::lazy<Act>>::variation(
    const std::string &var_name) -> queryosity::lazy<Act> & {
  return (this->has_variation(var_name) ? m_var_map.at(var_name) : m_nom);
}

template <typename Act>
auto queryosity::varied<queryosity::lazy<Act>>::nominal() const
    -> queryosity::lazy<Act> const & {
  return this->m_nom;
}

template <typename Act>
auto queryosity::varied<queryosity::lazy<Act>>::variation(
    const std::string &var_name) const -> queryosity::lazy<Act> const & {
  return (this->has_variation(var_name) ? m_var_map.at(var_name) : m_nom);
}

template <typename Act>
bool queryosity::varied<queryosity::lazy<Act>>::has_variation(
    const std::string &var_name) const {
  return m_var_map.find(var_name) != m_var_map.end();
}

template <typename Act>
std::set<std::string>
queryosity::varied<queryosity::lazy<Act>>::get_variation_names() const {
  return m_var_names;
}

template <typename Act>
template <typename Col, typename V,
          std::enable_if_t<queryosity::is_selection_v<V>, bool>>
auto queryosity::varied<queryosity::lazy<Act>>::filter(Col const &col)
    -> varied<lazy<selection::node>> {

  using varied_type = varied<lazy<selection::node>>;

  auto syst = varied_type(this->nominal().filter(col.nominal()));

  for (auto const &var_name : systematic::get_variation_names(*this, col)) {
    syst.set_variation(
        var_name, this->variation(var_name).filter(col.variation(var_name)));
  }
  return syst;
}

template <typename Act>
template <typename Col, typename V,
          std::enable_if_t<queryosity::is_selection_v<V>, bool>>
auto queryosity::varied<queryosity::lazy<Act>>::weight(Col const &col)
    -> varied<lazy<selection::node>> {

  using varied_type = varied<lazy<selection::node>>;

  auto syst = varied_type(this->nominal().weight(col.nominal()));

  for (auto const &var_name : systematic::get_variation_names(*this, col)) {
    syst.set_variation(
        var_name, this->variation(var_name).weight(col.variation(var_name)));
  }
  return syst;
}

template <typename Act>
template <typename Expr, typename V,
          std::enable_if_t<queryosity::is_selection_v<V>, bool>>
auto queryosity::varied<queryosity::lazy<Act>>::filter(
    queryosity::column::expression<Expr> const &expr)
    -> varied<
        todo<selection::applicator<selection::cut, column::equation_t<Expr>>>> {

  using varied_type = varied<
      todo<selection::applicator<selection::cut, column::equation_t<Expr>>>>;

  auto syst = varied_type(this->nominal().filter(expr));

  for (auto const &var_name : systematic::get_variation_names(*this)) {
    syst.set_variation(var_name, this->variation(var_name).filter(expr));
  }
  return syst;
}

template <typename Act>
template <typename Expr, typename V,
          std::enable_if_t<queryosity::is_selection_v<V>, bool>>
auto queryosity::varied<queryosity::lazy<Act>>::weight(
    queryosity::column::expression<Expr> const &expr)
    -> varied<todo<
        selection::applicator<selection::weight, column::equation_t<Expr>>>> {

  using varied_type = varied<
      todo<selection::applicator<selection::weight, column::equation_t<Expr>>>>;

  auto syst = varied_type(this->nominal().weight(expr));

  for (auto const &var_name : systematic::get_variation_names(*this)) {
    syst.set_variation(var_name, this->variation(var_name).weight(expr));
  }
  return syst;
}

template <typename Act>
template <typename Agg, typename V,
          std::enable_if_t<queryosity::is_selection_v<V>, bool>>
auto queryosity::varied<queryosity::lazy<Act>>::book(Agg &&agg) {
  return agg.at(*this);
}

template <typename Act>
template <typename... Aggs, typename V,
          std::enable_if_t<queryosity::is_selection_v<V>, bool>>
auto queryosity::varied<queryosity::lazy<Act>>::book(Aggs &&...aggs) {
  return std::make_tuple((aggs.at(*this), ...));
}

template <typename Act>
template <typename V,
          std::enable_if_t<queryosity::query::has_result_v<V>, bool>>
auto queryosity::varied<queryosity::lazy<Act>>::operator[](
    const std::string &var_name) -> queryosity::lazy<V> & {
  if (!this->has_variation(var_name)) {
    throw std::out_of_range("variation does not exist");
  }
  return this->variation(var_name);
}

template <typename Act>
template <typename V,
          std::enable_if_t<queryosity::query::has_result_v<V>, bool>>
auto queryosity::varied<queryosity::lazy<Act>>::operator[](
    const std::string &var_name) const -> queryosity::lazy<V> const & {
  if (!this->has_variation(var_name)) {
    throw std::out_of_range("variation does not exist");
  }
  return this->variation(var_name);
}

DEFINE_LAZY_VARIED_UNARY_OP(minus, -)
DEFINE_LAZY_VARIED_UNARY_OP(logical_not, !)

DEFINE_LAZY_VARIED_BINARY_OP(+)
DEFINE_LAZY_VARIED_BINARY_OP(-)
DEFINE_LAZY_VARIED_BINARY_OP(*)
DEFINE_LAZY_VARIED_BINARY_OP(/)
DEFINE_LAZY_VARIED_BINARY_OP(<)
DEFINE_LAZY_VARIED_BINARY_OP(>)
DEFINE_LAZY_VARIED_BINARY_OP(<=)
DEFINE_LAZY_VARIED_BINARY_OP(>=)
DEFINE_LAZY_VARIED_BINARY_OP(==)
DEFINE_LAZY_VARIED_BINARY_OP(&&)
DEFINE_LAZY_VARIED_BINARY_OP(||)
DEFINE_LAZY_VARIED_BINARY_OP([])

namespace queryosity {

/**
 * @brief Minimal query with an output result.
 * @details This ABC should be used for actions that do not require any input
 * columns.
 */
template <typename T> class query::aggregation : public node {

public:
  using result_type = T;

public:
  aggregation() = default;
  virtual ~aggregation() = default;

  /**
   * Create and return the result of the query.
   * If multiple slots are run concurrently in multithreaded mode, the results
   * are merged into one (see `merge`).
   * @return The result.
   */
  virtual T result() const = 0;

  /**
   * Merge the results from concurrent slots into one representing the full
   * dataset.
   * @param[in] results Partial result from each thread.
   * @return Merged result.
   */
  virtual T merge(std::vector<T> const &results) const = 0;

  using node::count;

  /**
   * Shortcut for `result()`.
   * @return The result.
   */
  T operator->() const { return this->result(); }

protected:
};

} // namespace queryosity

namespace queryosity {

namespace column {

template <typename T> class observable;

template <typename T> class variable;

template <typename T> class view;

}

/**
 * @brief Query filled with column value(s) per-entry.
 * @tparam Out Output result type.
 * @tparam Ins Input column data types.
 */
template <typename... Ins>
class query::fillable {

public:
  using vartup_type = std::tuple<column::variable<Ins>...>;

public:
  fillable() = default;
  virtual ~fillable() = default;

  /**
   * @brief Perform the counting action for an entry.
   * @param[in] observables Input column observables.
   * @param[in] weight The weight value of the booked selection for the passed
   * entry.
   * @details This action is performed N times for a passed entry, where N is
   * the number of `fill()` calls made to its lazy node.
   */
  virtual void fill(column::observable<Ins>... observables, double weight) = 0;

  template <typename... Vals>
  void enter_columns(column::view<Vals> const &...cols);

protected:
  std::vector<vartup_type> m_fills;
};

} // namespace queryosity

template <typename... Ins>
template <typename... Vals>
void queryosity::query::fillable<Ins...>::enter_columns(
    column::view<Vals> const &...cols) {
  static_assert(sizeof...(Ins) == sizeof...(Vals),
                "dimension mis-match between filled variables & columns.");
  m_fills.emplace_back(cols...);
}

namespace queryosity {

/**
 * @brief Query filled with column value(s) per-entry.
 * @tparam Out Output result type.
 * @tparam Ins Input column data types.
 */
template <typename Out, typename... Ins>
class query::definition<Out(Ins...)> : public query::aggregation<Out>,
                                       public query::fillable<Ins...> {

public:
  using vartup_type = std::tuple<column::variable<Ins>...>;

public:
  definition() = default;
  virtual ~definition() = default;

  /**
   * @brief Perform the counting action for an entry.
   * @param[in] observables Input column observables.
   * @param[in] weight The weight value of the booked selection for the passed
   * entry.
   * @details This action is performed N times for a passed entry, where N is
   * the number of `fill()` calls made to its lazy node.
   */
  virtual void count(double w) final override;
};

} // namespace queryosity

template <typename Out, typename... Ins>
void queryosity::query::definition<Out(Ins...)>::count(double w) {
  for (unsigned int ifill = 0; ifill < this->m_fills.size(); ++ifill) {
    std::apply(
        [this, w](const column::variable<Ins> &...obs) {
          this->fill(obs..., w);
        },
        this->m_fills[ifill]);
  }
}

#include <vector>

namespace queryosity
{

namespace query
{

template <typename T> class series : public queryosity::query::definition<std::vector<T>(T)>
{

  public:
    series() = default;
    ~series() = default;

    virtual void initialize(unsigned int, unsigned long long, unsigned long long) final override;
    virtual void fill(column::observable<T>, double) final override;
    virtual void finalize(unsigned int) final override;
    virtual std::vector<T> result() const final override;
    virtual std::vector<T> merge(std::vector<std::vector<T>> const &results) const final override;

  protected:
    std::vector<T> m_result;
};

} // namespace query

} // namespace queryosity

template <typename T>
void queryosity::query::series<T>::initialize(unsigned int, unsigned long long begin, unsigned long long end)
{
    m_result.reserve(end - begin);
}

template <typename T> void queryosity::query::series<T>::fill(column::observable<T> x, double)
{
    m_result.push_back(x.value());
}

template <typename T> void queryosity::query::series<T>::finalize(unsigned int)
{
    m_result.resize(m_result.size());
}

template <typename T> std::vector<T> queryosity::query::series<T>::result() const
{
    return m_result;
}

template <typename T>
std::vector<T> queryosity::query::series<T>::merge(std::vector<std::vector<T>> const &results) const
{
    std::vector<T> merged;
    size_t merged_size = 0;
    for (auto const &result : results)
    {
        merged_size += result.size();
    }
    merged.reserve(merged_size);
    for (auto const &result : results)
    {
        merged.insert(merged.end(), result.begin(), result.end());
    }
    return merged;
}

namespace queryosity {

namespace column {

/**
 * @brief Argumnet for column series.
 * @tparam Col (Varied) lazy column node.
 * @todo C++20: Use concept to require lazy<column<Val>(::varied)>.
 */
template <typename Col> struct series {

public:
  using value_type = column::value_t<typename Col::action_type>;

public:
  series(Col const &col);
  ~series() = default;

  auto make(dataflow &df) const;

  auto make(lazy<selection::node> &sel) const;

  auto make(varied<lazy<selection::node>> &sel) const ->
      varied<lazy<query::series<value_type>>>;

protected:
  Col m_column;
};

} // namespace column

} // namespace queryosity

template <typename Col>
queryosity::column::series<Col>::series(Col const &col) : m_column(col){};

template <typename Col>
auto queryosity::column::series<Col>::make(dataflow &df) const {
  return df.get(query::output<query::series<value_type>>()).fill(m_column);
}

template <typename Col>
auto queryosity::column::series<Col>::make(lazy<selection::node> &sel) const {
  auto df = sel.m_df;
  return df->get(query::output<query::series<value_type>>())
      .fill(m_column)
      .at(sel);
}

template <typename Col>
auto queryosity::column::series<Col>::make(varied<lazy<selection::node>> &sel)
    const -> varied<lazy<query::series<value_type>>> {
  auto df = sel.nominal().m_df;
  return df->get(query::output<query::series<value_type>>())
      .fill(m_column)
      .at(sel);
}

template <typename Action>
template <typename Derived>
queryosity::lazy<Action>::lazy(dataflow &df,
                               std::vector<Derived *> const &slots)
    : dataflow::node(df) {
  m_slots.clear();
  m_slots.reserve(slots.size());
  for (auto slot : slots) {
    m_slots.push_back(static_cast<Action *>(slot));
  }
}

template <typename Action>
template <typename Derived>
queryosity::lazy<Action>::lazy(
    dataflow &df, std::vector<std::unique_ptr<Derived>> const &slots)
    : dataflow::node(df) {
  m_slots.clear();
  m_slots.reserve(slots.size());
  for (auto const &slot : slots) {
    m_slots.push_back(static_cast<Action *>(slot.get()));
  }
}

template <typename Action>
template <typename Base>
queryosity::lazy<Action>::operator lazy<Base>() const {
  return lazy<Base>(*this->m_df, this->m_slots);
}

template <typename Action>
std::vector<Action *> const &queryosity::lazy<Action>::get_slots() const {
  return this->m_slots;
}

template <typename Action>
void queryosity::lazy<Action>::set_variation(const std::string &, lazy) {
  // should never be called
  throw std::logic_error("cannot set variation to a nominal-only action");
}

template <typename Action> auto queryosity::lazy<Action>::nominal() -> lazy & {
  // this is nominal
  return *this;
}

template <typename Action>
auto queryosity::lazy<Action>::variation(const std::string &) -> lazy & {
  // propagation of variations must occur "transparently"
  return *this;
}

template <typename Action>
auto queryosity::lazy<Action>::nominal() const -> lazy const & {
  // this is nominal
  return *this;
}

template <typename Action>
auto queryosity::lazy<Action>::variation(const std::string &) const
    -> lazy const & {
  // propagation of variations must occur "transparently"
  return *this;
}

template <typename Action>
std::set<std::string> queryosity::lazy<Action>::get_variation_names() const {
  // no variations to list
  return std::set<std::string>();
}

template <typename Action>
bool queryosity::lazy<Action>::has_variation(const std::string &) const {
  // always false
  return false;
}

template <typename Action>
template <typename To, typename V,
          std::enable_if_t<queryosity::is_column_v<V>, bool>>
auto queryosity::lazy<Action>::to() const -> lazy<column::valued<To>> {
  if constexpr (std::is_same_v<To, column::value_t<V>> ||
                std::is_base_of_v<To, column::value_t<V>>) {
    return lazy<column::valued<To>>(*this->m_df, this->get_slots());
  } else {
    return lazy<column::valued<To>>(
        *this->m_df, this->m_df->template _convert<To>(*this).get_slots());
  }
}

template <typename Action>
template <typename Col>
auto queryosity::lazy<Action>::filter(lazy<Col> const &col) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    return this->m_df->template _apply<selection::cut>(*this, col);
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "filter must be called from a selection");
  }
}

template <typename Action>
template <typename Col>
auto queryosity::lazy<Action>::weight(lazy<Col> const &col) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    return this->m_df->template _apply<selection::weight>(*this, col);
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "filter must be called from a selection");
  }
}

template <typename Action>
template <typename Col>
auto queryosity::lazy<Action>::filter(Col const &col) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    using varied_type = varied<lazy<selection::node>>;
    auto syst = varied_type(this->filter(col.nominal()));
    for (auto const &var_name : col.get_variation_names()) {
      syst.set_variation(var_name, this->filter(col.variation(var_name)));
    }
    return syst;
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "filter must be called from a selection");
  }
}

template <typename Action>
template <typename Col>
auto queryosity::lazy<Action>::weight(Col const &col) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    using varied_type = varied<lazy<selection::node>>;
    auto syst = varied_type(this->weight(col.nominal()));
    for (auto const &var_name : col.get_variation_names()) {
      syst.set_variation(var_name, this->weight(col.variation(var_name)));
    }
    return syst;
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "weight must be called from a selection");
  }
}

template <typename Action>
template <typename Expr>
auto queryosity::lazy<Action>::filter(
    queryosity::column::expression<Expr> const &expr) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    return this->m_df->template _select<selection::cut>(*this, expr);
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "filter must be called from a selection");
  }
}

template <typename Action>
template <typename Expr>
auto queryosity::lazy<Action>::weight(
    queryosity::column::expression<Expr> const &expr) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    return this->m_df->template _select<selection::weight>(*this, expr);
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "filter must be called from a selection");
  }
}

template <typename Action>
template <typename Qry>
auto queryosity::lazy<Action>::book(Qry &&qry) const {
  static_assert(std::is_base_of_v<selection::node, Action>,
                "book must be called from a selection");
  return qry.at(*this);
}

template <typename Action>
template <typename... Qrys>
auto queryosity::lazy<Action>::book(Qrys &&...qrys) const {
  static_assert(std::is_base_of_v<selection::node, Action>,
                "book must be called from a selection");
  return std::make_tuple((qrys.at(*this), ...));
}

template <typename Action>
template <typename Col, std::enable_if_t<queryosity::is_nominal_v<Col>, bool>>
auto queryosity::lazy<Action>::get(queryosity::column::series<Col> const &col)
    -> lazy<query::series<typename column::series<Col>::value_type>> {
  return col.make(*this);
}

template <typename Action>
template <typename Col, std::enable_if_t<queryosity::is_varied_v<Col>, bool>>
auto queryosity::lazy<Action>::get(queryosity::column::series<Col> const &col)
    -> varied<lazy<query::series<typename column::series<Col>::value_type>>> {
  return col.make(*this);
}

template <typename Action>
template <typename V,
          std::enable_if_t<queryosity::query::has_result_v<V>, bool>>
auto queryosity::lazy<Action>::result()
    -> decltype(std::declval<V>().result()) {
  this->m_df->analyze();
  this->merge_results();
  return this->m_result;
}

template <typename Action>
template <typename V,
          std::enable_if_t<queryosity::query::has_result_v<V>, bool> e>
void queryosity::lazy<Action>::merge_results() {
  if (this->m_merged)
    return;
  auto model = this->get_slot(0);
  using result_type = decltype(model->result());
  const auto nslots = this->size();
  if (nslots == 1) {
    this->m_result = std::move(model->result());
  } else {
    std::vector<result_type> results;
    results.reserve(nslots);
    for (size_t islot = 0; islot < nslots; ++islot) {
      results.push_back(std::move(this->get_slot(islot)->result()));
    }
    this->m_result = std::move(model->merge(results));
  }
  this->m_merged = true;
}

template <typename DS>
queryosity::dataflow::input<DS>::loaded(queryosity::dataflow &df, DS &ds)
    : m_df(&df), m_ds(&ds) {}

template <typename DS>
template <typename Val>
auto queryosity::dataflow::input<DS>::read(dataset::column<Val> const &col)
    -> lazy<queryosity::column::valued<Val>> {
  return col.template _read(*this);
}

template <typename DS>
template <typename... Vals>
auto queryosity::dataflow::input<DS>::read(
    dataset::column<Vals> const &...cols) {
  return std::make_tuple(cols.template _read(*this)...);
}

template <typename DS>
template <typename Val>
auto queryosity::dataflow::input<DS>::vary(
    dataset::column<Val> const &col,
    std::map<std::string, std::string> const &vars) {
  auto nom = this->read(col);
  varied<decltype(nom)> varied_column(std::move(nom));
  for (auto const &var : vars) {
    varied_column.set_variation(var.first,
                                this->read(dataset::column<Val>(var.second)));
  }
  return varied_column;
}

namespace queryosity {

class dataflow;

template <typename Val> class lazy;

namespace column {

/**
 * @ingroup api
 * @brief Argument to define a column of constant value in the dataflow.
 * @tparam Val Data type of the constant value.
 */
template <typename Val> struct constant {

public:
  /**
   * @brief Argument constructor.
   * @param[in] val Constant value.
   */
  constant(Val const &val);
  ~constant() = default;

  auto _assign(dataflow &df) const -> lazy<column::valued<Val>>;

protected:
  Val m_val;
};

} // namespace column

} // namespace queryosity

template <typename Val>
queryosity::column::constant<Val>::constant(Val const &val) : m_val(val) {}

template <typename Val>
auto queryosity::column::constant<Val>::_assign(queryosity::dataflow &df) const
    -> lazy<column::valued<Val>> {
  return df._assign(this->m_val);
}

#include <string>
#include <tuple>

namespace queryosity {

class dataflow;

template <typename T> class lazy;

namespace selection {

class node;

}

namespace column {

/**
 * @brief Argument to define a column evaluated out of an expression in the
 * dataflow.
 * @tparam Expr Concrete type of C++ function, functor, or lambda.
 */
template <typename Expr> struct expression {

public:
  using function_type = decltype(std::function(std::declval<Expr>()));
  using equation_type = equation_t<Expr>;

public:
  /**
   * @brief Argument constructor.
   * @param[in] expr The callable expression.
   */
  expression(Expr expr);
  ~expression() = default;

  auto _equate(dataflow &df) const;

  template <typename Sel> auto _select(dataflow &df) const;

  template <typename Sel>
  auto _select(dataflow &df, lazy<selection::node> const &presel) const;

protected:
  function_type m_expression;
};

} // namespace column

} // namespace queryosity

template <typename Expr>
queryosity::column::expression<Expr>::expression(Expr expr)
    : m_expression(std::move(expr)) {}

template <typename Expr>
auto queryosity::column::expression<Expr>::_equate(
    queryosity::dataflow &df) const {
  return df._equate(this->m_expression);
}

template <typename Expr>
template <typename Sel>
auto queryosity::column::expression<Expr>::_select(
    queryosity::dataflow &df) const {
  return df._select<Sel>(this->m_expression);
}

template <typename Expr>
template <typename Sel>
auto queryosity::column::expression<Expr>::_select(
    queryosity::dataflow &df, lazy<selection::node> const &presel) const {
  return df._select<Sel>(presel, this->m_expression);
}

namespace queryosity {

template <typename> class lazy;

namespace column {

template <typename Col> struct nominal {

public:
  using column_type = valued<value_t<Col>>;

public:
  nominal(lazy<Col> const &nom);
  ~nominal() = default;

  auto get() const -> lazy<valued<value_t<Col>>> const &;

protected:
  lazy<valued<value_t<Col>>> const &m_nom;
};

} // namespace systematic

} // namespace queryosity

template <typename Col>
queryosity::column::nominal<Col>::nominal(lazy<Col> const &nom) : m_nom(nom) {}

template <typename Col>
auto queryosity::column::nominal<Col>::get() const -> lazy<valued<value_t<Col>>> const & {
  return m_nom;
}

#include <set>
#include <string>
#include <type_traits>

namespace queryosity {

namespace column {

template <typename Val> struct variation {

public:
  template <typename Col>
  variation(lazy<Col> const& var);
  ~variation() = default;

  auto get() const -> lazy<valued<Val>> const &;

protected:
  lazy<valued<Val>> m_var;
};

}

} // namespace queryosity

template <typename Val>
template <typename Col>
queryosity::column::variation<Val>::variation(queryosity::lazy<Col> const& var) : m_var(var.template to<Val>()) {}

template <typename Val>
auto queryosity::column::variation<Val>::get() const
    -> lazy<valued<Val>> const & {
  return m_var;
}

#include <functional>
#include <memory>
#include <tuple>

namespace queryosity {

class dataflow;

namespace query {

/**
 * @ingroup api
 * @brief Argument to specify a query in the dataflow.
 * @tparam Qry Concrete implementation of
 * `queryosity::query::definition<T(Obs...)>`.
 */
template <typename Qry> struct output {

public:
  /**
   * @brief Argument constructor.
   * @tparam Args Constructor argument types for @p Qry.
   * @param args Constructor arguments for @p Qry.
   */
  template <typename... Args> output(Args const &...args);
  ~output() = default;

  auto make(dataflow &df) const;

protected:
  std::function<todo<query::booker<Qry>>(dataflow &)> m_make;
};

} // namespace query

} // namespace queryosity

template <typename Qry>
template <typename... Args>
queryosity::query::output<Qry>::output(Args const &...args)
    : m_make([args...](dataflow &df) { return df._make<Qry>(args...); }) {}

template <typename Qry>
auto queryosity::query::output<Qry>::make(queryosity::dataflow &df) const {
  return this->m_make(df);
}

inline queryosity::dataflow::dataflow()
    : m_processor(multithread::disable()), m_weight(1.0), m_nrows(-1),
      m_analyzed(false) {}

template <typename Kwd>
queryosity::dataflow::dataflow(Kwd &&kwarg) : dataflow() {
  this->accept_kwarg(std::forward<Kwd>(kwarg));
}

template <typename Kwd1, typename Kwd2>
queryosity::dataflow::dataflow(Kwd1 &&kwarg1, Kwd2 &&kwarg2) : dataflow() {
  static_assert(!std::is_same_v<Kwd1, Kwd2>, "each keyword argument must be unique");
  this->accept_kwarg(std::forward<Kwd1>(kwarg1));
  this->accept_kwarg(std::forward<Kwd2>(kwarg2));
}

template <typename Kwd1, typename Kwd2, typename Kwd3>
queryosity::dataflow::dataflow(Kwd1 &&kwarg1, Kwd2 &&kwarg2, Kwd3 &&kwarg3)
    : dataflow() {
  static_assert(!std::is_same_v<Kwd1, Kwd2>, "each keyword argument must be unique");
  static_assert(!std::is_same_v<Kwd1, Kwd3>, "each keyword argument must be unique");
  static_assert(!std::is_same_v<Kwd2, Kwd3>, "each keyword argument must be unique");
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
        return plyr->template evaluate(*calc, *cols...);
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
         Cols const *...cols) { return plyr->template apply(*appl, *cols...); },
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

  m_processor.process(m_sources, m_weight, m_nrows);
  m_analyzed = true;
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
      [&val](dataset::player *plyr) { return plyr->template assign<Val>(val); },
      m_processor.get_slots());
  auto lzy = lazy<column::valued<Val>>(*this, act);
  return lzy;
}

template <typename Col>
auto queryosity::dataflow::_cut(lazy<Col> const &col) -> lazy<selection::node> {
  return this->filter(col);
}

template <typename Lzy>
auto queryosity::dataflow::_cut(varied<Lzy> const &col)
    -> varied<lazy<selection::node>> {
  return this->filter(col);
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

template <typename Sel, typename Fn> auto queryosity::dataflow::_select(Fn fn) {
  return todo<selection::applicator<Sel, typename column::equation_t<Fn>>>(
      *this, ensemble::invoke(
                 [fn](dataset::player *plyr) {
                   return plyr->template select<Sel>(nullptr, fn);
                 },
                 m_processor.get_slots()));
}

template <typename Sel, typename Fn>
auto queryosity::dataflow::_select(lazy<selection::node> const &prev, Fn fn) {
  return todo<selection::applicator<Sel, typename column::equation_t<Fn>>>(
      *this, ensemble::invoke(
                 [fn](dataset::player *plyr, selection::node const *prev) {
                   return plyr->template select<Sel>(prev, fn);
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

template <typename Sel, typename Col>
auto queryosity::dataflow::_apply(lazy<Col> const &dec)
    -> lazy<selection::node> {
  auto act = ensemble::invoke(
      [](dataset::player *plyr, Col *col) {
        return plyr->template apply<Sel>(nullptr, *col);
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
        return plyr->template apply<Sel>(prev, *col);
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

namespace queryosity {

/**
 * @ingroup api
 * @brief Complete the instantiation of a lazy action.
 * @details A todo node is an intermediate state between the dataflow graph and
 * a lazy node, when additional methods must be chained in order to instantiate
 * the action.
 * @tparam Helper Helper class to instantiate the lazy action.
 */
template <typename Helper>
class todo : public dataflow::node,
             public ensemble::slotted<Helper>,
             public systematic::resolver<todo<Helper>> {

public:
  todo(dataflow &df, std::vector<std::unique_ptr<Helper>> bkr);
  virtual ~todo() = default;

  todo(todo &&) = default;
  todo &operator=(todo &&) = default;

  virtual std::vector<Helper *> const &get_slots() const final override;

  virtual void set_variation(const std::string &var_name,
                             todo var) final override;

  virtual todo &nominal() final override;
  virtual todo &variation(const std::string &var_name) final override;
  virtual todo const &nominal() const final override;
  virtual todo const &
  variation(const std::string &var_name) const final override;

  virtual bool has_variation(const std::string &var_name) const final override;
  virtual std::set<std::string> get_variation_names() const final override;

  /**
   * @brief Evaluate the column definition with input columns.
   * @param[in] columns Input columns.
   * @param[in][out] Evaluated column.
   */
  template <
      typename... Nodes, typename V = Helper,
      std::enable_if_t<queryosity::column::is_evaluatable_v<V>, bool> = false>
  auto evaluate(Nodes &&...columns) const
      -> decltype(std::declval<todo<V>>()._evaluate(
          std::forward<Nodes>(columns)...)) {
    return this->_evaluate(std::forward<Nodes>(columns)...);
  }

  /**
   * @brief Apply the selection with input columns.
   * @param[in] columns Input columns.
   * @param[in][out] Applied selection.
   */
  template <
      typename... Nodes, typename V = Helper,
      std::enable_if_t<queryosity::selection::is_applicable_v<V>, bool> = false>
  auto apply(Nodes &&...columns) const
      -> decltype(std::declval<todo<V>>()._apply(
          std::forward<Nodes>(columns)...)) {
    return this->template _apply(std::forward<Nodes>(columns)...);
  }

  /**
   * @brief Fill query with input columns per-entry.
   * @param[in] columns Input columns.
   * @returns Updated query plan filled with input columns.
   */
  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::query::is_fillable_v<query::booked_t<V>>, bool> = false>
  auto fill(Nodes &&...columns) const
      -> decltype(std::declval<todo<V>>()._fill(std::declval<Nodes>()...)) {
    return this->_fill(std::forward<Nodes>(columns)...);
  }

  /**
   * @brief Book a query at a selection.
   * @param[in] sel Selection node at which query is counted/filled.
   * @return The query booked at the selection.
   */
  template <typename Node> auto at(Node &&selection) const {
    return this->_book(std::forward<Node>(selection));
  }

  /**
   * @brief Book a query at multiple selections.
   * @tparam Sels... Selections.
   * @param[in] sels... selection nodes.
   * @return `std::tuple` of queries booked at each selection.
   */
  template <typename... Sels> auto at(Sels &&...sels) const {
    static_assert(query::is_bookable_v<Helper>, "not bookable");
    return this->_book(std::forward<Sels>(sels)...);
  }

  /**
   * @brief Shorthand for `evaluate()`.
   * @tparam Args... Input column types.
   * @param[in] columns... Input columns.
   * @return Evaluated column.
   */
  template <typename... Args> auto operator()(Args &&...columns) const {
    if constexpr (column::is_evaluatable_v<Helper>) {
      return this->evaluate(std::forward<Args>(columns)...);
    } else if constexpr (selection::is_applicable_v<Helper>) {
      return this->apply(std::forward<Args>(columns)...);
    } else if constexpr (query::is_bookable_v<Helper>) {
      return this->fill(std::forward<Args>(columns)...);
    }
  }

protected:
  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::column::is_evaluatable_v<V> &&
                                 queryosity::has_no_variation_v<Nodes...>,
                             bool> = false>
  auto _evaluate(Nodes const &...columns) const
      -> lazy<column::evaluated_t<V>> {
    return this->m_df->_evaluate(*this, columns...);
  }

  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::column::is_evaluatable_v<V> &&
                                 queryosity::has_variation_v<Nodes...>,
                             bool> = false>
  auto _evaluate(Nodes const &...columns) const
      -> varied<lazy<column::evaluated_t<V>>> {

    using varied_type =
        varied<lazy<column::evaluated_t<V>>>;

    auto nom = this->m_df->_evaluate(*this, columns.nominal()...);
    auto sys = varied_type(std::move(nom));

    for (auto const &var_name : systematic::get_variation_names(columns...)) {
      auto var = this->m_df->_evaluate(*this, columns.variation(var_name)...);
      sys.set_variation(var_name, std::move(var));
    }

    return sys;
  }

  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::selection::is_applicable_v<V> &&
                                 queryosity::has_no_variation_v<Nodes...>,
                             bool> = false>
  auto _apply(Nodes const &...columns) const -> lazy<selection::node> {
    using selection_type = typename V::selection_type;
    return this->m_df->template _apply<selection_type>(*this, columns...);
  }

  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::selection::is_applicable_v<V> &&
                                 queryosity::has_variation_v<Nodes...>,
                             bool> = false>
  auto _apply(Nodes const &...columns) const -> varied<lazy<selection::node>> {

    using selection_type = typename V::selection_type;
    using varied_type = varied<lazy<selection::node>>;

    auto nom = this->m_df->template _apply<selection_type>(
        *this, columns.nominal()...);
    auto sys = varied_type(nom);

    for (auto const &var_name : systematic::get_variation_names(columns...)) {
      auto var = this->m_df->template _apply<selection_type>(
          *this, columns.variation(var_name)...);
      sys.set_variation(var_name, var);
    }

    return sys;
  }

  template <typename Node, typename V = Helper,
            std::enable_if_t<queryosity::query::is_bookable_v<V> &&
                                 queryosity::is_nominal_v<Node>,
                             bool> = false>
  auto _book(Node const &sel) const -> lazy<query::booked_t<V>> {
    return this->m_df->_book(*this, sel);
  }

  template <typename Node, typename V = Helper,
            std::enable_if_t<queryosity::query::is_bookable_v<V> &&
                                 queryosity::is_varied_v<Node>,
                             bool> = false>
  auto _book(Node const &sel) const -> varied<lazy<query::booked_t<V>>> {
    using varied_type = varied<lazy<query::booked_t<V>>>;
    auto sys = varied_type(this->m_df->_book(*this, sel.nominal()));
    for (auto const &var_name : systematic::get_variation_names(sel)) {
      sys.set_variation(var_name,
                         this->m_df->_book(*this, sel.variation(var_name)));
    }
    return sys;
  }

  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::query::is_bookable_v<V> &&
                                 queryosity::has_no_variation_v<Nodes...>,
                             bool> = false>
  auto _book(Nodes const &...sels) const
      -> std::array<lazy<query::booked_t<V>>, sizeof...(Nodes)> {
    return std::array<lazy<query::booked_t<V>>, sizeof...(Nodes)>{
        this->m_df->_book(*this, sels)...};
  }

  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::query::is_bookable_v<V> &&
                                 has_variation_v<Nodes...>,
                             bool> = false>
  auto _book(Nodes const &...sels) const
      -> std::array<varied<lazy<query::booked_t<V>>>, sizeof...(Nodes)> {
    using varied_type = varied<lazy<query::booked_t<V>>>;
    using array_of_varied_type = std::array<varied_type, sizeof...(Nodes)>;
    auto var_names = systematic::get_variation_names(sels...);
    auto _book_varied =
        [var_names,
         this](systematic::resolver<lazy<selection::node>> const &sel) {
          auto sys = varied_type(this->m_df->_book(*this, sel.nominal()));
          for (auto const &var_name : var_names) {
            sys.set_variation(
                var_name, this->m_df->_book(*this, sel.variation(var_name)));
          }
          return sys;
        };
    return array_of_varied_type{_book_varied(sels)...};
  }

  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::query::is_fillable_v<query::booked_t<V>> &&
                                 queryosity::has_no_variation_v<Nodes...>,
                             bool> = false>
  auto _fill(Nodes const &...columns) const -> todo<V> {
    return todo<V>(*this->m_df,
                   ensemble::invoke(
                       [](V *fillable, typename Nodes::action_type *...cols) {
                         return fillable->add_columns(*cols...);
                       },
                       this->get_slots(), columns.get_slots()...));
  }

  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::query::is_fillable_v<query::booked_t<V>> &&
                                 has_variation_v<Nodes...>,
                             bool> = false>
  auto _fill(Nodes const &...columns) const -> varied<todo<Helper>> {
    using varied_type = varied<todo<V>>;
    auto sys = varied_type(std::move(this->_fill(columns.nominal()...)));
    for (auto const &var_name : systematic::get_variation_names(columns...)) {
      sys.set_variation(
          var_name, std::move(this->_fill(columns.variation(var_name)...)));
    }
    return sys;
  }

protected:
  std::vector<std::unique_ptr<Helper>> m_slots;
  std::vector<Helper *> m_ptrs;
};

} // namespace queryosity

template <typename Helper>
queryosity::todo<Helper>::todo(queryosity::dataflow &df,
                               std::vector<std::unique_ptr<Helper>> bkr)
    : dataflow::node(df), m_slots(std::move(bkr)) {
  m_ptrs.reserve(m_slots.size());
  for (auto const &slot : m_slots) {
    m_ptrs.push_back(slot.get());
  }
}

template <typename Helper>
std::vector<Helper *> const &queryosity::todo<Helper>::get_slots() const {
  return m_ptrs;
}

template <typename Helper>
void queryosity::todo<Helper>::set_variation(const std::string &,
                                             todo<Helper>) {
  // should never be called
  throw std::logic_error("cannot set variation to a nominal-only action");
}

template <typename Helper>
auto queryosity::todo<Helper>::nominal() -> todo<Helper> & {
  // this is nominal
  return *this;
}

template <typename Helper>
auto queryosity::todo<Helper>::variation(const std::string &)
    -> todo<Helper> & {
  // propagation of variations must occur "transparently"
  return *this;
}

template <typename Helper>
auto queryosity::todo<Helper>::nominal() const -> todo const & {
  // this is nominal
  return *this;
}

template <typename Helper>
auto queryosity::todo<Helper>::variation(const std::string &) const
    -> todo const & {
  // propagation of variations must occur "transparently"
  return *this;
}

template <typename Helper>
std::set<std::string> queryosity::todo<Helper>::get_variation_names() const {
  // no variations to list
  return std::set<std::string>();
}

template <typename Helper>
bool queryosity::todo<Helper>::has_variation(const std::string &) const {
  // always false
  return false;
}

namespace queryosity {

class dataflow;

/**
 * @ingroup abc
 * @brief Column with user-defined return value type and evaluation
 * dataset.
 * @tparam Out Output data type.
 * @tparam Ins Input column data type(s).
 */
template <typename Out, typename... Ins>
class column::definition<Out(Ins...)> : public column::calculation<Out> {

public:
  using vartuple_type = std::tuple<variable<Ins>...>;
  using obstuple_type = std::tuple<observable<Ins>...>;

public:
  definition() = default;
  virtual ~definition() = default;

public:
  virtual Out calculate() const final override;

  /**
   * @brief Compute the quantity of interest for the entry
   * @note Columns observables are not computed until `value()` is
   * called.
   * @param[in] args Input column observables.
   */
  virtual Out evaluate(observable<Ins>... args) const = 0;

  template <typename... Args> void set_arguments(const view<Args> &...args);

protected:
  vartuple_type m_arguments;
};

/**
 * @ingroup api
 * @brief Argument to define a custom column in the dataflow.
 * @tparam Def Concrete implementation of
 * `queryosity::column::definition<Out(Ins...)>`
 */
template <typename Def> class column::definition {

public:
  /**
   * @brief Argument constructor.
   * @param[in] args Constructor arguments of @p Def.
   */
  template <typename... Args> definition(Args const &...args);

  auto _define(dataflow &df) const;

protected:
  std::function<todo<evaluator<Def>>(dataflow &)> m_define;
};

} // namespace queryosity

template <typename Out, typename... Ins>
template <typename... Args>
void queryosity::column::definition<Out(Ins...)>::set_arguments(
    view<Args> const &...args) {
  static_assert(sizeof...(Ins) == sizeof...(Args));
  m_arguments = std::make_tuple(std::invoke(
      [](const view<Args> &args) -> variable<Ins> {
        return variable<Ins>(args);
      },
      args)...);
}

template <typename Out, typename... Ins>
Out queryosity::column::definition<Out(Ins...)>::calculate() const {
  return std::apply(
      [this](const variable<Ins> &...args) { return this->evaluate(args...); },
      m_arguments);
}

template <typename Def>
template <typename... Args>
queryosity::column::definition<Def>::definition(Args const &...args) {
  m_define = [args...](dataflow &df) { return df._define<Def>(args...); };
}

template <typename Def>
auto queryosity::column::definition<Def>::_define(dataflow &df) const {
  return this->m_define(df);
}

#include <cmath>

namespace queryosity {

namespace selection {

/**
 * @brief Yield (sum of weights and squared error) at a selection.
 */
struct count_t {

  unsigned long long entries;
  double value;
  double error;
};

class counter : public query::aggregation<count_t> {

public:
  counter() = default;
  virtual ~counter() = default;

  virtual void count(double w) final override;
  virtual count_t result() const final override;
  virtual void finalize(unsigned int) final override;
  virtual count_t merge(std::vector<count_t> const &results) const final override;

protected:
  count_t m_cnt;
};

/**
 * @brief Argumnet for column yield.
 * @tparam Sel (Varied) lazy column node.
 * @todo C++20: Use concept to require lazy<column<Val>(::varied)>.
 */
template <typename... Sels> struct yield {

public:
  yield(Sels const &...sels);
  ~yield() = default;

  auto make(dataflow &df) const;

protected:
  std::tuple<Sels...> m_selections;
};

} // namespace selection

} // namespace queryosity

inline void queryosity::selection::counter::count(double w) {
  m_cnt.entries++;
  m_cnt.value += w;
  m_cnt.error += w * w;
}

inline void queryosity::selection::counter::finalize(unsigned int) {
  m_cnt.error = std::sqrt(m_cnt.error);
}

inline queryosity::selection::count_t queryosity::selection::counter::result() const {
  return m_cnt;
}

inline queryosity::selection::count_t
queryosity::selection::counter::merge(std::vector<count_t> const& cnts) const {
  count_t sum;
  for (auto const &cnt : cnts) {
    sum.entries += cnt.entries;
    sum.value += cnt.value;
    sum.error += cnt.error * cnt.error;
  }
  sum.error = std::sqrt(sum.error);
  return sum;
}

template <typename... Sels>
queryosity::selection::yield<Sels...>::yield(Sels const &...sels)
    : m_selections(sels...) {}

template <typename... Sels>
auto queryosity::selection::yield<Sels...>::make(dataflow &df) const {
  return std::apply(
      [&df](Sels const &...sels) {
        return df.get(query::output<counter>()).at(sels...);
      },
      m_selections);
}

namespace queryosity {

/**
 * @ingroup api
 * @brief Varied version of a todo item.
 * @details A todo varied item is functionally equivalent to a @p todo
 * node with each method being propagated to independent todo nodes
 * corresponding to nominal and systematic variations.
 */
template <typename Helper>
class varied<todo<Helper>> : public dataflow::node,
                             systematic::resolver<todo<Helper>> {

public:
  template <typename> friend class lazy;
  template <typename> friend class varied;

public:
  varied(todo<Helper> &&nom);
  ~varied() = default;

  varied(varied &&) = default;
  varied &operator=(varied &&) = default;

  virtual void set_variation(const std::string &var_name,
                             todo<Helper> var) final override;

  virtual todo<Helper> &nominal() final override;
  virtual todo<Helper> &variation(const std::string &var_name) final override;
  virtual todo<Helper> const &nominal() const final override;
  virtual todo<Helper> const &
  variation(const std::string &var_name) const final override;

  virtual bool has_variation(const std::string &var_name) const final override;
  virtual std::set<std::string> get_variation_names() const final override;

public:
  template <
      typename... Cols, typename V = Helper,
      std::enable_if_t<queryosity::column::is_evaluatable_v<V>, bool> = false>
  auto evaluate(Cols &&...cols) -> varied<
      decltype(this->nominal().evaluate(std::forward<Cols>(cols.nominal)...))>;

  template <
      typename... Cols, typename V = Helper,
      std::enable_if_t<queryosity::selection::is_applicable_v<V>, bool> = false>
  auto apply(Cols &&...cols) -> varied<lazy<selection::applied_t<V>>>;

  /**
   * @brief Fill the query with input columns.
   * @param[in] columns... Input columns to fill the query with.
   * @return A new todo query node with input columns filled.
   */
  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::query::is_fillable_v<queryosity::query::booked_t<V>>, bool> = false>
  auto fill(Nodes const &...columns) -> varied;

  /**
   * @brief Book the query at a selection.
   * @param[in] selection Lazy selection to book query at.
   * @return Lazy query booked at selection.
   */
  template <typename Node, typename V = Helper,
            std::enable_if_t<queryosity::query::is_bookable_v<V>, bool> = false>
  auto at(Node const &selection) -> varied<lazy<query::booked_t<V>>>;

  /**
   * @brief Book the query at multiple selections.
   * @param[in] selection Lazy selection to book queries at.
   * @return Delayed query containing booked lazy queries.
   */
  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::query::is_bookable_v<V>, bool> = false>
  auto at(Nodes const &...selections)
      -> std::array<varied<lazy<query::booked_t<V>>>, sizeof...(Nodes)>;

  template <typename... Cols>
  auto operator()(Cols &&...cols)
      -> varied<decltype(std::declval<todo<Helper>>().operator()(
          std::forward<Cols>(cols).nominal()...))>;

protected:
  todo<Helper> m_nominal;
  std::unordered_map<std::string, todo<Helper>> m_variation_map;
  std::set<std::string> m_variation_names;
};

} // namespace queryosity

template <typename Helper>
queryosity::varied<queryosity::todo<Helper>>::varied(
    queryosity::todo<Helper> &&nom)
    : dataflow::node(*nom.m_df), m_nominal(std::move(nom)) {}

template <typename Helper>
void queryosity::varied<queryosity::todo<Helper>>::set_variation(
    const std::string &var_name, queryosity::todo<Helper> var) {
  m_variation_map.insert(std::move(std::make_pair(var_name, std::move(var))));
  m_variation_names.insert(var_name);
}

template <typename Helper>
auto queryosity::varied<queryosity::todo<Helper>>::nominal() -> todo<Helper> & {
  return m_nominal;
}

template <typename Helper>
auto queryosity::varied<queryosity::todo<Helper>>::nominal() const
    -> todo<Helper> const & {
  return m_nominal;
}

template <typename Helper>
auto queryosity::varied<queryosity::todo<Helper>>::variation(
    const std::string &var_name) -> todo<Helper> & {
  return (this->has_variation(var_name) ? m_variation_map.at(var_name)
                                        : m_nominal);
}

template <typename Helper>
auto queryosity::varied<queryosity::todo<Helper>>::variation(
    const std::string &var_name) const -> todo<Helper> const & {
  return (this->has_variation(var_name) ? m_variation_map.at(var_name)
                                        : m_nominal);
}

template <typename Helper>
bool queryosity::varied<queryosity::todo<Helper>>::has_variation(
    const std::string &var_name) const {
  return m_variation_map.find(var_name) != m_variation_map.end();
}

template <typename Helper>
std::set<std::string>
queryosity::varied<queryosity::todo<Helper>>::get_variation_names() const {
  return m_variation_names;
}

template <typename Helper>
template <typename... Cols, typename V,
          std::enable_if_t<queryosity::column::is_evaluatable_v<V>, bool>>
auto queryosity::varied<queryosity::todo<Helper>>::evaluate(Cols &&...cols)
    -> varied<decltype(this->nominal().evaluate(
        std::forward<Cols>(cols.nominal)...))> {
  using varied_type = varied<decltype(this->nominal().evaluate(
      std::forward<Cols>(cols.nominal)...))>;
  auto syst = varied_type(
      this->nominal().evaluate(std::forward<Cols>(cols).nominal()...));
  for (auto const &var_name :
       systematic::get_variation_names(*this, std::forward<Cols>(cols)...)) {
    syst.set_variation(var_name,
                       variation(var_name).evaluate(
                           std::forward<Cols>(cols).variation(var_name)...));
  }
  return syst;
}

template <typename Helper>
template <typename... Cols, typename V,
          std::enable_if_t<queryosity::selection::is_applicable_v<V>, bool>>
auto queryosity::varied<queryosity::todo<Helper>>::apply(Cols &&...cols)
    -> varied<queryosity::lazy<selection::applied_t<V>>> {
  using varied_type = varied<queryosity::lazy<selection::applied_t<V>>>;
  auto syst =
      varied_type(this->nominal().apply(std::forward<Cols>(cols).nominal()...));
  for (auto const &var_name :
       systematic::get_variation_names(*this, std::forward<Cols>(cols)...)) {
    syst.set_variation(var_name,
                       variation(var_name).apply(
                           std::forward<Cols>(cols).variation(var_name)...));
  }
  return syst;
}

template <typename Helper>
template <typename... Nodes, typename V,
          std::enable_if_t<queryosity::query::is_fillable_v<queryosity::query::booked_t<V>>, bool>>
auto queryosity::varied<queryosity::todo<Helper>>::fill(Nodes const &...columns)
    -> varied {
  auto syst = varied(std::move(this->nominal().fill(columns.nominal()...)));
  for (auto const &var_name :
       systematic::get_variation_names(*this, columns...)) {
    syst.set_variation(var_name, std::move(variation(var_name).fill(
                                     columns.variation(var_name)...)));
  }
  return syst;
}

template <typename Helper>
template <typename Node, typename V,
          std::enable_if_t<queryosity::query::is_bookable_v<V>, bool>>
auto queryosity::varied<queryosity::todo<Helper>>::at(Node const &selection)
    -> varied<lazy<query::booked_t<V>>> {
  using varied_type = varied<lazy<query::booked_t<V>>>;
  auto syst = varied_type(this->nominal().at(selection.nominal()));
  for (auto const &var_name :
       systematic::get_variation_names(*this, selection)) {
    syst.set_variation(
        var_name, this->variation(var_name).at(selection.variation(var_name)));
  }
  return syst;
}

template <typename Helper>
template <typename... Nodes, typename V,
          std::enable_if_t<queryosity::query::is_bookable_v<V>, bool>>
auto queryosity::varied<queryosity::todo<Helper>>::at(
    Nodes const &...selections)
    -> std::array<varied<lazy<query::booked_t<V>>>, sizeof...(Nodes)> {
  // variations
  using varied_type = varied<lazy<query::booked_t<V>>>;
  using array_of_varied_type =
      std::array<varied<lazy<query::booked_t<V>>>, sizeof...(Nodes)>;
  auto var_names = systematic::get_variation_names(*this, selections...);
  auto _book_varied =
      [var_names,
       this](systematic::resolver<lazy<selection::node>> const &sel) {
        auto syst = varied_type(this->nominal().at(sel.nominal()));
        for (auto const &var_name : var_names) {
          syst.set_variation(
              var_name, this->variation(var_name).at(sel.variation(var_name)));
        }
        return syst;
      };
  return array_of_varied_type{_book_varied(selections)...};
}

template <typename Helper>
template <typename... Cols>
auto queryosity::varied<queryosity::todo<Helper>>::operator()(Cols &&...cols)
    -> varied<decltype(std::declval<todo<Helper>>().operator()(
        std::forward<Cols>(cols).nominal()...))> {

  using varied_type = varied<decltype(std::declval<todo<Helper>>().operator()(
      std::forward<Cols>(cols).nominal()...))>;

  auto syst = varied_type(
      this->nominal().operator()(std::forward<Cols>(cols).nominal()...));
  for (auto const &var_name :
       systematic::get_variation_names(*this, std::forward<Cols>(cols)...)) {
    syst.set_variation(var_name,
                       variation(var_name).operator()(
                           std::forward<Cols>(cols).variation(var_name)...));
  }
  return syst;
}

namespace qty = queryosity;