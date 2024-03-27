#pragma once

/**
 * @defgroup ext Extensions
 * @defgroup api API
 * @defgroup abc ABCs
 */

#include <algorithm>
#include <cassert>
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
void call(Fn const &fn, std::vector<Args> const &...args);

template <typename Fn, typename... Args>
auto invoke(Fn const &fn, std::vector<Args> const &...args)
    -> std::vector<typename std::invoke_result_t<Fn, Args...>>;

} // namespace ensemble

namespace multithread {

class core {

public:
  core(int suggestion);

  core(const core &) = default;
  core &operator=(const core &) = default;

  ~core() = default;

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
  return first.size();
}

template <typename Fn, typename... Args>
inline void queryosity::ensemble::call(Fn const &fn,
                                       std::vector<Args> const &...args) {
  const auto nslots = check(args...);
  for (size_t i = 0; i < nslots; ++i) {
    fn(args.at(i)...);
  }
}

template <typename Fn, typename... Args>
inline auto queryosity::ensemble::invoke(Fn const &fn,
                                         std::vector<Args> const &...args)
    -> std::vector<typename std::invoke_result_t<Fn, Args...>> {
  auto nslots = check(args...);
  typename std::vector<typename std::invoke_result_t<Fn, Args...>> invoked;
  invoked.reserve(nslots);
  for (size_t i = 0; i < nslots; ++i) {
    invoked.push_back(std::move((fn(args.at(i)...))));
  }
  return invoked;
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

template <typename... Args> class variation;

template <typename Lzy> class nominal;

class mode {
public:
  mode() : m_variation_name("") {}
  ~mode() = default;

  void set_variation_name(const std::string &var_name) {
    m_variation_name = var_name;
  }

  bool is_nominal() const { return m_variation_name.empty(); }
  std::string variation_name() const { return m_variation_name; }

protected:
  std::string m_variation_name;
};

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

class action : public systematic::mode {

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

class node : public action {};

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
  virtual ~variable() = default;

  variable(variable &&) = default;
  variable &operator=(variable &&) = default;

  T const &value() const;
  T const *field() const;

protected:
  std::unique_ptr<const view<T>> m_view;
};

// easy to move around
template <typename T> class observable {

public:
  observable(variable<T> const &obs);
  virtual ~observable() = default;

  T const &value() const;
  T const *field() const;

  T const &operator*() const;
  T const *operator->() const;

protected:
  const variable<T> *m_var;
};

template <typename To, typename From>
std::unique_ptr<view<To>> view_as(view<From> const &from);

class computation;

template <typename T> class reader;

template <typename T> class fixed;

template <typename T> class calculation;

template <typename T> class definition;

template <typename T, typename U> class conversion;

template <typename T> class equation;

template <typename T> class representation;

template <typename T> class constant;

template <typename T> class expression;

template <typename T> class evaluator;

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
check_representation(typename column::representation<T> const &);
constexpr std::false_type check_representation(...);

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
constexpr bool is_representation_v = decltype(check_representation(
    std::declval<std::decay_t<T> const &>()))::value;

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
    : m_var(&var) {}

template <typename T>
T const &queryosity::column::observable<T>::operator*() const {
  return m_var->value();
}

template <typename T>
T const *queryosity::column::observable<T>::operator->() const {
  return m_var->field();
}

template <typename T>
T const &queryosity::column::observable<T>::value() const {
  return m_var->value();
}

template <typename T>
T const *queryosity::column::observable<T>::field() const {
  return m_var->field();
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

  virtual void parallelize(unsigned int concurrency) = 0;

  virtual double normalize();
  virtual void initialize();

  /**
   * @brief Determine dataset partition for parallel processing.
   * @return Dataset entry partition
   *
   * @details
   *
   * - The dataflow *must* load at least one dataset with a valid partition.
   * - A valid partition *must* begin at 0 and be in sorted contiguous order,
   * e.g. `{{0,100},{100,200}}`.
   * - If a dataset returns an empty partition, it relinquishes the control to
   * another dataset in the dataflow.
   * @attention The empty-partition dataset *must* be able to fulfill
   * @code{.cpp}execute(entry)@endcode calls for any `entry` as
   * requested by the other datasets in the dataflow.
   *
   * Valid partitions reported by loaded datasets undergo the following changes:
   * 1. A common alignment partition is calculated across all loaded datasets.
   * @attention All non-empty partitions in the dataflow *must* have the same
   * total number of entries in order them to be alignable.
   * 2. Entries past the maximum to be processed are truncated.
   * 3. Neighbouring ranges are merged to match thread concurrency.
   *
   */
  virtual std::vector<std::pair<unsigned long long, unsigned long long>>
  partition() = 0;

  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) override;
  virtual void execute(unsigned int slot, unsigned long long entry) override;
  virtual void finalize(unsigned int slot) override;
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

inline double queryosity::dataset::source::normalize() { return 1.0; }

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

template <typename T> class column::evaluator {

public:
  using evaluated_type = T;

public:
  template <typename... Args> evaluator(Args const &...args);
  ~evaluator() = default;

  template <typename... Vals>
  std::unique_ptr<T> _evaluate(view<Vals> const &...cols) const;

protected:
  std::function<std::unique_ptr<T>()> m_make_unique;
};

} // namespace queryosity

template <typename T>
template <typename... Args>
queryosity::column::evaluator<T>::evaluator(Args const &...args)
    : m_make_unique(std::bind(
          [](Args const &...args) { return std::make_unique<T>(args...); },
          args...)) {}

template <typename T>
template <typename... Vals>
std::unique_ptr<T> queryosity::column::evaluator<T>::_evaluate(
    view<Vals> const &...columns) const {
  auto defn = m_make_unique();

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

using partition_t = std::vector<part_t>;

namespace partition {

partition_t align(std::vector<partition_t> const &partitions);

partition_t truncate(partition_t const &parts, long long nentries_max);

partition_t merge(partition_t const &parts, unsigned int nslots_max);

} // namespace partition

} // namespace dataset

} // namespace queryosity

inline queryosity::dataset::partition_t queryosity::dataset::partition::align(
    std::vector<partition_t> const &partitions) {
  std::map<unsigned long long, unsigned int> edge_counts;
  const unsigned int num_vectors = partitions.size();

  // Count appearances of each edge
  for (const auto &vec : partitions) {
    std::map<unsigned long long, bool>
        seen_edges; // Ensure each edge is only counted once per vector
    for (const auto &p : vec) {
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
  std::vector<unsigned long long> aligned_edges;
  for (const auto &pair : edge_counts) {
    if (pair.second == num_vectors) {
      aligned_edges.push_back(pair.first);
    }
  }

  // Create aligned vector of pairs
  std::vector<std::pair<unsigned long long, unsigned long long>> aligned_ranges;
  for (size_t i = 0; i < aligned_edges.size() - 1; ++i) {
    aligned_ranges.emplace_back(aligned_edges[i], aligned_edges[i + 1]);
  }

  return aligned_ranges;
}

inline queryosity::dataset::partition_t queryosity::dataset::partition::merge(
    queryosity::dataset::partition_t const &parts, unsigned int nslots_max) {

  // no merging needed
  if (nslots_max >= static_cast<unsigned int>(parts.size()))
    return parts;

  assert(!parts.empty() && nslots_max > 0);

  partition_t parts_merged;

  const unsigned int total_size = parts.back().second - parts.front().first;
  const unsigned int size_per_slot = total_size / nslots_max;
  const unsigned int extra_size = total_size % nslots_max;

  unsigned int current_start = parts[0].first;
  unsigned int current_end = current_start;
  unsigned int accumulated_size = 0;
  unsigned int nslots_created = 0;

  for (const auto &part : parts) {
    unsigned int part_size = part.second - part.first;
    // check if another part can be added
    if (accumulated_size + part_size >
            size_per_slot + (nslots_created < extra_size ? 1 : 0) &&
        nslots_created < nslots_max - 1) {
      // add the current range if adding next part will exceed the average size
      parts_merged.emplace_back(current_start, current_end);
      current_start = current_end;
      accumulated_size = 0;
      ++nslots_created;
    }

    // add part size to the current slot
    accumulated_size += part_size;
    current_end += part_size;

    // handle the last slot differently to include all remaining parts
    if (nslots_created == nslots_max - 1) {
      parts_merged.emplace_back(current_start, parts.back().second);
      break; // All parts have been processed
    }
  }

  // ensure we have exactly nslots_max slots
  if (static_cast<unsigned int>(parts_merged.size()) < nslots_max) {
    parts_merged.emplace_back(current_start, parts.back().second);
  }

  return parts_merged;
}

inline queryosity::dataset::partition_t
queryosity::dataset::partition::truncate(
    queryosity::dataset::partition_t const &parts, long long nentries_max) {
  if (nentries_max < 0)
    return parts;

  partition_t parts_truncated;

  for (const auto &part : parts) {
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
  auto evaluate(evaluator<Def> &calc, Cols const &...cols) -> Def *;

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
  template <typename Fn> equation(Fn fn);
  virtual ~equation() = default;

public:
  virtual Out evaluate(observable<Ins>... args) const override;

protected:
  vartuple_type m_arguments;
  function_type m_evaluate;
};

template <typename Fn>
auto make_equation(Fn fn) -> std::unique_ptr<column::equation_t<Fn>>;

} // namespace column

} // namespace queryosity

template <typename Out, typename... Ins>
template <typename Fn>
queryosity::column::equation<Out(Ins...)>::equation(Fn fn) : m_evaluate(fn) {}

template <typename Out, typename... Ins>
Out queryosity::column::equation<Out(Ins...)>::evaluate(
    observable<Ins>... args) const {
  return this->m_evaluate(args.value()...);
}

template <typename Fn>
auto queryosity::column::make_equation(Fn fn)
    -> std::unique_ptr<queryosity::column::equation_t<Fn>> {
  return std::make_unique<queryosity::column::equation_t<Fn>>(fn);
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

  const Val &value() const override;

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
auto queryosity::column::computation::evaluate(evaluator<Def> &calc,
                                               Cols const &...cols) -> Def * {
  auto defn = calc._evaluate(cols...);
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

template <typename T> class apply;

class node : public column::calculation<double> {

public:
  node(const selection::node *presel, column::variable<double> dec);
  virtual ~node() = default;

public:
  bool is_initial() const noexcept;
  const selection::node *get_previous() const noexcept;

  template <typename T> void set_decision(column::valued<T> const &dec);

  virtual bool passed_cut() const = 0;
  virtual double get_weight() const = 0;

protected:
  const selection::node *const m_preselection;
  column::variable<double> m_decision;
};

template <typename T> struct is_applicable : std::false_type {};
template <typename T>
struct is_applicable<selection::apply<T>> : std::true_type {};
template <typename T>
static constexpr bool is_applicable_v = is_applicable<T>::value;

template <typename F>
using apply_t = typename selection::apply<
    queryosity::column::equation_t<F>>;

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

template <typename T> class definition;

template <typename T> class book;

template <typename T> class plan;

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
  virtual void execute(unsigned int slot, unsigned long long entry) override;
  virtual void finalize(unsigned int slot) override;

  virtual void count(double w) = 0;

protected:
  bool m_raw;
  double m_scale;
  const selection::node *m_selection;
};

template <typename T>
constexpr std::true_type check_implemented(const query::aggregation<T> &);
constexpr std::false_type check_implemented(...);

template <typename Out, typename... Vals>
constexpr std::true_type
check_fillable(const typename query::definition<Out(Vals...)> &);
constexpr std::false_type check_fillable(...);

template <typename T> struct is_book : std::false_type {};
template <typename T> struct is_book<query::book<T>> : std::true_type {};

template <typename T>
constexpr bool is_aggregation_v =
    decltype(check_implemented(std::declval<T>()))::value;

template <typename T>
constexpr bool is_fillable_v =
    decltype(check_fillable(std::declval<T>()))::value;

template <typename T> constexpr bool is_bookable_v = is_book<T>::value;

template <typename Bkr> using booked_t = typename Bkr::query_type;

} // namespace query

} // namespace queryosity

inline queryosity::query::node::node()
    : m_raw(false), m_scale(1.0), m_selection(nullptr) {}

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

inline void queryosity::query::node::use_weight(bool use) { m_raw = !use; }

inline void queryosity::query::node::initialize(unsigned int,
                                                unsigned long long,
                                                unsigned long long) {
  if (!m_selection)
    throw std::runtime_error("no booked selection");
}

inline void queryosity::query::node::execute(unsigned int, unsigned long long) {
  if (m_selection->passed_cut()) {
    this->count(m_raw ? 1.0 : m_scale * m_selection->get_weight());
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

namespace queryosity {

class selection::cutflow {

public:
  cutflow() = default;
  ~cutflow() = default;

public:
  template <typename Sel, typename Val>
  auto select(selection::node const *prev, column::valued<Val> const &dec)
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
  virtual double calculate() const override;
  virtual bool passed_cut() const override;
  virtual double get_weight() const override;
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
  virtual double calculate() const override;
  virtual bool passed_cut() const override;
  virtual double get_weight() const override;
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
auto queryosity::selection::cutflow::select(selection::node const *prev,
                                            column::valued<Val> const &dec)
    -> selection::node * {
  auto sel = std::make_unique<Sel>(prev, column::variable<double>(dec));
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

template <typename T> class query::book {

public:
  using query_type = T;

public:
  template <typename... Args> book(Args... args);
  ~book() = default;

  // copyable
  book(const book &) = default;
  book &operator=(const book &) = default;

  template <typename... Vals>
  auto book_fill(column::valued<Vals> const &...cols) const
      -> std::unique_ptr<book<T>>;

  auto set_selection(const selection::node &sel) const -> std::unique_ptr<T>;

protected:
  std::unique_ptr<T> make_query();
  template <typename... Vals>
  void fill_query(column::valued<Vals> const &...cols);

protected:
  std::function<std::unique_ptr<T>()> m_make_unique_query;
  std::vector<std::function<void(T &)>> m_fill_columns;
};

} // namespace queryosity

template <typename T>
template <typename... Args>
queryosity::query::book<T>::book(Args... args)
    : m_make_unique_query(std::bind(
          [](Args... args) { return std::make_unique<T>(args...); }, args...)) {
}

template <typename T>
template <typename... Vals>
auto queryosity::query::book<T>::book_fill(
    column::valued<Vals> const &...columns) const -> std::unique_ptr<book<T>> {
  // use a fresh one with its current fills
  auto filled = std::make_unique<book<T>>(*this);
  // add fills
  filled->fill_query(columns...);
  // return new book
  return filled;
}

template <typename T>
template <typename... Vals>
void queryosity::query::book<T>::fill_query(
    column::valued<Vals> const &...columns) {
  // use a snapshot of its current calls
  m_fill_columns.push_back(std::bind(
      [](T &cnt, column::valued<Vals> const &...cols) {
        cnt.enter_columns(cols...);
      },
      std::placeholders::_1, std::ref(columns)...));
}

template <typename T>
auto queryosity::query::book<T>::set_selection(const selection::node &sel) const
    -> std::unique_ptr<T> {
  // call constructor
  auto cnt = m_make_unique_query();
  // fill columns (if set)
  for (const auto &fill_query : m_fill_columns) {
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
  std::unique_ptr<query::book<Qry>> make(Args &&...args);

  template <typename Qry>
  auto book(query::book<Qry> const &bkr, const selection::node &sel) -> Qry *;

  void scale_current_queries(double scale);
  void clear_current_queries();

protected:
  template <typename Qry> auto add_query(std::unique_ptr<Qry> qry) -> Qry *;

protected:
  std::vector<std::unique_ptr<query::node>> m_query_history;
  std::vector<query::node *> m_queries;
};

} // namespace queryosity

inline void queryosity::query::experiment::clear_current_queries() {
  m_queries.clear();
}

inline void queryosity::query::experiment::scale_current_queries(double scale) {
  for (auto const &qry : m_queries) {
    qry->apply_scale(scale);
  }
}

template <typename Qry, typename... Args>
std::unique_ptr<queryosity::query::book<Qry>>
queryosity::query::experiment::make(Args &&...args) {
  auto bkr = std::make_unique<query::book<Qry>>(std::forward<Args>(args)...);
  return bkr;
}

template <typename Qry>
auto queryosity::query::experiment::book(query::book<Qry> const &bkr,
                                         const selection::node &sel) -> Qry * {
  auto qry = bkr.set_selection(sel);
  return this->add_query(std::move(qry));
}

template <typename Qry>
auto queryosity::query::experiment::add_query(std::unique_ptr<Qry> qry)
    -> Qry * {
  auto out = qry.get();
  m_queries.push_back(out);
  m_query_history.push_back(std::move(qry));
  return out;
}

namespace queryosity {

namespace dataset {

class player : public queryosity::column::computation,
               public query::experiment {

public:
  player() = default;
  virtual ~player() = default;

public:
  void play(std::vector<std::unique_ptr<source>> const &sources, double scale,
            unsigned int slot, unsigned long long begin,
            unsigned long long end);
};

} // namespace dataset

} // namespace queryosity

inline void queryosity::dataset::player::play(
    std::vector<std::unique_ptr<source>> const &sources, double scale,
    unsigned int slot, unsigned long long begin, unsigned long long end) {

  for (auto const &qry : m_queries) {
    qry->apply_scale(scale);
  }

  // initialize
  for (auto const &ds : sources) {
    ds->initialize(slot, begin, end);
  }
  for (auto const &col : m_columns) {
    col->initialize(slot, begin, end);
  }
  for (auto const &sel : m_selections) {
    sel->initialize(slot, begin, end);
  }
  for (auto const &qry : m_queries) {
    qry->initialize(slot, begin, end);
  }

  // execute
  for (auto entry = begin; entry < end; ++entry) {
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

  // clear out queries (should not be re-played)
  m_queries.clear();
}

namespace queryosity {

namespace dataset {

class processor : public multithread::core, public ensemble::slotted<player> {
public:
  processor(int suggestion);
  ~processor() = default;

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
    : multithread::core::core(suggestion) {
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
  std::vector<std::vector<std::pair<unsigned long long, unsigned long long>>>
      partitions;
  for (auto const &ds : sources) {
    auto partition = ds->partition();
    if (partition.size())
      partitions.push_back(partition);
  }
  if (!partitions.size()) {
    throw std::logic_error("no valid dataset partition implemented");
  }
  // find common denominator partition
  auto partition = dataset::partition::align(partitions);
  // truncate entries to row limit
  partition = dataset::partition::truncate(partition, nrows);
  // merge partition to concurrency limit
  partition = dataset::partition::merge(partition, nslots);
  // match processor & partition parallelism
  this->downsize(partition.size());

  // 3. run event loop
  this->run(
      [&sources,
       scale](dataset::player *plyr, unsigned int slot,
              std::pair<unsigned long long, unsigned long long> part) {
        plyr->play(sources, scale, slot, part.first, part.second);
      },
      m_player_ptrs, m_range_slots, partition);

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
   * @tparam Fn Callable type.
   * @tparam Cols Input column types.
   * @param[in] expr C++ function, functor, lambda, or any other callable.
   * @param[in] cols Input columns.
   * @return Lazy column.
   */
  template <typename Fn, typename... Cols>
  auto define(column::expression<Fn> const &expr, lazy<Cols> const &...cols)
      -> lazy<column::equation_t<Fn>>;

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
  auto filter(queryosity::column::expression<Fn> const &expr,
              Cols const &...cols);

  /**
   * @brief Initiate a cutflow.
   * @tparam Fn C++ Callable object.
   * @tparam Cols Column types.
   * @param[in] Input (varied) columns used to evaluate weight decision.
   * @return Lazy (varied) selection.
   */
  template <typename Fn, typename... Cols>
  auto weight(queryosity::column::expression<Fn> const &expr,
              Cols const &...cols);

  /**
   * @brief Plan a query.
   * @tparam Qry Concrete queryosity::query::definition implementation.
   * @param[in] plan Query plan (constructor arguments).
   * @return queryosity::todo query booker.
   */
  template <typename Qry>
  auto make(query::plan<Qry> const &plan) -> todo<query::book<Qry>>;

  template <typename Val, typename... Vars>
  auto vary(column::constant<Val> const &nom, Vars const &...vars);

  template <typename Fn, typename... Vars>
  auto vary(column::expression<Fn> const &expr, Vars const &...vars);

  template <typename Defn, typename... Vars>
  auto vary(column::definition<Defn> const &defn, Vars const &...vars);

  /* public, but not really... */

  template <typename Val>
  auto _assign(Val const &val) -> lazy<column::fixed<Val>>;

  template <typename To, typename Col>
  auto _convert(lazy<Col> const &col)
      -> lazy<column::conversion<To, column::value_t<Col>>>;

  template <typename Def, typename... Args> auto _define(Args &&...args);
  template <typename Def> auto _define(column::definition<Def> const &defn) -> todo<column::evaluator<Def>>;

  template <typename Fn> auto _equate(Fn fn);
  template <typename Fn> auto _equate(column::expression<Fn> const &expr) -> todo<column::evaluator<column::equation_t<Fn>>> ;

  template <typename Sel, typename Col>
  auto _select(lazy<Col> const &col) -> lazy<selection::node>;

  template <typename Sel, typename Col>
  auto _select(lazy<selection::node> const &prev, lazy<Col> const &col)
      -> lazy<selection::node>;

  template <typename Qry, typename... Args>
  auto _make(Args &&...args) -> todo<query::book<Qry>>;

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

  template <typename Qry>
  auto _book(todo<query::book<Qry>> const &bkr,
             lazy<selection::node> const &sel) -> lazy<Qry>;
  template <typename Qry, typename... Sels>
  auto _book(todo<query::book<Qry>> const &bkr, lazy<Sels> const &...sels)
      -> std::array<lazy<Qry>, sizeof...(Sels)>;

  template <typename Syst, typename Val>
  void _vary(Syst &syst, const std::string &name,
             column::constant<Val> const &cnst);

  template <typename Syst, typename Fn>
  void _vary(Syst &syst, const std::string &name,
             column::expression<Fn> const &expr);

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

#include <memory>

namespace queryosity {

namespace dataset {

/**
 * @ingroup api
 * @brief Argument for queryosity::dataflow::load().
 * @tparam DS queryosity::dataset::reader implementation.
 */
template <typename DS> struct input {
  /**
   * @brief Constructor.
   * @tparam Args `DS` constructor argument types.
   * @param[in] args Constructor arguments for `DS`.
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
  auto read(dataset::column<Val> const &col) -> lazy<read_column_t<DS, Val>>;

  template <typename... Vals> auto read(dataset::columns<Vals...> const &cols);

  template <typename Val>
  auto vary(dataset::column<Val> const &nom,
            systematic::variation<std::string> const &var);

  template <typename Val, typename... Vars>
  auto vary(dataset::column<Val> const &nom,
            systematic::variation<Vars> const &...vars);

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
 * @brief Argument for queryosity::dataflow::read().
 * @tparam Val Column data type.
 */
template <typename Val> class dataset::column {

public:
  /**
   * @brief Constructor.
   * @param[in] column_name Name of column.
   */
  column(const std::string &column_name);
  ~column() = default;

  template <typename DS> auto _read(dataset::loaded<DS> &ds) const;

protected:
  std::string m_name;
};

template <typename... Vals> class dataset::columns {

public:
  template <typename... Names> columns(Names const &...names);
  ~columns() = default;

  template <std::size_t... Is> auto _read(std::index_sequence<Is...>) const {
    // call _read for each type and name, unpacking the indices
    return std::make_tuple(this->_read<Vals>(this->m_names[Is])...);
  }

  template <typename DS> auto _read(dataset::loaded<DS> &ds) const;

protected:
  template <std::size_t... Is>
  auto _construct(const std::array<std::string, sizeof...(Vals)> &names,
                  std::index_sequence<Is...>) {
    return std::make_tuple(column<Vals>(names[Is])...);
  }

  template <typename DS, std::size_t... Is>
  auto _read(dataset::loaded<DS> &ds, std::index_sequence<Is...>) const;

protected:
  std::tuple<column<Vals>...> m_columns;
};

} // namespace queryosity

template <typename Val>
queryosity::dataset::column<Val>::column(const std::string &name)
    : m_name(name) {}

template <typename Val>
template <typename DS>
auto queryosity::dataset::column<Val>::_read(
    queryosity::dataset::loaded<DS> &ds) const {
  return ds.template _read<Val>(this->m_name);
}

template <typename... Vals>
template <typename... Names>
queryosity::dataset::columns<Vals...>::columns(Names const &...names)
    : m_columns(_construct(std::array<std::string, sizeof...(Vals)>{names...},
                           std::make_index_sequence<sizeof...(Vals)>{})) {
  static_assert(sizeof...(Vals) == sizeof...(Names));
}

template <typename... Vals>
template <typename DS>
auto queryosity::dataset::columns<Vals...>::_read(
    queryosity::dataset::loaded<DS> &ds) const {
  return this->_read(ds, std::make_index_sequence<sizeof...(Vals)>{});
}

template <typename... Vals>
template <typename DS, std::size_t... Is>
auto queryosity::dataset::columns<Vals...>::_read(
    queryosity::dataset::loaded<DS> &ds, std::index_sequence<Is...>) const {
  return std::make_tuple(std::get<Is>(m_columns)._read(ds)...);
}

#include <iostream>
#include <memory>
#include <set>
#include <type_traits>
#include <vector>

#include <type_traits>

#define CHECK_FOR_BINARY_OP(op_name, op_symbol)                                \
  struct has_no_##op_name {};                                                  \
  template <typename T, typename Arg>                                          \
  has_no_##op_name operator op_symbol(const T &, const Arg &);                 \
  template <typename T, typename Arg = T> struct has_##op_name {               \
    enum {                                                                     \
      value = !std::is_same<decltype(std::declval<T>()                         \
                                         op_symbol std::declval<Arg>()),       \
                            has_no_##op_name>::value                           \
    };                                                                         \
  };                                                                           \
  template <typename T, typename Arg = T>                                      \
  static constexpr bool has_##op_name##_v = has_##op_name<T, Arg>::value;

#define DEFINE_LAZY_BINARY_OP(op_name, op_symbol)                              \
  template <typename Arg, typename V = Action,                                 \
            std::enable_if_t<                                                  \
                queryosity::is_column_v<V> &&                                  \
                    queryosity::is_column_v<typename Arg::action_type> &&      \
                    detail::has_##op_name##_v<                                 \
                        column::value_t<V>,                           \
                        column::value_t<typename Arg::action_type>>,  \
                bool> = false>                                                 \
  auto operator op_symbol(Arg const &arg) const {                              \
    return this->m_df->define(                                                 \
        queryosity::column::expression(                                        \
            [](column::value_t<V> const &me,                          \
               column::value_t<typename Arg::action_type> const       \
                   &you) { return me op_symbol you; }),                        \
        *this, arg);                                                           \
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
  template <typename V = Action,                                               \
            std::enable_if_t<                                                  \
                queryosity::is_column_v<V> &&                                  \
                    detail::has_##op_name##_v<column::value_t<V>>,    \
                bool> = false>                                                 \
  auto operator op_symbol() const {                                            \
    return this->m_df->define(queryosity::column::expression(                  \
                                  [](column::value_t<V> const &me) {  \
                                    return (op_symbol me);                     \
                                  }),                                          \
                              *this);                                          \
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
  template <typename Arg, typename V = Action,                                 \
            std::enable_if_t<                                                  \
                is_column_v<V> &&                                              \
                    detail::has_subscript_v<                                   \
                        column::value_t<V>,                           \
                        column::value_t<typename Arg::action_type>>,  \
                bool> = false>                                                 \
  auto operator[](Arg const &arg) const {                                      \
    return this->m_df->define(                                                 \
        queryosity::column::expression(                                        \
            [](column::value_t<V> me,                                 \
               column::value_t<typename Arg::action_type> index) {    \
              return me[index];                                                \
            }),                                                                \
        *this, arg);                                                           \
  }

#define DECLARE_LAZY_VARIED_BINARY_OP(op_symbol)                               \
  template <typename Arg>                                                      \
  auto operator op_symbol(Arg &&b) const->typename lazy<                       \
      typename decltype(std::declval<lazy<Act>>().operator op_symbol(          \
          std::forward<Arg>(b).nominal()))::action_type>::varied;

#define DEFINE_LAZY_VARIED_BINARY_OP(op_symbol)                                \
  template <typename Act>                                                      \
  template <typename Arg>                                                      \
  auto queryosity::lazy<Act>::varied::operator op_symbol(Arg &&b) const->      \
      typename lazy<                                                           \
          typename decltype(std::declval<lazy<Act>>().operator op_symbol(      \
              std::forward<Arg>(b).nominal()))::action_type>::varied {         \
    auto syst = typename lazy<                                                 \
        typename decltype(std::declval<lazy<Act>>().operator op_symbol(        \
            std::forward<Arg>(b).nominal()))::action_type>::                   \
        varied(this->nominal().operator op_symbol(                             \
            std::forward<Arg>(b).nominal()));                                  \
    for (auto const &var_name :                                                \
         systematic::get_variation_names(*this, std::forward<Arg>(b))) {       \
      syst.set_variation(var_name,                                             \
                         variation(var_name).operator op_symbol(               \
                             std::forward<Arg>(b).variation(var_name)));       \
    }                                                                          \
    return syst;                                                               \
  }

#define DECLARE_LAZY_VARIED_UNARY_OP(op_symbol)                                \
  template <typename V = Act,                                                  \
            std::enable_if_t<queryosity::is_column_v<V>, bool> = false>        \
  auto operator op_symbol() const->typename lazy<                              \
      typename decltype(std::declval<lazy<V>>().                               \
                        operator op_symbol())::action_type>::varied;
#define DEFINE_LAZY_VARIED_UNARY_OP(op_name, op_symbol)                        \
  template <typename Act>                                                      \
  template <typename V, std::enable_if_t<queryosity::is_column_v<V>, bool>>    \
  auto queryosity::lazy<Act>::varied::operator op_symbol() const->             \
      typename lazy<                                                           \
          typename decltype(std::declval<lazy<V>>().                           \
                            operator op_symbol())::action_type>::varied {      \
    auto syst =                                                                \
        typename lazy<typename decltype(std::declval<lazy<V>>().               \
                                        operator op_symbol())::action_type>::  \
            varied(this->nominal().operator op_symbol());                      \
    for (auto const &var_name : systematic::get_variation_names(*this)) {      \
      syst.set_variation(var_name, variation(var_name).operator op_symbol());  \
    }                                                                          \
    return syst;                                                               \
  }

namespace queryosity {

namespace detail {

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

// mixin class to conditionally add a member variable
template <typename Action, typename Enable = void>
struct result_if_aggregation {};

// Specialization for types satisfying is_query
template <typename Action>
struct result_if_aggregation<
    Action, std::enable_if_t<query::is_aggregation_v<Action>>> {
  using result_type = decltype(std::declval<Action>().result());
  result_if_aggregation() : m_merged(false) {}
  virtual ~result_if_aggregation() = default;

protected:
  result_type m_result;
  bool m_merged;
};

/**
 * @ingroup api
 * @brief Lazy action over dataset.
 * @tparam Action Action to be performed.
 */
template <typename Action>
class lazy : public dataflow::node,
             public ensemble::slotted<Action>,
             public systematic::resolver<lazy<Action>>,
             public result_if_aggregation<Action> {

public:
  class varied;

public:
  using action_type = Action;

public:
  friend class dataflow;
  template <typename> friend class lazy;

public:
  lazy(dataflow &df, std::vector<Action *> const &slots)
      : dataflow::node(df), m_slots(slots) {}

  template <typename Derived>
  lazy(dataflow &df, std::vector<Derived *> const &slots);
  template <typename Derived>
  lazy(dataflow &df, std::vector<std::unique_ptr<Derived>> const &slots);

  lazy(const lazy &) = default;
  lazy &operator=(const lazy &) = default;

  lazy(lazy &&) = default;
  lazy &operator=(lazy &&) = default;

  virtual ~lazy() = default;

  virtual std::vector<Action *> const &get_slots() const override;

  virtual void set_variation(const std::string &var_name, lazy var) override;

  virtual lazy &nominal() override;
  virtual lazy &variation(const std::string &var_name) override;
  virtual lazy const &nominal() const override;
  virtual lazy const &variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> get_variation_names() const override;

  template <typename To, typename V = Action,
            std::enable_if_t<queryosity::is_column_v<V>, bool> = false>
  auto to() const -> lazy<column::valued<To>>;

  template <typename Col> auto filter(lazy<Col> const &col) const;
  template <typename Col> auto weight(lazy<Col> const &col) const;

  template <typename Col> auto filter(Col const &col) const;
  template <typename Col> auto weight(Col const &col) const;

  template <typename Expr, typename... Cols>
  auto filter(queryosity::column::expression<Expr> const &expr,
              Cols const &...cols) const;

  template <typename Expr, typename... Cols>
  auto weight(queryosity::column::expression<Expr> const &expr,
              Cols const &...cols) const;

  template <typename Agg> auto book(Agg &&agg) const;
  template <typename... Aggs> auto book(Aggs &&...aggs) const;

  template <typename V = Action,
            std::enable_if_t<queryosity::query::is_aggregation_v<V>,
                             bool> = false>
  auto result() -> decltype(std::declval<V>().result());

  template <typename V = Action,
            std::enable_if_t<queryosity::query::is_aggregation_v<V>,
                             bool> = false>
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
  DEFINE_LAZY_BINARY_OP(logical_or, ||)
  DEFINE_LAZY_BINARY_OP(logical_and, &&)
  DEFINE_LAZY_BINARY_OP(greater_than, >)
  DEFINE_LAZY_BINARY_OP(less_than, <)
  DEFINE_LAZY_BINARY_OP(greater_than_or_equal_to, >=)
  DEFINE_LAZY_BINARY_OP(less_than_or_equal_to, <=)
  DEFINE_LAZY_INDEX_OP()

protected:
  template <typename V = Action,
            std::enable_if_t<queryosity::query::is_aggregation_v<V>,
                             bool> = false>
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

/**
 * @ingroup api
 * @brief Variations of a lazy action.
 * @details A varied lazy node encapsulates independent nominal and variations
 * of a lazy action.
 * @tparam Action Action to be performed.
 */
template <typename Act>
class lazy<Act>::varied : public dataflow::node,
                          public systematic::resolver<lazy<Act>> {

public:
  using action_type = typename lazy<Act>::action_type;

public:
  varied(lazy<Act> nom);
  ~varied() = default;

  varied(varied &&) = default;
  varied &operator=(varied &&) = default;

  virtual void set_variation(const std::string &var_name, lazy var) override;

  virtual lazy &nominal() override;
  virtual lazy &variation(const std::string &var_name) override;
  virtual lazy const &nominal() const override;
  virtual lazy const &variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> get_variation_names() const override;

  /**
   * Apply a filter.
   */
  template <typename Col, typename V = Act,
            std::enable_if_t<queryosity::is_selection_v<V>, bool> = false>
  auto filter(Col const &col) -> typename lazy<selection::node>::varied;

  template <typename Col, typename V = Act,
            std::enable_if_t<queryosity::is_selection_v<V>, bool> = false>
  auto weight(Col const &col) -> typename lazy<selection::node>::varied;

  template <typename Expr, typename... Args, typename V = Act,
            std::enable_if_t<queryosity::is_selection_v<V>, bool> = false>
  auto filter(column::expression<Expr> const &expr, Args &&...args) ->
      typename lazy<selection::node>::varied;

  template <typename Expr, typename... Args, typename V = Act,
            std::enable_if_t<queryosity::is_selection_v<V>, bool> = false>
  auto weight(column::expression<Expr> const &expr, Args &&...args) ->
      typename lazy<selection::node>::varied;

  template <typename Agg, typename V = Act,
            std::enable_if_t<queryosity::is_selection_v<V>, bool> = false>
  auto book(Agg &&agg);

  template <typename... Aggs, typename V = Act,
            std::enable_if_t<queryosity::is_selection_v<V>, bool> = false>
  auto book(Aggs &&...aggs);

  template <typename V = Act,
            std::enable_if_t<queryosity::query::is_aggregation_v<V>,
                             bool> = false>
  auto operator[](const std::string &var_name) -> lazy<V> &;
  template <typename V = Act,
            std::enable_if_t<queryosity::query::is_aggregation_v<V>,
                             bool> = false>
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
queryosity::lazy<Act>::varied::varied(lazy<Act> nom)
    : dataflow::node(*nom.m_df), m_nom(std::move(nom)) {}

template <typename Act>
void queryosity::lazy<Act>::varied::set_variation(const std::string &var_name,
                                                  lazy var) {
  ensemble::call([var_name](action *act) { act->set_variation_name(var_name); },
                 var.get_slots());
  m_var_map.insert(std::make_pair(var_name, std::move(var)));
  m_var_names.insert(var_name);
}

template <typename Act>
auto queryosity::lazy<Act>::varied::nominal() -> lazy & {
  return this->m_nom;
}

template <typename Act>
auto queryosity::lazy<Act>::varied::variation(const std::string &var_name)
    -> lazy & {
  return (this->has_variation(var_name) ? m_var_map.at(var_name) : m_nom);
}

template <typename Act>
auto queryosity::lazy<Act>::varied::nominal() const -> lazy const & {
  return this->m_nom;
}

template <typename Act>
auto queryosity::lazy<Act>::varied::variation(const std::string &var_name) const
    -> lazy const & {
  return (this->has_variation(var_name) ? m_var_map.at(var_name) : m_nom);
}

template <typename Act>
bool queryosity::lazy<Act>::varied::has_variation(
    const std::string &var_name) const {
  return m_var_map.find(var_name) != m_var_map.end();
}

template <typename Act>
std::set<std::string>
queryosity::lazy<Act>::varied::get_variation_names() const {
  return m_var_names;
}

template <typename Act>
template <typename Col, typename V,
          std::enable_if_t<queryosity::is_selection_v<V>, bool>>
auto queryosity::lazy<Act>::varied::filter(Col const &col) ->
    typename lazy<selection::node>::varied {

  using varied_type = typename lazy<selection::node>::varied;

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
auto queryosity::lazy<Act>::varied::weight(Col const &col) ->
    typename lazy<selection::node>::varied {

  using varied_type = typename lazy<selection::node>::varied;

  auto syst = varied_type(this->nominal().weight(col.nominal()));

  for (auto const &var_name : systematic::get_variation_names(*this, col)) {
    syst.set_variation(
        var_name, this->variation(var_name).weight(col.variation(var_name)));
  }
  return syst;
}

template <typename Act>
template <typename Expr, typename... Args, typename V,
          std::enable_if_t<queryosity::is_selection_v<V>, bool>>
auto queryosity::lazy<Act>::varied::filter(
    queryosity::column::expression<Expr> const &expr, Args &&...args) ->
    typename lazy<selection::node>::varied {

  using varied_type = typename lazy<selection::node>::varied;

  auto syst = varied_type(
      this->nominal().filter(expr, std::forward<Args>(args).nominal()...));

  for (auto const &var_name :
       systematic::get_variation_names(*this, std::forward<Args>(args)...)) {
    syst.set_variation(
        var_name, this->variation(var_name).filter(
                      expr, std::forward<Args>(args).variation(var_name)...));
  }
  return syst;
}

template <typename Act>
template <typename Expr, typename... Args, typename V,
          std::enable_if_t<queryosity::is_selection_v<V>, bool>>
auto queryosity::lazy<Act>::varied::weight(
    queryosity::column::expression<Expr> const &expr, Args &&...args) ->
    typename lazy<selection::node>::varied {

  using varied_type = typename lazy<selection::node>::varied;

  auto syst = varied_type(
      this->nominal().weight(expr, std::forward<Args>(args).nominal()...));

  for (auto const &var_name :
       systematic::get_variation_names(*this, std::forward<Args>(args)...)) {
    syst.set_variation(
        var_name, this->variation(var_name).weight(
                      expr, std::forward<Args>(args).variation(var_name)...));
  }
  return syst;
}

template <typename Act>
template <typename Agg, typename V,
          std::enable_if_t<queryosity::is_selection_v<V>, bool>>
auto queryosity::lazy<Act>::varied::book(Agg &&agg) {
  return agg.book(*this);
}

template <typename Act>
template <typename... Aggs, typename V,
          std::enable_if_t<queryosity::is_selection_v<V>, bool>>
auto queryosity::lazy<Act>::varied::book(Aggs &&...aggs) {
  return std::make_tuple((aggs.book(*this), ...));
}

template <typename Act>
template <
    typename V,
    std::enable_if_t<queryosity::query::is_aggregation_v<V>, bool>>
auto queryosity::lazy<Act>::varied::operator[](const std::string &var_name)
    -> lazy<V> & {
  if (!this->has_variation(var_name)) {
    throw std::out_of_range("variation does not exist");
  }
  return this->variation(var_name);
}

template <typename Act>
template <
    typename V,
    std::enable_if_t<queryosity::query::is_aggregation_v<V>, bool>>
auto queryosity::lazy<Act>::varied::operator[](
    const std::string &var_name) const -> lazy<V> const & {
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
    return this->m_df->template _select<selection::cut>(*this, col);
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "filter must be called from a selection");
  }
}

template <typename Action>
template <typename Col>
auto queryosity::lazy<Action>::weight(lazy<Col> const &col) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    return this->m_df->template _select<selection::weight>(*this, col);
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "filter must be called from a selection");
  }
}

template <typename Action>
template <typename Col>
auto queryosity::lazy<Action>::filter(Col const &col) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    using varied_type = typename lazy<selection::node>::varied;
    auto syst = varied_type(*this->m_df, this->filter(col.nominal()));
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
    using varied_type = typename lazy<selection::node>::varied;
    auto syst = varied_type(*this->m_df, this->weight(col.nominal()));
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
template <typename Expr, typename... Cols>
auto queryosity::lazy<Action>::filter(
    queryosity::column::expression<Expr> const &expr,
    Cols const &...cols) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    auto col = this->m_df->define(expr, cols...);
    return this->filter(col);
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "filter must be called from a selection");
  }
}

template <typename Action>
template <typename Expr, typename... Cols>
auto queryosity::lazy<Action>::weight(
    queryosity::column::expression<Expr> const &expr,
    Cols const &...cols) const {
  if constexpr (std::is_base_of_v<selection::node, Action>) {
    auto col = this->m_df->define(expr, cols...);
    return this->weight(col);
  } else {
    static_assert(std::is_base_of_v<selection::node, Action>,
                  "filter must be called from a selection");
  }
}

template <typename Action>
template <typename Agg>
auto queryosity::lazy<Action>::book(Agg &&agg) const {
  static_assert(std::is_base_of_v<selection::node, Action>,
                "book must be called from a selection");
  return agg.book(*this);
}

template <typename Action>
template <typename... Aggs>
auto queryosity::lazy<Action>::book(Aggs &&...aggs) const {
  static_assert(std::is_base_of_v<selection::node, Action>,
                "book must be called from a selection");
  return std::make_tuple((aggs.book(*this), ...));
}

template <typename Action>
template <
    typename V,
    std::enable_if_t<queryosity::query::is_aggregation_v<V>, bool>>
auto queryosity::lazy<Action>::result()
    -> decltype(std::declval<V>().result()) {
  this->m_df->analyze();
  this->merge_results();
  return this->m_result;
}

template <typename Action>
template <
    typename V,
    std::enable_if_t<queryosity::query::is_aggregation_v<V>, bool> e>
void queryosity::lazy<Action>::merge_results() {
  if (this->m_merged)
    return;
  auto model = this->get_slot(0);
  using result_type = decltype(model->result());
  const auto nslots = this->size();
  if (nslots == 1) {
    this->m_result = model->result();
  } else {
    std::vector<result_type> results;
    results.reserve(nslots);
    for (size_t islot = 0; islot < nslots; ++islot) {
      results.push_back(std::move(this->get_slot(islot)->result()));
    }
    this->m_result = model->merge(results);
  }
  this->m_merged = true;
}

#include <set>
#include <string>
#include <type_traits>

namespace queryosity {

template <typename... Args> class systematic::variation {

public:
  variation(const std::string &name, Args... args)
      : m_name(name), m_args(args...) {}

  std::string const &name() const { return m_name; }
  std::tuple<Args...> const &args() const { return m_args; }

protected:
  std::string m_name;
  std::tuple<Args...> m_args;
};

template <typename Action> class systematic::variation<lazy<Action>> {

public:
  variation(const std::string &name, lazy<Action> const &var);
  virtual ~variation() = default;

  auto name() const -> std::string;
  auto get() const -> lazy<Action> const &;

protected:
  std::string m_name;
  lazy<Action> const &m_var;
};

} // namespace queryosity

template <typename Action>
queryosity::systematic::variation<queryosity::lazy<Action>>::variation(
    const std::string &name, queryosity::lazy<Action> const &var)
    : m_name(name), m_var(var) {}

template <typename Action>
auto queryosity::systematic::variation<queryosity::lazy<Action>>::name() const
    -> std::string {
  return m_name;
}

template <typename Action>
auto queryosity::systematic::variation<queryosity::lazy<Action>>::get() const
    -> queryosity::lazy<Action> const & {
  return m_var;
}

template <typename DS>
queryosity::dataset::loaded<DS>::loaded(queryosity::dataflow &df, DS &ds)
    : m_df(&df), m_ds(&ds) {}

template <typename DS>
template <typename Val>
auto queryosity::dataset::loaded<DS>::read(dataset::column<Val> const &col)
    -> lazy<read_column_t<DS, Val>> {
  return col.template _read(*this);
}

template <typename DS>
template <typename... Vals>
auto queryosity::dataset::loaded<DS>::read(
    dataset::columns<Vals...> const &cols) {
  return cols.template _read(*this);
}

template <typename DS>
template <typename Val>
auto queryosity::dataset::loaded<DS>::vary(
    dataset::column<Val> const &col,
    systematic::variation<std::string> const &var) {
  auto nom = this->read(col);
  typename decltype(nom)::varied varied_column(std::move(nom));
  varied_column.set_variation(
      var.name(), this->read(dataset::column<Val>(std::get<0>(var.args()))));
  return varied_column;
}

template <typename DS>
template <typename Val, typename... Vars>
auto queryosity::dataset::loaded<DS>::vary(
    dataset::column<Val> const &col,
    systematic::variation<Vars> const &...vars) {
  auto nom = this->read(col);
  typename decltype(nom)::varied varied_column(std::move(nom));
  (varied_column.set_variation(
       vars.name(), this->read(dataset::column<Val>(std::get<0>(vars.args())))),
   ...);
  return varied_column;
}

namespace queryosity {

class dataflow;

template <typename Val> class lazy;

/**
 * @brief Define a constant column in dataflow.
 */
template <typename Val> class column::constant {

public:
  constant(Val const &val);
  ~constant() = default;

  auto _assign(dataflow &df) const -> lazy<column::fixed<Val>>;

protected:
  Val m_val;
};

} // namespace queryosity

template <typename Val>
queryosity::column::constant<Val>::constant(Val const &val) : m_val(val) {}

template <typename Val>
auto queryosity::column::constant<Val>::_assign(queryosity::dataflow &df) const
    -> lazy<column::fixed<Val>> {
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
 * @brief Define a column evaluated out of an expression.
 */
template <typename Expr> class expression {

public:
  using function_type = decltype(std::function(std::declval<Expr>()));
  using equation_type = equation_t<Expr>;

public:
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
    : m_expression(expr) {}

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

#include <functional>
#include <memory>
#include <tuple>

namespace queryosity {

class dataflow;

namespace query {

/**
 * @brief Plan a query
 * @tparam Qry Concrete implementation of
 * queryosity::query::definition<T(Obs...)>.
 */
template <typename Qry> class plan {

public:
  /**
   * @brief Constructor.
   * @tparam Args `Qry` constructor argument types.
   * @param args Constructor arguments.
   */
  template <typename... Args> plan(Args const &...args);

  auto _make(dataflow &df) const;

protected:
  std::function<todo<query::book<Qry>>(dataflow &)> m_make;
};

} // namespace query

} // namespace queryosity

template <typename Qry>
template <typename... Args>
queryosity::query::plan<Qry>::plan(Args const &...args)
    : m_make([args...](dataflow &df) { return df._make<Qry>(args...); }) {}

template <typename Qry>
auto queryosity::query::plan<Qry>::_make(queryosity::dataflow &df) const {
  return this->m_make(df);
}

namespace queryosity {

template <typename T> class lazy;

namespace systematic {

template <typename Lzy> class nominal {

public:
  nominal(Lzy const &nom);
  virtual ~nominal() = default;

  auto get() const -> Lzy const &;

protected:
  Lzy const &m_nom;
};

} // namespace systematic

} // namespace queryosity

template <typename Lzy>
queryosity::systematic::nominal<Lzy>::nominal(Lzy const &nom) : m_nom(nom) {}

template <typename Lzy>
auto queryosity::systematic::nominal<Lzy>::get() const -> Lzy const & {
  return m_nom;
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

template <typename Fn, typename... Cols>
auto queryosity::dataflow::define(
    queryosity::column::expression<Fn> const &expr, lazy<Cols> const &...cols)
    -> lazy<column::equation_t<Fn>> {
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

template <typename Fn, typename... Cols>
auto queryosity::dataflow::filter(
    queryosity::column::expression<Fn> const &expr, Cols const &...cols) {
  auto dec = this->define(expr, cols...);
  return this->filter(dec);
}

template <typename Fn, typename... Cols>
auto queryosity::dataflow::weight(
    queryosity::column::expression<Fn> const &expr, Cols const &...cols) {
  auto dec = this->define(expr, cols...);
  return this->weight(dec);
}

template <typename Qry, typename... Args>
auto queryosity::dataflow::_make(Args &&...args) -> todo<query::book<Qry>> {
  return todo<query::book<Qry>>(*this, ensemble::invoke(
                                           [&args...](dataset::player *plyr) {
                                             return plyr->template make<Qry>(
                                                 std::forward<Args>(args)...);
                                           },
                                           m_processor.get_slots()));
}

template <typename Qry>
auto queryosity::dataflow::make(queryosity::query::plan<Qry> const &cntr)
    -> todo<query::book<Qry>> {
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
auto queryosity::dataflow::_book(todo<query::book<Qry>> const &bkr,
                                 lazy<selection::node> const &sel)
    -> lazy<Qry> {
  // new query booked: dataset will need to be analyzed
  this->reset();
  auto act = ensemble::invoke(
      [](dataset::player *plyr, query::book<Qry> *bkr,
         const selection::node *sel) { return plyr->book(*bkr, *sel); },
      m_processor.get_slots(), bkr.get_slots(), sel.get_slots());
  auto lzy = lazy<Qry>(*this, act);
  return lzy;
}

template <typename Qry, typename... Sels>
auto queryosity::dataflow::_book(todo<query::book<Qry>> const &bkr,
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

template <typename Fn, typename... Vars>
auto queryosity::dataflow::vary(queryosity::column::expression<Fn> const &expr,
                                Vars const &...vars) {
  auto nom = this->_equate(expr);
  using varied_type = typename decltype(nom)::varied;
  using function_type = typename column::expression<Fn>::function_type;
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
  return todo<queryosity::column::evaluator<Def>>(
      *this,
      ensemble::invoke(
          [&args...](dataset::player *plyr) {
            return plyr->template define<Def>(std::forward<Args>(args)...);
          },
          m_processor.get_slots()));
}

template <typename Def>
auto queryosity::dataflow::_define(
    queryosity::column::definition<Def> const &defn) -> todo<column::evaluator<Def>>  {
  return defn._define(*this);
}

template <typename Fn> auto queryosity::dataflow::_equate(Fn fn){
  return todo<column::evaluator<
      typename column::equation_t<Fn>>>(
      *this,
      ensemble::invoke(
          [fn](dataset::player *plyr) { return plyr->template equate(fn); },
          m_processor.get_slots()));
}

template <typename Fn>
auto queryosity::dataflow::_equate(
    queryosity::column::expression<Fn> const &expr) -> todo<column::evaluator<column::equation_t<Fn>>> {
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

template <typename Syst, typename Fn>
void queryosity::dataflow::_vary(
    Syst &syst, const std::string &name,
    queryosity::column::expression<Fn> const &expr) {
  syst.set_variation(name, this->_equate(expr));
}

namespace queryosity {

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
  class varied;

public:
  todo(dataflow &df, std::vector<std::unique_ptr<Helper>> bkr);
  virtual ~todo() = default;

  todo(todo &&) = default;
  todo &operator=(todo &&) = default;

  virtual std::vector<Helper *> const &get_slots() const override;

  virtual void set_variation(const std::string &var_name, todo var) override;

  virtual todo &nominal() override;
  virtual todo &variation(const std::string &var_name) override;
  virtual todo const &nominal() const override;
  virtual todo const &variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> get_variation_names() const override;

  /**
   * @brief Evaluate the column definition with input columns.
   * @param[in] columns Input columns.
   * @param[in][out] Evaluated column.
   */
  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::column::is_evaluatable_v<V>,
                             bool> = false>
  auto evaluate(Nodes &&...columns) const
      -> decltype(std::declval<todo<V>>()._evaluate(
          std::forward<Nodes>(columns)...)) {
    return this->_evaluate(std::forward<Nodes>(columns)...);
  }

  /**
   * @brief Fill query with input columns per-entry.
   * @param[in] columns Input columns.
   * @returns Updated query plan filled with input columns.
   */
  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::query::is_bookable_v<V>,
                             bool> = false>
  auto fill(Nodes &&...columns) const
      -> decltype(std::declval<todo<V>>()._fill(std::declval<Nodes>()...)) {
    return this->_fill(std::forward<Nodes>(columns)...);
  }

  /**
   * @brief Book a query at a selection.
   * @param[in] sel Selection node at which query is counted/filled.
   * @return The query booked at the selection.
   */
  template <typename Node> auto book(Node &&selection) const {
    return this->_book(std::forward<Node>(selection));
  }

  /**
   * @brief Book a query at multiple selections.
   * @tparam Sels... Selections.
   * @param[in] sels... selection nodes.
   * @return `std::tuple` of queries booked at each selection.
   */
  template <typename... Sels> auto book(Sels &&...sels) const {
    static_assert(query::is_bookable_v<Helper>, "not bookable");
    return this->_book(std::forward<Sels>(sels)...);
  }

  /**
   * @brief Shorthand for `evaluate()`.
   * @tparam Args... Input column types.
   * @param[in] columns... Input columns.
   * @return Evaluated column.
   */
  template <typename... Args, typename V = Helper,
            std::enable_if_t<column::is_evaluatable_v<V>,
                             bool> = false>
  auto operator()(Args &&...columns) const
      -> decltype(std::declval<todo<V>>().evaluate(
          std::forward<Args>(std::declval<Args &&>())...)) {
    return this->evaluate(std::forward<Args>(columns)...);
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
  auto _evaluate(Nodes const &...columns) const ->
      typename lazy<column::evaluated_t<V>>::varied {

    using varied_type = typename lazy<column::evaluated_t<V>>::varied;

    auto nom = this->m_df->_evaluate(this, columns.nominal()...);
    auto syst = varied_type(std::move(nom));

    for (auto const &var_name : systematic::get_variation_names(columns...)) {
      auto var = this->m_df->_evaluate(*this, columns.variation(var_name)...);
      syst.set_variation(var_name, std::move(var));
    }

    return syst;
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
  auto _book(Node const &sel) const ->
      typename lazy<query::booked_t<V>>::varied {
    using varied_type = typename lazy<query::booked_t<V>>::varied;
    auto syst = varied_type(this->m_df->_book(*this, sel.nominal()));
    for (auto const &var_name : systematic::get_variation_names(sel)) {
      syst.set_variation(var_name,
                         this->m_df->_book(*this, sel.variation(var_name)));
    }
    return syst;
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
      -> std::array<typename lazy<query::booked_t<V>>::varied,
                    sizeof...(Nodes)> {
    using varied_type = typename lazy<query::booked_t<V>>::varied;
    using array_of_varied_type =
        std::array<typename lazy<query::booked_t<V>>::varied, sizeof...(Nodes)>;
    auto var_names = systematic::get_variation_names(sels...);
    auto _book_varied =
        [var_names,
         this](systematic::resolver<lazy<selection::node>> const &sel) {
          auto syst = varied_type(this->m_df->_book(*this, sel.nominal()));
          for (auto const &var_name : var_names) {
            syst.set_variation(
                var_name, this->m_df->_book(*this, sel.variation(var_name)));
          }
          return syst;
        };
    return array_of_varied_type{_book_varied(sels)...};
  }

  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::query::is_bookable_v<V> &&
                                 queryosity::has_no_variation_v<Nodes...>,
                             bool> = false>
  auto _fill(Nodes const &...columns) const -> todo<V> {
    return todo<V>(*this->m_df,
                   ensemble::invoke(
                       [](V *fillable, typename Nodes::action_type *...cols) {
                         return fillable->book_fill(*cols...);
                       },
                       this->get_slots(), columns.get_slots()...));
  }

  template <typename... Nodes, typename V = Helper,
            std::enable_if_t<queryosity::query::is_bookable_v<V> &&
                                 has_variation_v<Nodes...>,
                             bool> = false>
  auto _fill(Nodes const &...columns) const -> varied {
    auto syst = varied(std::move(this->_fill(columns.nominal()...)));
    for (auto const &var_name : systematic::get_variation_names(columns...)) {
      syst.set_variation(
          var_name, std::move(this->_fill(columns.variation(var_name)...)));
    }
    return syst;
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

template <typename Helper> auto queryosity::todo<Helper>::nominal() -> todo & {
  // this is nominal
  return *this;
}

template <typename Helper>
auto queryosity::todo<Helper>::variation(const std::string &) -> todo & {
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
  /**
   * @brief Default constructor.
   */
  definition() = default;
  /**
   * @brief Default destructor.
   */
  virtual ~definition() = default;

public:
  virtual Out calculate() const final override;

  /**
   * @brief Compute the quantity of interest for the entry
   * @note Columns passed in as observables are not computed until `value()` is
   * called.
   * @param[in] args Input observables.
   */
  virtual Out evaluate(observable<Ins>... args) const = 0;

  template <typename... Args> void set_arguments(const view<Args> &...args);

protected:
  vartuple_type m_arguments;
};

/**
 * @ingroup api
 * @brief Define a custom column in dataflow.
 * @tparam Def Concrete queryosity::column::definition<Out(Ins...)>
 * implementation
 */
template <typename Def> class column::definition {

public:
  /**
   * @param[in] args Constructor arguments of @p Def.
   * @brief Define a custom column in dataflow.
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

/**
 * @brief Query filled with column value(s) per-entry.
 * @tparam T Output result type.
 * @tparam Obs Input column data types.
 */
template <typename T, typename... Obs>
class query::definition<T(Obs...)> : public query::aggregation<T> {

public:
  using vartup_type = std::tuple<column::variable<Obs>...>;

public:
  definition() = default;
  virtual ~definition() = default;

  /**
   * @brief Perform the counting action for an entry.
   * @param[in] observables The `observable` of each input column.
   * @param[in] weight The weight value of the booked selection for the passed
   * entry.
   * @details This action is performed N times for a passed entry, where N is
   * the number of `fill` calls made during its `todo` configuration.
   */
  virtual void fill(column::observable<Obs>... observables, double w) = 0;
  virtual void count(double w) final override;

  template <typename... Vals>
  void enter_columns(column::view<Vals> const &...cols);

protected:
  std::vector<vartup_type> m_fills;
};

} // namespace queryosity

template <typename T, typename... Obs>
template <typename... Vals>
void queryosity::query::definition<T(Obs...)>::enter_columns(
    column::view<Vals> const &...cols) {
  static_assert(sizeof...(Obs) == sizeof...(Vals),
                "dimension mis-match between filled variables & columns.");
  m_fills.emplace_back(cols...);
}

template <typename T, typename... Obs>
void queryosity::query::definition<T(Obs...)>::count(double w) {
  for (unsigned int ifill = 0; ifill < m_fills.size(); ++ifill) {
    std::apply(
        [this, w](const column::variable<Obs> &...obs) {
          this->fill(obs..., w);
        },
        m_fills[ifill]);
  }
}

namespace queryosity {

/**
 * @ingroup api
 * @brief Varied version of a todo item.
 * @details A todo varied item is functionally equivalent to a todo
 * node with each method being propagated to independent todo nodes
 * corresponding to nominal and systematic variations.
 */
template <typename Bld>
class todo<Bld>::varied : public dataflow::node,
                          systematic::resolver<todo<Bld>> {

public:
  varied(todo<Bld> &&nom);
  ~varied() = default;

  varied(varied &&) = default;
  varied &operator=(varied &&) = default;

  virtual void set_variation(const std::string &var_name, todo var) override;

  virtual todo &nominal() override;
  virtual todo &variation(const std::string &var_name) override;
  virtual todo const &nominal() const override;
  virtual todo const &variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> get_variation_names() const override;

public:
  template <typename... Args, typename V = Bld,
            std::enable_if_t<queryosity::column::is_evaluatable_v<V>,
                             bool> = false>
  auto evaluate(Args &&...args) ->
      typename queryosity::lazy<column::evaluated_t<V>>::varied;

  /**
   * @brief Fill the query with input columns.
   * @param[in] columns... Input columns to fill the query with.
   * @return A new todo query node with input columns filled.
   */
  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<queryosity::query::is_bookable_v<V>,
                             bool> = false>
  auto fill(Nodes const &...columns) -> varied;

  /**
   * @brief Book the query logic at a selection.
   * @param[in] selection Lazy selection to book query at.
   * @return Lazy query booked at selection.
   */
  template <typename Node, typename V = Bld,
            std::enable_if_t<queryosity::query::is_bookable_v<V>,
                             bool> = false>
  auto book(Node const &selection) -> typename lazy<query::booked_t<V>>::varied;

  /**
   * @brief Book the query logic at multiple selections.
   * @param[in] selection Lazy selection to book queries at.
   * @return Delayed query containing booked lazy queries.
   */
  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<queryosity::query::is_bookable_v<V>,
                             bool> = false>
  auto book(Nodes const &...selections)
      -> std::array<typename lazy<query::booked_t<V>>::varied,
                    sizeof...(Nodes)>;

  /**
   * @brief Evaluate the column definition with input columns.
   * @param[in] args... Lazy input columns
   * @return Lazy column definition
   */
  template <typename... Args>
  auto operator()(Args &&...args) ->
      typename lazy<typename decltype(std::declval<todo<Bld>>().operator()(
          std::forward<Args>(args).nominal()...))::action_type>::varied;

protected:
  todo<Bld> m_nominal;
  std::unordered_map<std::string, todo<Bld>> m_variation_map;
  std::set<std::string> m_variation_names;
};

} // namespace queryosity

template <typename Bld>
queryosity::todo<Bld>::varied::varied(todo<Bld> &&nom)
    : dataflow::node(*nom.m_df), m_nominal(std::move(nom)) {}

template <typename Bld>
void queryosity::todo<Bld>::varied::set_variation(const std::string &var_name,
                                                  todo var) {
  m_variation_map.insert(std::move(std::make_pair(var_name, std::move(var))));
  m_variation_names.insert(var_name);
}

template <typename Bld>
auto queryosity::todo<Bld>::varied::nominal() -> todo & {
  return m_nominal;
}

template <typename Bld>
auto queryosity::todo<Bld>::varied::nominal() const -> todo const & {
  return m_nominal;
}

template <typename Bld>
auto queryosity::todo<Bld>::varied::variation(const std::string &var_name)
    -> todo & {
  return (this->has_variation(var_name) ? m_variation_map.at(var_name)
                                        : m_nominal);
}

template <typename Bld>
auto queryosity::todo<Bld>::varied::variation(const std::string &var_name) const
    -> todo const & {
  return (this->has_variation(var_name) ? m_variation_map.at(var_name)
                                        : m_nominal);
}

template <typename Bld>
bool queryosity::todo<Bld>::varied::has_variation(
    const std::string &var_name) const {
  return m_variation_map.find(var_name) != m_variation_map.end();
}

template <typename Bld>
std::set<std::string>
queryosity::todo<Bld>::varied::get_variation_names() const {
  return m_variation_names;
}

template <typename Bld>
template <
    typename... Args, typename V,
    std::enable_if_t<queryosity::column::is_evaluatable_v<V>, bool>>
auto queryosity::todo<Bld>::varied::evaluate(Args &&...args) ->
    typename queryosity::lazy<column::evaluated_t<V>>::varied {
  using varied_type =
      typename queryosity::lazy<column::evaluated_t<V>>::varied;
  auto syst = varied_type(
      this->nominal().evaluate(std::forward<Args>(args).nominal()...));
  for (auto const &var_name :
       systematic::get_variation_names(*this, std::forward<Args>(args)...)) {
    syst.set_variation(var_name,
                       variation(var_name).evaluate(
                           std::forward<Args>(args).variation(var_name)...));
  }
  return syst;
}

template <typename Bld>
template <typename... Nodes, typename V,
          std::enable_if_t<queryosity::query::is_bookable_v<V>, bool>>
auto queryosity::todo<Bld>::varied::fill(Nodes const &...columns) -> varied {
  auto syst = varied(std::move(this->nominal().fill(columns.nominal()...)));
  for (auto const &var_name :
       systematic::get_variation_names(*this, columns...)) {
    syst.set_variation(var_name, std::move(variation(var_name).fill(
                                     columns.variation(var_name)...)));
  }
  return syst;
}

template <typename Bld>
template <typename Node, typename V,
          std::enable_if_t<queryosity::query::is_bookable_v<V>, bool>>
auto queryosity::todo<Bld>::varied::book(Node const &selection) ->
    typename lazy<query::booked_t<V>>::varied {
  using varied_type = typename lazy<query::booked_t<V>>::varied;
  auto syst = varied_type(this->nominal().book(selection.nominal()));
  for (auto const &var_name :
       systematic::get_variation_names(*this, selection)) {
    syst.set_variation(var_name, this->variation(var_name).book(
                                     selection.variation(var_name)));
  }
  return syst;
}

template <typename Bld>
template <typename... Nodes, typename V,
          std::enable_if_t<queryosity::query::is_bookable_v<V>, bool>>
auto queryosity::todo<Bld>::varied::book(Nodes const &...selections)
    -> std::array<typename lazy<query::booked_t<V>>::varied, sizeof...(Nodes)> {
  // variations
  using varied_type = typename lazy<query::booked_t<V>>::varied;
  using array_of_varied_type =
      std::array<typename lazy<query::booked_t<V>>::varied, sizeof...(Nodes)>;
  auto var_names = systematic::get_variation_names(*this, selections...);
  auto _book_varied =
      [var_names,
       this](systematic::resolver<lazy<selection::node>> const &sel) {
        auto syst =
            varied_type(this->m_df->_book(this->nominal(), sel.nominal()));
        for (auto const &var_name : var_names) {
          syst.set_variation(var_name,
                             this->m_df->_book(this->variation(var_name),
                                               sel.variation(var_name)));
        }
        return syst;
      };
  return array_of_varied_type{_book_varied(selections)...};
}

template <typename Bld>
template <typename... Args>
auto queryosity::todo<Bld>::varied::operator()(Args &&...args) ->
    typename lazy<typename decltype(std::declval<todo<Bld>>().operator()(
        std::forward<Args>(args).nominal()...))::action_type>::varied {

  using varied_type =
      typename lazy<typename decltype(std::declval<todo<Bld>>().operator()(
          std::forward<Args>(args).nominal()...))::action_type>::varied;

  auto syst = varied_type(
      this->nominal().operator()(std::forward<Args>(args).nominal()...));
  for (auto const &var_name :
       systematic::get_variation_names(*this, std::forward<Args>(args)...)) {
    syst.set_variation(var_name,
                       variation(var_name).operator()(
                           std::forward<Args>(args).variation(var_name)...));
  }
  return syst;
}

#include <string>
#include <type_traits>

namespace queryosity {

template <typename T> class lazy;

namespace systematic {

template <typename Lzy, typename... Vars>
auto vary(systematic::nominal<Lzy> const &nom, Vars const &...vars);

} // namespace systematic

} // namespace queryosity

template <typename Lzy, typename... Vars>
auto queryosity::systematic::vary(systematic::nominal<Lzy> const &nom,
                                  Vars const &...vars) {
  using action_type = typename Lzy::action_type;
  using value_type = column::value_t<action_type>;
  using nominal_type = lazy<column::valued<value_type>>;
  using varied_type = typename nominal_type::varied;
  varied_type syst(nom.get().template to<value_type>());
  (syst.set_variation(vars.name(), vars.get().template to<value_type>()), ...);
  return syst;
}