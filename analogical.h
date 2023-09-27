#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <thread>
#include <type_traits>
#include <vector>

namespace ana {

template <typename T> class dataflow;

struct multithread {

public:
  static inline int s_suggestion = 0;
  static void enable(int suggestion = -1);
  static void disable();
  static bool status();
  static unsigned int concurrency();
};

namespace lockstep {

template <typename T> class slotted;
template <typename T> class node;
template <typename T> class view;

} // namespace lockstep

template <typename T> class lockstep::slotted {

public:
  virtual T *get_model() const = 0;
  virtual T *get_slot(size_t i) const = 0;
  virtual size_t concurrency() const = 0;
};

template <typename T> class lockstep::node : public lockstep::slotted<T> {

public:
  template <typename> friend class node;
  template <typename> friend class view;
  template <typename> friend class ana::dataflow;

public:
  node() = default;
  ~node() = default;

  node(const node &) = delete;
  node &operator=(const node &) = delete;

  node(node &&) = default;
  node &operator=(node &&) = default;

  template <typename U> node(node<U> &&derived);
  template <typename U> node &operator=(node<U> &&derived);

  void set_model(std::unique_ptr<T> model);
  void add_slot(std::unique_ptr<T> slot);
  void clear_slots();

  virtual T *get_model() const override;
  virtual T *get_slot(size_t i) const override;
  virtual size_t concurrency() const override;

  operator view<T>() const;

  /**
   * @brief Get the result of calling method(s) on the model.
   * @param fn Function to be called. The first argument must accept the slot
   * type by `const&`.
   * @param args Arguments to be applied for all slots to fn.
   * @return result The value of the result from model.
   * @details The result *must* be equal by value between the model and all
   * slots.
   */
  template <typename Fn, typename... Args>
  auto get_model_value(Fn const &fn, Args const &...args) const
      -> std::invoke_result_t<Fn, T const &, Args const &...>;

  /**
   * @brief Get the result of calling method(s) on each underlying model and
   * slots.
   * @param fn Function to be called. The first argument must accept the slot
   * type by reference.
   * @param args `concurrent<Args>` that holds an `Args const&` applied as the
   * argument for each slot call.
   * @details Call method(s) on all held underlying objects.
   */
  template <typename Fn, typename... Args>
  void call_all_slots(Fn const &fn, view<Args> const &...args) const;

  /**
   * @brief Get the result of calling method(s) on each underlying model and
   * slots.
   * @param fn Function to be called. The first argument must accept the slot
   * type by `&`, and return a `std::unique_ptr<Result>`.
   * @param args (Optional) arguments that are applied per-slot to fn.
   * @return result `concurrent<Result>` where `std::unique_ptr<Result>` is
   * returned by fn.
   * @details Preserve the concurrency of result of operations performed on the
   * underlying objects.
   */
  template <typename Fn, typename... Args>
  auto get_lockstep_node(Fn const &fn, view<Args> const &...args) const
      -> lockstep::node<
          typename std::invoke_result_t<Fn, T &, Args &...>::element_type>;

  template <typename Fn, typename... Args>
  auto get_lockstep_view(Fn const &fn, view<Args> const &...args) const
      -> lockstep::view<std::remove_pointer_t<
          typename std::invoke_result_t<Fn, T &, Args &...>>>;

  /**
   * @brief Run the function on the underlying slots, multi-threading if
   * enabled.
   * @param fn Function to be called. The first argument must accept the slot
   * type by `&`.
   * @param args (Optional) arguments that are applied per-slot to fn.
   * @details The methods are called on slots, while the model is left untouched
   * (as it is meant to represent the "merged" instance of all slots).
   */
  template <typename Fn, typename... Args>
  void run_slots(Fn const &fn, view<Args> const &...args) const;

protected:
  std::unique_ptr<T> m_model;
  std::vector<std::unique_ptr<T>> m_slots;
};

template <typename T> class lockstep::view : public lockstep::slotted<T> {

public:
  template <typename> friend class node;

public:
  view() = default;
  ~view() = default;

  view(const view &other) = default;
  view &operator=(const view &other) = default;

  template <typename U> view(const view<U> &derived);
  template <typename U> view &operator=(const view<U> &derived);
  template <typename U> view(const node<U> &derived);
  template <typename U> view &operator=(const node<U> &derived);

  virtual T *get_model() const override;
  virtual T *get_slot(size_t i) const override;
  virtual size_t concurrency() const override;

  template <typename Fn, typename... Args>
  auto get_model_value(Fn const &fn, Args const &...args) const
      -> std::invoke_result_t<Fn, T const &, Args const &...>;

protected:
  T *m_model;
  std::vector<T *> m_slots;
};

} // namespace ana

inline void ana::multithread::enable(int suggestion) {
  s_suggestion = suggestion;
}

inline void ana::multithread::disable() { s_suggestion = 0; }

inline bool ana::multithread::status() {
  return s_suggestion == 0 ? false : true;
}

inline unsigned int ana::multithread::concurrency() {
  return std::max<unsigned int>(
      1, s_suggestion < 0 ? std::thread::hardware_concurrency() : s_suggestion);
}

template <typename T>
template <typename U>
ana::lockstep::node<T>::node(node<U> &&derived) {
  static_assert(std::is_base_of_v<T, U>, "incompatible node types");
  this->m_model = std::move(derived.m_model);
  this->m_slots.clear();
  for (size_t i = 0; i < derived.concurrency(); ++i) {
    this->m_slots.emplace_back(std::move(derived.m_slots[i]));
  }
}

template <typename T>
template <typename U>
ana::lockstep::node<T> &ana::lockstep::node<T>::operator=(node<U> &&derived) {
  this->m_model = derived.m_model;
  this->m_slots.clear();
  for (size_t i = 0; i < derived.concurrency(); ++i) {
    this->m_slots.emplace_back(std::move(derived.m_slots[i]));
  }
  return *this;
}

template <typename T>
void ana::lockstep::node<T>::add_slot(std::unique_ptr<T> slot) {
  this->m_slots.emplace_back(std::move(slot));
}

template <typename T> void ana::lockstep::node<T>::clear_slots() {
  this->m_slots.clear();
}

template <typename T>
void ana::lockstep::node<T>::set_model(std::unique_ptr<T> model) {
  this->m_model = std::move(model);
}

template <typename T> T *ana::lockstep::node<T>::get_model() const {
  return this->m_model.get();
}

template <typename T> T *ana::lockstep::node<T>::get_slot(size_t i) const {
  return this->m_slots.at(i).get();
}

template <typename T> size_t ana::lockstep::node<T>::concurrency() const {
  return this->m_slots.size();
}

template <typename T> ana::lockstep::node<T>::operator view<T>() const {
  return view<T>(static_cast<lockstep::node<T> const &>(*this));
}

template <typename T>
template <typename Fn, typename... Args>
auto ana::lockstep::node<T>::get_model_value(Fn const &fn,
                                             Args const &...args) const
    -> std::invoke_result_t<Fn, T const &, Args const &...> {
  auto result = fn(static_cast<const T &>(*this->get_model()), args...);
  // result at each slot must match the model's
  for (size_t i = 0; i < concurrency(); ++i) {
    assert(result == fn(static_cast<const T &>(*this->get_slot(i)), args...));
  }
  return result;
}

template <typename T>
template <typename Fn, typename... Args>
auto ana::lockstep::node<T>::get_lockstep_node(Fn const &fn,
                                               view<Args> const &...args) const
    -> lockstep::node<
        typename std::invoke_result_t<Fn, T &, Args &...>::element_type> {
  assert(((concurrency() == args.concurrency()) && ...));
  typename lockstep::node<
      typename std::invoke_result_t<Fn, T &, Args &...>::element_type>
      invoked;
  invoked.m_model = std::move(fn(*this->get_model(), *args.get_model()...));
  invoked.m_slots.reserve(this->concurrency());
  for (size_t i = 0; i < this->concurrency(); ++i) {
    invoked.m_slots.emplace_back(fn(*this->get_slot(i), *args.get_slot(i)...));
  }
  return invoked;
}

template <typename T>
template <typename Fn, typename... Args>
auto ana::lockstep::node<T>::get_lockstep_view(Fn const &fn,
                                               view<Args> const &...args) const
    -> typename lockstep::view<std::remove_pointer_t<
        typename std::invoke_result_t<Fn, T &, Args &...>>> {
  assert(((concurrency() == args.concurrency()) && ...));
  typename lockstep::view<
      std::remove_pointer_t<typename std::invoke_result_t<Fn, T &, Args &...>>>
      invoked;
  invoked.m_model = fn(*this->get_model(), *args.get_model()...);
  invoked.m_slots.reserve(this->concurrency());
  for (size_t i = 0; i < this->concurrency(); ++i) {
    invoked.m_slots.push_back(fn(*this->get_slot(i), *args.get_slot(i)...));
  }
  return invoked;
}

template <typename T>
template <typename Fn, typename... Args>
void ana::lockstep::node<T>::call_all_slots(Fn const &fn,
                                            view<Args> const &...args) const {
  assert(((concurrency() == args.concurrency()) && ...));
  fn(*this->get_model(), *args.get_model()...);
  for (size_t i = 0; i < concurrency(); ++i) {
    fn(*this->get_slot(i), *args.get_slot(i)...);
  }
}

template <typename T>
template <typename Fn, typename... Args>
void ana::lockstep::node<T>::run_slots(Fn const &fn,
                                       view<Args> const &...args) const {
  assert(((concurrency() == args.concurrency()) && ...));

  // multi-lockstep
  if (multithread::status()) {
    std::vector<std::thread> pool;
    for (size_t islot = 0; islot < this->concurrency(); ++islot) {
      pool.emplace_back(fn, std::ref(*this->get_slot(islot)),
                        std::ref(*args.get_slot(islot))...);
    }
    for (auto &&thread : pool) {
      thread.join();
    }
    // single-lockstep
  } else {
    for (size_t islot = 0; islot < this->concurrency(); ++islot) {
      fn(*this->get_slot(islot), *args.get_slot(islot)...);
    }
  }
}

template <typename T>
template <typename U>
ana::lockstep::view<T>::view(const ana::lockstep::node<U> &derived) {
  static_assert(std::is_base_of_v<T, U>, "incompatible slot types");
  this->m_model = derived.m_model.get();
  this->m_slots.clear();
  for (size_t i = 0; i < derived.concurrency(); ++i) {
    this->m_slots.push_back(derived.m_slots[i].get());
  }
}

template <typename T>
template <typename U>
ana::lockstep::view<T> &
ana::lockstep::view<T>::operator=(const ana::lockstep::node<U> &derived) {
  this->m_model = derived.m_model.get();
  this->m_slots.clear();
  for (size_t i = 0; i < derived.concurrency(); ++i) {
    this->m_slots.push_back(derived.m_slots[i].get());
  }
  return *this;
}

template <typename T> T *ana::lockstep::view<T>::get_model() const {
  return this->m_model;
}

template <typename T> T *ana::lockstep::view<T>::get_slot(size_t i) const {
  return this->m_slots.at(i);
}

template <typename T> size_t ana::lockstep::view<T>::concurrency() const {
  return this->m_slots.size();
}

template <typename T>
template <typename Fn, typename... Args>
auto ana::lockstep::view<T>::get_model_value(Fn const &fn,
                                             Args const &...args) const
    -> std::invoke_result_t<Fn, T const &, Args const &...> {
  auto result = fn(static_cast<const T &>(*this->get_model()), args...);
  // result at each slot must match the model's
  for (size_t i = 0; i < concurrency(); ++i) {
    assert(result == fn(static_cast<const T &>(*this->get_slot(i)), args...));
  }
  return result;
}

#include <cassert>
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

namespace ana {

namespace dataset {

template <typename T> class input;

template <typename T> class reader;

/**
 * @brief Range of a dataset to process by one thread slot.
 */
struct range {
  range(size_t slot, unsigned long long begin, unsigned long long end);
  ~range() = default;

  range operator+(const range &next);
  range &operator+=(const range &next);

  unsigned long long entries() const;

  /**
   * @brief Thread index that the processed range belongs to.
   */
  size_t slot;
  /**
   * @brief The first entry in the range
   */
  unsigned long long begin;
  /**
   * @brief The last entry+1 in the range
   */
  unsigned long long end;
};

/**
 * @brief Partition of a dataset.
 * @details A partition contains one or more ranges of the datset in sequential
 * order of their entries. They can dynamically truncated and merged according
 * to the maximum limit of entries of the dataset and multithreading
 * configuration as specified by the analyzer.
 */
struct partition {
  static std::vector<std::vector<dataset::range>>
  group_parts(const std::vector<range> &parts, size_t n);
  static range sum_parts(const std::vector<range> &parts);

  partition() : fixed(false) {}
  ~partition() = default;

  /**
   * @brief Add a range to the partition.
   * @details The added range must satisfy that the `end` of the previous range
   * (if it exists) is equal to the incoming `begin`. Otherwise, the partition
   * is considered to be in an invalid state, and `truncate()` and `merge()`
   * operations respectively will fail assertions in place.
   */
  void add_part(size_t islot, unsigned long long begin, unsigned long long end);

  range get_part(size_t irange) const;
  range total() const;

  size_t size() const;

  void truncate(long long max_entries = -1);

  void merge(size_t max_parts);

  bool fixed;
  std::vector<range> parts;
};

} // namespace dataset

template <typename T>
using read_dataset_t = typename decltype(std::declval<T const &>().read_dataset(
    std::declval<const ana::dataset::range &>()))::element_type;

template <typename T, typename Val>
using read_column_t =
    typename decltype(std::declval<T const &>().template read_column<Val>(
        std::declval<dataset::range const &>(),
        std::declval<std::string const &>()))::element_type;

template <typename T> struct is_unique_ptr : std::false_type {};
template <typename T>
struct is_unique_ptr<std::unique_ptr<T>> : std::true_type {};
template <typename T>
static constexpr bool is_unique_ptr_v = is_unique_ptr<T>::value;

} // namespace ana

#include <functional>
#include <memory>
#include <type_traits>

namespace ana {

/**
 * @class operation
 * @brief abstract base class with initialization, execution, finalization steps
 */
class operation {

public:
  operation() = default;
  virtual ~operation() = default;

  virtual void initialize(const dataset::range &part) = 0;
  virtual void execute(const dataset::range &part,
                       unsigned long long entry) = 0;
  virtual void finalize(const dataset::range &part) = 0;
};

} // namespace ana

namespace ana {

template <typename T> struct is_callable {
  using yes = bool;
  using no = int;
  template <typename C> static yes check_callable(decltype(&C::operator()));
  template <typename C> static no check_callable(...);
  enum { value = sizeof(check_callable<T>(0)) == sizeof(yes) };
};

/**
 * @brief Determine whether a type is callable, i.e. has a valid `operator()`.
 */
template <typename T> constexpr bool is_callable_v = is_callable<T>::value;

class column;

template <typename T> constexpr bool is_column_v = std::is_base_of_v<column, T>;

class column : public operation {

public:
  template <typename T> class computation;

  template <typename T> class reader;

  template <typename T> class constant;

  template <typename T> class calculation;

  template <typename T> class definition;

  template <typename T> class equation;

  template <typename T> class representation;

  template <typename T> class evaluator;

public:
  column() = default;
  virtual ~column() = default;

public:
  template <typename T>
  static constexpr std::true_type
  check_reader(typename column::reader<T> const &);
  static constexpr std::false_type check_reader(...);

  template <typename T>
  static constexpr std::true_type
  check_constant(typename column::constant<T> const &);
  static constexpr std::false_type check_constant(...);

  template <typename T>
  static constexpr std::true_type
  check_definition(typename column::definition<T> const &);
  static constexpr std::false_type check_definition(...);

  template <typename T>
  static constexpr std::true_type
  check_equation(typename column::equation<T> const &);
  static constexpr std::false_type check_equation(...);

  template <typename T>
  static constexpr std::true_type
  check_representation(typename column::representation<T> const &);
  static constexpr std::false_type check_representation(...);

  template <typename T> struct is_evaluator : std::false_type {};
  template <typename T>
  struct is_evaluator<column::evaluator<T>> : std::true_type {};

  template <typename T, typename = void> struct evaluator_traits;
  template <typename T>
  struct evaluator_traits<T, typename std::enable_if_t<ana::is_column_v<T>>> {
    using evaluator_type = typename ana::column::template evaluator<T>;
  };

  template <typename F> struct equation_traits {
    using equation_type = typename equation_traits<decltype(std::function{
        std::declval<F>()})>::equation_type;
  };

  template <typename Ret, typename... Args>
  struct equation_traits<std::function<Ret(Args...)>> {
    using equation_type =
        typename column::equation<std::decay_t<Ret>(std::decay_t<Args>...)>;
  };

  template <typename F>
  using equation_t = typename equation_traits<F>::equation_type;

  template <typename F>
  struct evaluator_traits<F, typename std::enable_if_t<!ana::is_column_v<F>>> {
    using evaluator_type = typename ana::column::template evaluator<
        ana::column::template equation_t<F>>;
  };

  template <typename T>
  static constexpr bool is_reader_v =
      decltype(check_reader(std::declval<std::decay_t<T> const &>()))::value;

  template <typename T>
  static constexpr bool is_constant_v =
      decltype(check_constant(std::declval<std::decay_t<T> const &>()))::value;

  template <typename T>
  static constexpr bool is_definition_v = decltype(check_definition(
      std::declval<std::decay_t<T> const &>()))::value;

  template <typename T>
  static constexpr bool is_equation_v =
      decltype(check_equation(std::declval<std::decay_t<T> const &>()))::value;

  template <typename T>
  static constexpr bool is_representation_v = decltype(check_representation(
      std::declval<std::decay_t<T> const &>()))::value;

  template <typename T>
  static constexpr bool is_evaluator_v = is_evaluator<T>::value;

  template <typename T>
  using evaluator_t = typename evaluator_traits<T>::evaluator_type;

  template <typename T> using evaluated_t = typename T::evaluated_type;
};

//---------------------------------------------------
// cell can actually report on the concrete data type
//---------------------------------------------------
template <typename T> class cell {

public:
  using value_type = T;

public:
  template <typename U> class conversion_of;

  template <typename U> class interface_of;

public:
  cell() = default;
  virtual ~cell() = default;

  virtual const T &value() const = 0;
  virtual const T *field() const;
};

//------------------------------------
// conversion between compatible types
//------------------------------------
template <typename To>
template <typename From>
class cell<To>::conversion_of : public cell<To> {

public:
  conversion_of(const cell<From> &from);
  virtual ~conversion_of() = default;

public:
  virtual const To &value() const override;

private:
  const cell<From> *m_from;
  mutable To m_conversion_of;
};

//------------------------------------------
// interface between inherited -> base type
//------------------------------------------
template <typename To>
template <typename From>
class cell<To>::interface_of : public cell<To> {

public:
  interface_of(const cell<From> &from);
  virtual ~interface_of() = default;

public:
  virtual const To &value() const override;

private:
  const cell<From> *m_impl;
};

template <typename T> class term : public column, public cell<T> {

public:
  using value_type = typename cell<T>::value_type;

public:
  term() = default;
  virtual ~term() = default;

  virtual void initialize(const dataset::range &part) override;
  virtual void execute(const dataset::range &part,
                       unsigned long long entry) override;
  virtual void finalize(const dataset::range &part) override;
};

// costly to move around
template <typename T> class variable {

public:
  variable() = default;
  template <typename U> variable(const cell<U> &val);
  virtual ~variable() = default;

  variable(variable &&) = default;
  variable &operator=(variable &&) = default;

  const T &value() const;
  const T *field() const;

protected:
  std::unique_ptr<const cell<T>> m_val;
};

// easy to move around
template <typename T> class observable {

public:
  observable(const variable<T> &obs);
  virtual ~observable() = default;

  const T &value() const;
  const T *field() const;

  const T &operator*() const;
  const T *operator->() const;

protected:
  const variable<T> *m_var;
};

template <typename To, typename From>
std::unique_ptr<cell<To>> cell_as(const cell<From> &from);

template <typename T>
using cell_value_t = std::decay_t<decltype(std::declval<T>().value())>;

} // namespace ana

template <typename T>
void ana::term<T>::initialize(const ana::dataset::range &) {}

template <typename T>
void ana::term<T>::execute(const ana::dataset::range &, unsigned long long) {}

template <typename T>
void ana::term<T>::finalize(const ana::dataset::range &) {}

template <typename T> const T *ana::cell<T>::field() const {
  return &this->value();
}

template <typename To>
template <typename From>
ana::cell<To>::conversion_of<From>::conversion_of(const cell<From> &from)
    : m_from(&from) {}

template <typename To>
template <typename From>
const To &ana::cell<To>::conversion_of<From>::value() const {
  m_conversion_of = m_from->value();
  return m_conversion_of;
}

template <typename Base>
template <typename Impl>
ana::cell<Base>::interface_of<Impl>::interface_of(const cell<Impl> &from)
    : m_impl(&from) {}

template <typename Base>
template <typename Impl>
const Base &ana::cell<Base>::interface_of<Impl>::value() const {
  return m_impl->value();
}

template <typename To, typename From>
std::unique_ptr<ana::cell<To>> ana::cell_as(const cell<From> &from) {
  if constexpr (std::is_same_v<From, To> || std::is_base_of_v<From, To>) {
    return std::make_unique<
        typename ana::cell<To>::template interface_of<From>>(from);
  } else if constexpr (std::is_convertible_v<From, To>) {
    return std::make_unique<
        typename ana::cell<To>::template conversion_of<From>>(from);
  } else {
    static_assert(std::is_same_v<From, To> || std::is_base_of_v<From, To> ||
                      std::is_convertible_v<From, To>,
                  "incompatible value types");
  }
}

// --------
// variable
// --------

template <typename T>
template <typename U>
ana::variable<T>::variable(const cell<U> &val) : m_val(cell_as<T>(val)) {}

template <typename T> const T &ana::variable<T>::value() const {
  return m_val->value();
}

template <typename T> const T *ana::variable<T>::field() const {
  return m_val->field();
}

template <typename T>
ana::observable<T>::observable(const variable<T> &var) : m_var(&var) {}

template <typename T> const T &ana::observable<T>::operator*() const {
  return m_var->value();
}

template <typename T> const T *ana::observable<T>::operator->() const {
  return m_var->field();
}

template <typename T> const T &ana::observable<T>::value() const {
  return m_var->value();
}

template <typename T> const T *ana::observable<T>::field() const {
  return m_var->field();
}

namespace ana {

namespace dataset {

template <typename T> class reader {

private:
  reader() = default;
  ~reader() = default;
  friend T;

public:
  // read a column of a data type with given name
  template <typename Val>
  decltype(auto) read_column(const range &part, const std::string &name) const;

  void start_part(const range &part);
  void read_entry(const range &part, unsigned long long entry);
  void finish_part(const range &part);
};

} // namespace dataset

} // namespace ana

template <typename T>
template <typename Val>
decltype(auto)
ana::dataset::reader<T>::read_column(const range &part,
                                     const std::string &name) const {

  using result_type =
      decltype(static_cast<const T *>(this)->template read<Val>(part, name));
  static_assert(is_unique_ptr_v<result_type>,
                "must be a std::unique_ptr of ana::column::reader<T>");

  return static_cast<const T *>(this)->template read<Val>(part, name);
}

template <typename T>
void ana::dataset::reader<T>::start_part(const ana::dataset::range &part) {
  static_cast<T *>(this)->start(part);
}

template <typename T>
void ana::dataset::reader<T>::read_entry(const ana::dataset::range &part,
                                         unsigned long long entry) {
  static_cast<T *>(this)->next(part, entry);
}

template <typename T>
void ana::dataset::reader<T>::finish_part(const ana::dataset::range &part) {
  static_cast<T *>(this)->finish(part);
}

inline ana::dataset::range::range(size_t slot, unsigned long long begin,
                                  unsigned long long end)
    : slot(slot), begin(begin), end(end) {}

inline unsigned long long ana::dataset::range::entries() const {
  assert(this->end > this->begin);
  return end - begin;
}

inline ana::dataset::range ana::dataset::range::operator+(const range &next) {
  assert(this->end == next.begin);
  return range(this->slot, this->begin, next.end);
}

inline ana::dataset::range &ana::dataset::range::operator+=(const range &next) {
  assert(this->end == next.begin);
  this->end = next.end;
  return *this;
}

inline std::vector<std::vector<ana::dataset::range>>
ana::dataset::partition::group_parts(const std::vector<range> &parts,
                                     size_t n) {
  std::vector<std::vector<range>> grouped_parts;
  size_t length = parts.size() / n;
  size_t remain = parts.size() % n;
  size_t begin = 0;
  size_t end = 0;
  for (size_t i = 0; i < std::min(n, parts.size()); ++i) {
    end += (remain > 0) ? (length + !!(remain--)) : length;
    grouped_parts.push_back(
        std::vector<range>(parts.begin() + begin, parts.begin() + end));
    begin = end;
  }
  return grouped_parts;
}

inline ana::dataset::range
ana::dataset::partition::sum_parts(const std::vector<range> &parts) {
  return std::accumulate(std::next(parts.begin()), parts.end(), parts.front());
}

inline void ana::dataset::partition::add_part(size_t islot,
                                              unsigned long long begin,
                                              unsigned long long end) {
  this->parts.push_back(range(islot, begin, end));
}

inline ana::dataset::range
ana::dataset::partition::get_part(size_t islot) const {
  return this->parts[islot];
}

inline ana::dataset::range ana::dataset::partition::total() const {
  return sum_parts(this->parts);
}

inline size_t ana::dataset::partition::size() const {
  return this->parts.size();
}

inline void ana::dataset::partition::merge(size_t max_parts) {
  if (fixed)
    return;
  auto groups = group_parts(this->parts, max_parts);
  this->parts.clear();
  for (const auto &group : groups) {
    this->parts.push_back(sum_parts(group));
  }
}

inline void ana::dataset::partition::truncate(long long max_entries) {
  if (fixed)
    return;
  if (max_entries < 0)
    return;
  // remember the full parts
  auto full_parts = this->parts;
  // clear the parts to be added anew
  this->parts.clear();
  for (const auto &part : full_parts) {
    auto part_end = max_entries >= 0
                        ? std::min(part.begin + max_entries, part.end)
                        : part.end;
    this->parts.push_back(range(part.slot, part.begin, part_end));
    max_entries -= part_end;
    if (!max_entries)
      break;
  }
}

namespace ana {
namespace dataset {

/**
 * @brief
 * @tparam T Input dataset type (CRTP: see above)
 * @details This class uses the [Curiously Recurring Template Parameter
 * (CRPT)](https://en.cppreference.com/w/cpp/language/crtp). A proper
 * implementation should be written as follows:
 * ```cpp
 * #include "ana/analogical.h"
 * class DataFormat : public ana::dataset::dataset<DataFormat> {};
 * ```
 */
template <typename T> class input {

private:
  input() = default;
  ~input() = default;
  friend T;

public:
  partition allocate_partition();
  double normalize_scale();
  decltype(auto) read_dataset(const range &part) const;

  /**
   * @brief Allocate partition for multithreading.
   * @details This step **must** be implemented by the analyzer.
   * It is performed to scan the dataset and determine any partitioning of the
   * dataset as specfieid by the analyzer. Each range in the partition as
   * allocated by this function will be processed concurrently as allowed by
   * `ana::multithread::enable()`, provided that the implemented input reader
   * can access each dataset range from the dataset in a thread-safe manner.
   * This method is non-\a const to allow altering the logical state of the
   * dataset input itself, e.g. check file paths and filter out invalid/missing
   * files.
   */
  partition allocate();

  /**
   * @brief Normalize the dataset.
   * @details This step can be optionally done to determine a global
   * normalization factor applied to all selection in the analysis. By default,
   * normalization is not applied, i.e. is equal to 1. This method is non-\a
   * const to allow altering the logical state of the dataset input itself, e.g.
   * check file paths and filter out invalid/missing files.
   */
  double normalize();

  /**
   * @details Open the dataset reader used to iterate over entries of the
   * dataset.
   * @return The input reader.
   * @details **Important**: the return type is required to be a
   * `std::unique_ptr` of a valid `dataset::reader` implementation as required
   * for use in a `ana::concurrent` container. This method is \a const as it is
   * to be performed N times for each thread, and each subsequent call should
   * not alter the logical state of this class.
   */
  decltype(auto) read(const range &part) const;

  void start_dataset();
  void finish_dataset();

  void start();
  void finish();
};

} // namespace dataset

} // namespace ana

template <typename T>
ana::dataset::partition ana::dataset::input<T>::allocate_partition() {
  return this->allocate();
}

template <typename T>
ana::dataset::partition ana::dataset::input<T>::allocate() {
  return static_cast<T *>(this)->allocate();
}

template <typename T> double ana::dataset::input<T>::normalize_scale() {
  return static_cast<T *>(this)->normalize();
}

template <typename T> inline double ana::dataset::input<T>::normalize() {
  return 1.0;
}

template <typename T> void ana::dataset::input<T>::start_dataset() {
  static_cast<T *>(this)->start();
}

template <typename T> void ana::dataset::input<T>::finish_dataset() {
  static_cast<T *>(this)->finish();
}

template <typename T> void ana::dataset::input<T>::start() {
  // nothing to do (yet)
}

template <typename T> void ana::dataset::input<T>::finish() {
  // nothing to do (yet)
}

template <typename T>
decltype(auto)
ana::dataset::input<T>::read_dataset(const ana::dataset::range &part) const {

  using result_type = decltype(static_cast<const T *>(this)->read(part));
  static_assert(is_unique_ptr_v<result_type>,
                "not a std::unique_ptr of ana::dataset::reader<T>");

  using reader_type = typename result_type::element_type;
  static_assert(std::is_base_of_v<dataset::reader<reader_type>, reader_type>,
                "not an implementation of ana::dataset::reader<T>");

  return this->read(part);
}

template <typename T>
decltype(auto)
ana::dataset::input<T>::read(const ana::dataset::range &part) const {
  return static_cast<const T *>(this)->read(part);
}

#include <memory>
#include <tuple>

namespace ana {

/**
 * @brief Calculate a column value for each dataset entry.
 * @tparam Val Column value type.
 * @details A calculation is performed once per-entry (if needed) and its value
 * is stored for multiple accesses by downstream operations within the entry.
 * The type `Val` must be *CopyConstructible* and *CopyAssignable*.
 */
template <typename Val> class column::calculation : public term<Val> {

public:
  calculation();
  virtual ~calculation() = default;

protected:
  template <typename... Args> calculation(Args &&...args);

public:
  virtual const Val &value() const final override;

  virtual Val calculate() const = 0;

  virtual void initialize(const dataset::range &part) override;
  virtual void execute(const dataset::range &part,
                       unsigned long long entry) override;
  virtual void finalize(const dataset::range &part) override;

protected:
  void update() const;
  void reset() const;

protected:
  mutable Val m_value;
  mutable bool m_updated;
};

} // namespace ana

template <typename Val>
ana::column::calculation<Val>::calculation()
    : m_value(Val{}), m_updated(false) {}

template <typename Val>
template <typename... Args>
ana::column::calculation<Val>::calculation(Args &&...args)
    : m_value(std::forward<Args>(args)...) {}

template <typename Val>
const Val &ana::column::calculation<Val>::value() const {
  if (!m_updated)
    this->update();
  return m_value;
}

template <typename Val> void ana::column::calculation<Val>::update() const {
  m_value = this->calculate();
  m_updated = true;
}

template <typename Val> void ana::column::calculation<Val>::reset() const {
  m_updated = false;
}

template <typename Val>
void ana::column::calculation<Val>::initialize(const ana::dataset::range &) {}

template <typename Val>
void ana::column::calculation<Val>::execute(const ana::dataset::range &,
                                            unsigned long long) {
  this->reset();
}

template <typename Val>
void ana::column::calculation<Val>::finalize(const ana::dataset::range &) {}

namespace ana {

template <typename Ret, typename... Vals>
class column::definition<Ret(Vals...)> : public column::calculation<Ret> {

public:
  using vartuple_type = std::tuple<variable<Vals>...>;
  using obstuple_type = std::tuple<observable<Vals>...>;

public:
  definition() = default;
  virtual ~definition() = default;

protected:
  template <typename... Args> definition(Args &&...args);

public:
  virtual Ret calculate() const final override;
  virtual Ret evaluate(observable<Vals>... args) const = 0;

  template <typename... Ins> void set_arguments(const cell<Ins> &...args);

protected:
  vartuple_type m_arguments;
};

} // namespace ana

template <typename Ret, typename... Vals>
template <typename... Args>
ana::column::definition<Ret(Vals...)>::definition(Args &&...args)
    : calculation<Ret>(std::forward<Args>(args)...) {}

template <typename Ret, typename... Vals>
template <typename... Ins>
void ana::column::definition<Ret(Vals...)>::set_arguments(
    cell<Ins> const &...args) {
  static_assert(sizeof...(Vals) == sizeof...(Ins));
  m_arguments = std::make_tuple(std::invoke(
      [](const cell<Ins> &args) -> variable<Vals> {
        return variable<Vals>(args);
      },
      args)...);
}

template <typename Ret, typename... Vals>
Ret ana::column::definition<Ret(Vals...)>::calculate() const {
  return std::apply(
      [this](const variable<Vals> &...args) { return this->evaluate(args...); },
      m_arguments);
}

namespace ana {

/**
 * @brief Abstract base class to read values of existing columns in an input
 * dataset.
 * @tparam T column data type.
 */
template <typename T> class column::reader : public term<T> {
public:
  reader();
  virtual ~reader() = default;

  /**
   * @brief Read the value of the column at current entry.
   * @return Column value
   */
  virtual T const &read(const dataset::range &part,
                        unsigned long long entry) const = 0;

  /**
   * @brief Get the value of the column at current entry.
   * @return Column value
   */
  virtual T const &value() const override;

  virtual void execute(const dataset::range &part,
                       unsigned long long entry) final override;

protected:
  mutable T const *m_addr;
  mutable bool m_updated;

  const dataset::range *m_part;
  unsigned long long m_current;
};

} // namespace ana

template <typename T>
ana::column::reader<T>::reader()
    : m_addr(nullptr), m_updated(false), m_part(nullptr), m_current(0) {}

template <typename T> T const &ana::column::reader<T>::value() const {
  if (!this->m_updated) {
    m_addr = &(this->read(*this->m_part, m_current));
    m_updated = true;
  }
  return *m_addr;
}

template <typename T>
void ana::column::reader<T>::execute(const ana::dataset::range &part,
                                     unsigned long long entry) {
  m_current = entry;
  m_part = &part;
  m_updated = false;
}

#include <functional>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ana {

template <typename T> class term;

template <typename T> class cell;

template <typename T> class variable;

template <typename T> class observable;

class selection;

class aggregation : public operation {

public:
  class experiment;

  template <typename T> class output;

  template <typename T> class logic;

  template <typename T> class booker;

  template <typename T> class bookkeeper;

  template <typename T> class summary;

public:
  aggregation();
  virtual ~aggregation() = default;

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
  static constexpr std::true_type
  check_implemented(const aggregation::output<T> &);
  static constexpr std::false_type check_implemented(...);

  template <typename Out, typename... Vals>
  static constexpr std::true_type
  check_fillable(const typename aggregation::logic<Out(Vals...)> &);
  static constexpr std::false_type check_fillable(...);

  template <typename T> struct is_booker : std::false_type {};
  template <typename T>
  struct is_booker<aggregation::booker<T>> : std::true_type {};

  template <typename T> struct is_bookkeeper : std::false_type {};
  template <typename T>
  struct is_bookkeeper<aggregation::bookkeeper<T>> : std::true_type {};

  template <typename T>
  static constexpr bool has_output_v =
      decltype(check_implemented(std::declval<T>()))::value;

  template <typename T>
  static constexpr bool is_fillable_v =
      decltype(check_fillable(std::declval<T>()))::value;

  template <typename T> static constexpr bool is_booker_v = is_booker<T>::value;
  template <typename T>
  static constexpr bool is_bookkeeper_v = is_bookkeeper<T>::value;

  template <typename Bkr> using booked_t = typename Bkr::aggregation_type;
};

} // namespace ana

namespace ana {

class selection;

template <typename Cnt> class aggregation::bookkeeper {

public:
  using aggregation_type = Cnt;

public:
  bookkeeper() = default;
  ~bookkeeper() = default;

  bookkeeper(const bookkeeper &) = default;
  bookkeeper &operator=(const bookkeeper &) = default;

  void bookkeep(Cnt &cnt, const selection &sel);

  std::set<std::string> list_selection_paths() const;
  Cnt *get_aggregation(const std::string &path) const;

protected:
  std::set<std::string> m_booked_selection_paths;
  std::unordered_map<std::string, Cnt *> m_booked_aggregation_map;
};

} // namespace ana

#include <memory>
#include <string>
#include <vector>

namespace ana {

class selection;

template <typename T>
constexpr bool is_selection_v = std::is_base_of_v<ana::selection, T>;

// class selection : public operation {
class selection : public column::calculation<double> {

public:
  class cutflow;

  class cut;
  class weight;

  template <typename T> class applicator;

public:
  static std::string concatenate_names(std::vector<std::string> const &names,
                                       std::string delimiter = "/");

public:
  selection(const selection *presel, bool ch, const std::string &name);
  virtual ~selection() = default;

public:
  std::string get_name() const noexcept;
  std::string get_path() const;
  std::string get_full_path() const;

  bool is_initial() const noexcept;
  bool is_channel() const noexcept;
  const selection *get_previous() const noexcept;

  template <typename T> void set_decision(std::unique_ptr<T> dec);

  virtual bool passed_cut() const = 0;
  virtual double get_weight() const = 0;

public:
  virtual void initialize(const dataset::range &part) override;
  virtual void execute(const dataset::range &part,
                       unsigned long long entry) override;
  virtual void finalize(const dataset::range &part) override;

private:
  const selection *const m_preselection;
  const bool m_channel;
  const std::string m_name;

  std::unique_ptr<column> m_decision;
  ana::variable<double> m_variable;

public:
  template <typename T> struct is_applicator : std::false_type {};
  template <typename T>
  struct is_applicator<selection::applicator<T>> : std::true_type {};
  template <typename T>
  static constexpr bool is_applicator_v = is_applicator<T>::value;

  template <typename F>
  using custom_applicator_t = typename selection::template applicator<
      ana::column::template equation_t<F>>;
  using trivial_applicator_type = typename selection::template applicator<
      ana::column::equation<double(double)>>;
};

} // namespace ana

#include <functional>

namespace ana {

template <typename Ret, typename... Vals>
class column::equation<Ret(Vals...)> : public column::definition<Ret(Vals...)> {

public:
  using vartuple_type = typename definition<Ret(Vals...)>::vartuple_type;
  using function_type =
      std::function<std::decay_t<Ret>(std::decay_t<Vals> const &...)>;

public:
  template <typename F> equation(F callable);
  template <typename F, typename... Args> equation(F callable, Args &&...args);
  virtual ~equation() = default;

public:
  virtual Ret evaluate(observable<Vals>... args) const override;

protected:
  vartuple_type m_arguments;
  function_type m_evaluate;
};

template <typename F>
auto make_equation(F expression)
    -> std::unique_ptr<column::template equation_t<F>>;

} // namespace ana

template <typename Ret, typename... Vals>
template <typename F>
ana::column::equation<Ret(Vals...)>::equation(F callable)
    : m_evaluate(callable) {}

template <typename Ret, typename... Vals>
template <typename F, typename... Args>
ana::column::equation<Ret(Vals...)>::equation(F callable, Args &&...args)
    : m_evaluate(callable),
      definition<Ret(Vals...)>(std::forward<Args>(args)...) {}

template <typename Ret, typename... Vals>
Ret ana::column::equation<Ret(Vals...)>::evaluate(
    ana::observable<Vals>... args) const {
  return this->m_evaluate(args.value()...);
}

template <typename F>
auto ana::make_equation(F expression)
    -> std::unique_ptr<ana::column::template equation_t<F>> {
  return std::make_unique<ana::column::template equation_t<F>>(expression);
}

inline std::string
ana::selection::concatenate_names(std::vector<std::string> const &names,
                                  std::string delimiter) {
  std::string joined;
  for (auto const &name : names) {
    joined += name;
    joined += delimiter;
  }
  return joined;
}

inline ana::selection::selection(const selection *presel, bool ch,
                                 const std::string &name)
    : m_preselection(presel), m_channel(ch), m_name(name) {}

inline bool ana::selection::is_initial() const noexcept {
  return m_preselection ? false : true;
}

inline const ana::selection *ana::selection::get_previous() const noexcept {
  return m_preselection;
}

inline bool ana::selection::is_channel() const noexcept { return m_channel; }

inline std::string ana::selection::get_name() const noexcept { return m_name; }

inline std::string ana::selection::get_path() const {
  std::vector<std::string> channels;
  const selection *presel = this->get_previous();
  while (presel) {
    if (presel->is_channel())
      channels.push_back(presel->get_name());
    presel = presel->get_previous();
  }
  std::reverse(channels.begin(), channels.end());
  return concatenate_names(channels, "/") + this->get_name();
}

inline std::string ana::selection::get_full_path() const {
  std::vector<std::string> presels;
  const selection *presel = this->get_previous();
  while (presel) {
    presels.push_back(presel->get_name());
    presel = presel->get_previous();
  }
  std::reverse(presels.begin(), presels.end());
  return concatenate_names(presels, "/") + this->get_name();
}

inline void ana::selection::initialize(const ana::dataset::range &part) {
  m_decision->initialize(part);
}

inline void ana::selection::execute(const ana::dataset::range &part,
                                    unsigned long long entry) {
  ana::column::calculation<double>::execute(part, entry);
  m_decision->execute(part, entry);
}

inline void ana::selection::finalize(const ana::dataset::range &part) {
  m_decision->finalize(part);
}

template <typename T>
void ana::selection::set_decision(std::unique_ptr<T> decision) {
  // link value to variable<double>
  m_variable = variable<double>((term<cell_value_t<T>> &)(*decision));
  // keep decision as term
  m_decision = std::move(decision);
}

template <typename Cnt>
void ana::aggregation::bookkeeper<Cnt>::bookkeep(Cnt &cnt,
                                                 const selection &sel) {
  // check if booking makes sense
  if (m_booked_aggregation_map.find(sel.get_path()) !=
      m_booked_aggregation_map.end()) {
    throw std::logic_error("aggregation already booked at selection");
  }
  m_booked_selection_paths.insert(sel.get_path());
  m_booked_aggregation_map.insert(std::make_pair(sel.get_path(), &cnt));
}

template <typename T>
T *ana::aggregation::bookkeeper<T>::get_aggregation(
    const std::string &selection_path) const {
  if (m_booked_aggregation_map.find(selection_path) ==
      m_booked_aggregation_map.end()) {
    throw std::out_of_range("aggregation not booked at selection path");
  }
  return m_booked_aggregation_map.at(selection_path);
}

inline ana::aggregation::aggregation()
    : m_raw(false), m_scale(1.0), m_selection(nullptr) {}

inline void ana::aggregation::set_selection(const selection &selection) {
  m_selection = &selection;
}

inline const ana::selection *ana::aggregation::get_selection() const {
  return m_selection;
}

inline void ana::aggregation::apply_scale(double scale) { m_scale *= scale; }

inline void ana::aggregation::use_weight(bool use) { m_raw = !use; }

inline void ana::aggregation::initialize(const ana::dataset::range &) {
  if (!m_selection)
    throw std::runtime_error("no booked selection");
}

inline void ana::aggregation::execute(const ana::dataset::range &,
                                      unsigned long long) {
  if (m_selection->passed_cut())
    this->count(m_raw ? 1.0 : m_scale * m_selection->get_weight());
}

namespace ana {

/**
 * @brief Minimal aggregation with an output result.
 * @details This ABC should be used for counting operations that do not require
 * any input columns, e.g. a cutflow of selections.
 */
template <typename T> class aggregation::output : public aggregation {

public:
  using result_type = T;

public:
  output();
  virtual ~output() = default;

  /**
   * @brief Create and return the result of the aggregation.
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
  using aggregation::count;

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

} // namespace ana

template <typename T> ana::aggregation::output<T>::output() : m_merged(false) {}

template <typename T> bool ana::aggregation::output<T>::is_merged() const {
  return m_merged;
}

template <typename T>
void ana::aggregation::output<T>::set_merged(bool merged) {
  m_merged = merged;
}

template <typename T>
void ana::aggregation::output<T>::finalize(const ana::dataset::range &) {
  m_result = this->result();
}

template <typename T> T const &ana::aggregation::output<T>::get_result() const {
  return m_result;
}

template <typename T>
void ana::aggregation::output<T>::merge_results(std::vector<T> results) {
  if (!results.size()) {
    throw std::logic_error("merging requires at least one result");
  }
  m_result = this->merge(results);
  this->set_merged(true);
}

namespace ana {
/**
 * @brief Counter output to be filled with columns using arbitrary logic.
 * @tparam T Output result type.
 * @tparam Obs... Input column data types.
 */
template <typename T, typename... Obs>
class aggregation::logic<T(Obs...)> : public aggregation::output<T> {

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
   * the number of `fill` calls made to its `lazy` operation, each with its the
   * set of input columns as provided then.
   */
  virtual void fill(ana::observable<Obs>... observables, double w) = 0;
  virtual void count(double w) final override;

  template <typename... Vals> void enter_columns(term<Vals> const &...cols);

protected:
  std::vector<vartup_type> m_fills;
};

} // namespace ana

template <typename T, typename... Obs>
template <typename... Vals>
void ana::aggregation::logic<T(Obs...)>::enter_columns(
    term<Vals> const &...cols) {
  static_assert(sizeof...(Obs) == sizeof...(Vals),
                "dimension mis-match between filled variables & columns.");
  m_fills.emplace_back(cols...);
}

template <typename T, typename... Obs>
void ana::aggregation::logic<T(Obs...)>::count(double w) {
  for (unsigned int ifill = 0; ifill < m_fills.size(); ++ifill) {
    std::apply(
        [this, w](const variable<Obs> &...obs) { this->fill(obs..., w); },
        m_fills[ifill]);
  }
}

/** @file */

#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

#include <iostream>
#include <memory>

#include <memory>
#include <vector>

#include <memory>
#include <string>
#include <vector>

namespace ana {

class selection::cutflow {

public:
  cutflow() = default;
  ~cutflow() = default;

public:
  template <typename Sel, typename F>
  auto filter(selection const *prev, const std::string &name,
              F expression) const
      -> std::unique_ptr<applicator<column::template equation_t<F>>>;

  template <typename Sel, typename F>
  auto channel(selection const *prev, const std::string &name,
               F expression) const
      -> std::unique_ptr<applicator<column::template equation_t<F>>>;

  template <typename Sel, typename... Cols>
  auto apply_selection(applicator<Sel> const &calc, Cols const &...columns)
      -> std::unique_ptr<selection>;

protected:
  void add_selection(selection &selection);

protected:
  std::vector<selection *> m_selections;
};

} // namespace ana

#include <memory>
#include <string>
#include <vector>

namespace ana {

template <typename T> class ana::selection::applicator {

public:
  template <typename Fn> applicator(Fn fn);
  ~applicator() = default;

  template <typename Sel>
  void set_selection(const selection *presel, bool ch, const std::string &name);

  template <typename... Vals>
  std::unique_ptr<selection>
  apply_selection(cell<Vals> const &...columns) const;

protected:
  std::function<std::unique_ptr<T>()> m_make_unique_equation;
  std::function<std::unique_ptr<selection>()> m_make_unique_selection;
};

} // namespace ana

template <typename T>
template <typename Fn>
ana::selection::applicator<T>::applicator(Fn fn)
    : m_make_unique_equation(std::bind(
          [](Fn fn) -> std::unique_ptr<T> { return std::make_unique<T>(fn); },
          fn)),
      m_make_unique_selection(
          []() -> std::unique_ptr<selection> { return nullptr; }) {}

template <typename T>
template <typename Sel>
void ana::selection::applicator<T>::set_selection(const selection *presel,
                                                  bool ch,
                                                  const std::string &name) {
  m_make_unique_selection = std::bind(
      [](const selection *presel, bool ch,
         const std::string &name) -> std::unique_ptr<selection> {
        return std::make_unique<Sel>(presel, ch, name);
      },
      presel, ch, name);
}

template <typename T>
template <typename... Vals>
std::unique_ptr<ana::selection> ana::selection::applicator<T>::apply_selection(
    cell<Vals> const &...columns) const {
  // make this selection
  auto eqn = this->m_make_unique_equation();
  eqn->set_arguments(columns...);

  auto sel = this->m_make_unique_selection();

  // set equation arguments

  // set selection decision
  // sel->set_decision(
  //     std::static_pointer_cast<term<cell_value_t<T>>>(m_equation));
  // auto eqn = std::unique_ptr<term<cell_value_t<T>>>(m_equation.release());
  sel->set_decision(std::move(eqn));

  return sel;
}

namespace ana {

class selection::cut : public selection {

public:
  cut(const selection *presel, bool ch, const std::string &name);
  virtual ~cut() = default;

public:
  virtual double calculate() const override;
  virtual bool passed_cut() const override;
  virtual double get_weight() const override;
};

} // namespace ana

inline ana::selection::cut::cut(const selection *presel, bool ch,
                                const std::string &name)
    : selection(presel, ch, name) {}

inline double ana::selection::cut::calculate() const {
  return this->m_preselection
             ? this->m_preselection->passed_cut() && m_variable.value()
             : m_variable.value();
}

inline bool ana::selection::cut::passed_cut() const { return this->value(); }

inline double ana::selection::cut::get_weight() const {
  return this->m_preselection ? this->m_preselection->get_weight() : 1.0;
}

namespace ana {

class selection::weight : public selection {

public:
  class a_times_b;

public:
  weight(const selection *presel, bool ch, const std::string &name);
  virtual ~weight() = default;

public:
  virtual double calculate() const override;
  virtual bool passed_cut() const override;
  virtual double get_weight() const override;
};

} // namespace ana

inline ana::selection::weight::weight(const selection *presel, bool ch,
                                      const std::string &name)
    : selection(presel, ch, name) {}

inline double ana::selection::weight::calculate() const {
  return this->m_preselection
             ? this->m_preselection->get_weight() * m_variable.value()
             : m_variable.value();
}

inline bool ana::selection::weight::passed_cut() const {
  return this->m_preselection ? this->m_preselection->passed_cut() : true;
}

inline double ana::selection::weight::get_weight() const {
  return this->value();
}

template <typename Sel, typename F>
auto ana::selection::cutflow::filter(selection const *prev,
                                     const std::string &name,
                                     F expression) const
    -> std::unique_ptr<applicator<column::template equation_t<F>>> {
  auto calc = std::make_unique<applicator<column::template equation_t<F>>>(
      std::function{expression});
  calc->template set_selection<Sel>(prev, false, name);
  return calc;
}

template <typename Sel, typename F>
auto ana::selection::cutflow::channel(selection const *prev,
                                      const std::string &name,
                                      F expression) const
    -> std::unique_ptr<applicator<column::template equation_t<F>>> {
  auto calc = std::make_unique<applicator<column::template equation_t<F>>>(
      std::function{expression});
  calc->template set_selection<Sel>(prev, true, name);
  return calc;
}

template <typename Sel, typename... Cols>
auto ana::selection::cutflow::apply_selection(applicator<Sel> const &calc,
                                              Cols const &...columns)
    -> std::unique_ptr<selection> {
  auto sel = calc.apply_selection(columns...);
  this->add_selection(*sel);
  return sel;
}

inline void ana::selection::cutflow::add_selection(ana::selection &sel) {
  m_selections.push_back(&sel);
}

#include <functional>
#include <utility>
#include <vector>

namespace ana {

template <typename T> class aggregation::booker {

public:
  using aggregation_type = T;

  using bkpr_and_cnts_type =
      typename std::pair<std::unique_ptr<bookkeeper<T>>,
                         std::vector<std::unique_ptr<T>>>;

public:
  template <typename... Args> booker(Args... args);
  ~booker() = default;

  // copyable
  booker(const booker &) = default;
  booker &operator=(const booker &) = default;

  template <typename... Vals>
  auto book_fill(term<Vals> const &...cols) const -> std::unique_ptr<booker<T>>;

  auto select_aggregation(const selection &sel) const -> std::unique_ptr<T>;

  template <typename... Sels>
  auto select_aggregations(Sels const &...selections) const
      -> bkpr_and_cnts_type;

protected:
  std::unique_ptr<T> make_aggregation();
  template <typename... Vals> void fill_aggregation(term<Vals> const &...cols);

protected:
  std::function<std::unique_ptr<T>()> m_make_unique_aggregation;
  std::vector<std::function<void(T &)>> m_fill_columns;
};

} // namespace ana

template <typename T>
template <typename... Args>
ana::aggregation::booker<T>::booker(Args... args)
    : m_make_unique_aggregation(std::bind(
          [](Args... args) { return std::make_unique<T>(args...); }, args...)) {
}

template <typename T>
template <typename... Vals>
auto ana::aggregation::booker<T>::book_fill(term<Vals> const &...columns) const
    -> std::unique_ptr<booker<T>> {
  // use a fresh one with its current fills
  auto filled = std::make_unique<booker<T>>(*this);
  // add fills
  filled->fill_aggregation(columns...);
  // return new booker
  return filled;
}

template <typename T>
template <typename... Vals>
void ana::aggregation::booker<T>::fill_aggregation(
    term<Vals> const &...columns) {
  // use a snapshot of its current calls
  m_fill_columns.push_back(std::bind(
      [](T &cnt, term<Vals> const &...cols) { cnt.enter_columns(cols...); },
      std::placeholders::_1, std::ref(columns)...));
}

template <typename T>
auto ana::aggregation::booker<T>::select_aggregation(const selection &sel) const
    -> std::unique_ptr<T> {
  // call constructor
  auto cnt = m_make_unique_aggregation();
  // fill columns (if set)
  for (const auto &fill_aggregation : m_fill_columns) {
    fill_aggregation(*cnt);
  }
  // book cnt at the selection
  cnt->set_selection(sel);
  // return
  return cnt;
}

template <typename T>
template <typename... Sels>
auto ana::aggregation::booker<T>::select_aggregations(
    const Sels &...selections) const -> bkpr_and_cnts_type {

  // make bookkeeper remember selections and gather aggregations into vector.
  auto aggregation_bookkeeper = std::make_unique<bookkeeper<T>>();
  std::vector<std::unique_ptr<T>> booked_aggregations;
  (std::invoke(
       [this, bkpr = aggregation_bookkeeper.get(),
        &cntr_list = booked_aggregations](const selection &sel) {
         auto cntr = this->select_aggregation(sel);
         bkpr->bookkeep(*cntr, sel);
         cntr_list.emplace_back(std::move(cntr));
       },
       selections),
   ...);

  // return a new booker with the selections added
  return std::make_pair(std::move(aggregation_bookkeeper),
                        std::move(booked_aggregations));
}

namespace ana {

class aggregation::experiment : public selection::cutflow {

public:
  experiment(double scale);
  ~experiment() = default;

public:
  template <typename Cnt, typename... Args>
  std::unique_ptr<booker<Cnt>> book(Args &&...args);

  template <typename Cnt>
  auto select_aggregation(booker<Cnt> const &bkr, const selection &sel)
      -> std::unique_ptr<Cnt>;

  template <typename Cnt, typename... Sels>
  auto select_aggregations(booker<Cnt> const &bkr, Sels const &...sels)
      -> std::pair<std::unique_ptr<bookkeeper<Cnt>>,
                   std::vector<std::unique_ptr<Cnt>>>;

  void clear_aggregations();

protected:
  void add_aggregation(aggregation &cnt);

protected:
  std::vector<aggregation *> m_aggregations;
  const double m_norm;
};

} // namespace ana

inline ana::aggregation::experiment::experiment(double norm) : m_norm(norm) {}

inline void
ana::aggregation::experiment::add_aggregation(ana::aggregation &cnt) {
  m_aggregations.push_back(&cnt);
}

inline void ana::aggregation::experiment::clear_aggregations() {
  m_aggregations.clear();
}

template <typename Cnt, typename... Args>
std::unique_ptr<ana::aggregation::booker<Cnt>>
ana::aggregation::experiment::book(Args &&...args) {
  auto bkr = std::make_unique<booker<Cnt>>(std::forward<Args>(args)...);
  return bkr;
}

template <typename Cnt>
auto ana::aggregation::experiment::select_aggregation(booker<Cnt> const &bkr,
                                                      const selection &sel)
    -> std::unique_ptr<Cnt> {
  auto cnt = bkr.select_aggregation(sel);
  cnt->apply_scale(m_norm);
  this->add_aggregation(*cnt);
  return cnt;
}

template <typename Cnt, typename... Sels>
auto ana::aggregation::experiment::select_aggregations(booker<Cnt> const &bkr,
                                                       Sels const &...sels)
    -> std::pair<std::unique_ptr<bookkeeper<Cnt>>,
                 std::vector<std::unique_ptr<Cnt>>> {
  // get a booker that has all the selections added
  auto bkpr_and_cntrs = bkr.select_aggregations(sels...);

  for (auto const &cntr : bkpr_and_cntrs.second) {
    this->add_aggregation(*cntr);
  }

  return std::move(bkpr_and_cntrs);
}

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace ana {

/**
 * @brief Computation graph of columns.
 * @details `column::computation<Dataset_t>` issues `unique::ptr<Column_t>`
 * for all columns, or their evaluators, used in the analysis.
 * It keeps a raw pointer of each, only if their operation needs to be called
 * for each dataset entry (e.g. constant values are not stored).
 */
template <typename T> class column::computation {

public:
  computation(const dataset::range &part, dataset::reader<T> &reader);
  virtual ~computation() = default;

public:
  template <typename Val>
  auto read(const std::string &name) -> std::unique_ptr<read_column_t<T, Val>>;

  template <typename Val>
  auto constant(Val const &val) -> std::unique_ptr<column::constant<Val>>;

  template <typename Def, typename... Args>
  auto define(Args const &...vars) const
      -> std::unique_ptr<ana::column::template evaluator_t<Def>>;

  template <typename F>
  auto define(F expression) const
      -> std::unique_ptr<ana::column::template evaluator_t<F>>;

  template <typename Def, typename... Cols>
  auto evaluate_column(column::evaluator<Def> &calc, Cols const &...columns)
      -> std::unique_ptr<Def>;

protected:
  void add_column(column &column);

protected:
  const dataset::range m_part;
  dataset::reader<T> *m_reader;
  std::vector<column *> m_columns;
};

} // namespace ana

namespace ana {

//------------------------------------------------------------------------------
// constant: value set manually
//------------------------------------------------------------------------------
template <typename Val> class column::constant : public term<Val> {

public:
  constant(const Val &val);
  virtual ~constant() = default;

  const Val &value() const override;

protected:
  Val m_value;
};

} // namespace ana

template <typename Val>
ana::column::constant<Val>::constant(const Val &val) : m_value(val) {}

template <typename Val> const Val &ana::column::constant<Val>::value() const {
  return m_value;
}

#include <functional>
#include <memory>
#include <type_traits>

namespace ana {

template <typename T> class column::evaluator {

public:
  using evaluated_type = T;

public:
  template <typename... Args> evaluator(Args const &...args);
  ~evaluator() = default;

  template <typename... Vals>
  std::unique_ptr<T> evaluate_column(cell<Vals> const &...cols) const;

protected:
  std::function<std::unique_ptr<T>()> m_make_unique;
};

} // namespace ana

template <typename T>
template <typename... Args>
ana::column::evaluator<T>::evaluator(Args const &...args)
    : m_make_unique(std::bind(
          [](Args const &...args) { return std::make_unique<T>(args...); },
          args...)) {}

template <typename T>
template <typename... Vals>
std::unique_ptr<T>
ana::column::evaluator<T>::evaluate_column(cell<Vals> const &...columns) const {
  auto defn = m_make_unique();

  defn->set_arguments(columns...);

  return defn;
}

template <typename T>
ana::column::computation<T>::computation(const ana::dataset::range &part,
                                         dataset::reader<T> &reader)
    : m_part(part), m_reader(&reader) {}

template <typename T>
template <typename Val>
auto ana::column::computation<T>::read(const std::string &name)
    -> std::unique_ptr<read_column_t<T, Val>> {
  using read_column_type = decltype(m_reader->template read_column<Val>(
      std::declval<const dataset::range &>(),
      std::declval<const std::string &>()));
  static_assert(is_unique_ptr_v<read_column_type>,
                "dataset must open a std::unique_ptr of its column reader");
  auto rdr = m_reader->template read_column<Val>(m_part, name);
  this->add_column(*rdr);
  return rdr;
}

template <typename T>
template <typename Val>
auto ana::column::computation<T>::constant(Val const &val)
    -> std::unique_ptr<ana::column::constant<Val>> {
  return std::make_unique<typename column::constant<Val>>(val);
}

template <typename T>
template <typename Def, typename... Args>
auto ana::column::computation<T>::define(Args const &...args) const
    -> std::unique_ptr<ana::column::template evaluator_t<Def>> {
  return std::make_unique<evaluator<Def>>(args...);
}

template <typename T>
template <typename F>
auto ana::column::computation<T>::define(F expression) const
    -> std::unique_ptr<ana::column::template evaluator_t<F>> {
  return std::make_unique<evaluator<ana::column::template equation_t<F>>>(
      expression);
}

template <typename T>
template <typename Def, typename... Cols>
auto ana::column::computation<T>::evaluate_column(column::evaluator<Def> &calc,
                                                  Cols const &...columns)
    -> std::unique_ptr<Def> {
  auto defn = calc.evaluate_column(columns...);
  // only if the evaluated column is a definition
  if constexpr (column::template is_definition_v<Def>) {
    this->add_column(*defn);
  }
  return defn;
}

template <typename T>
void ana::column::computation<T>::add_column(column &column) {
  m_columns.push_back(&column);
}

namespace ana {

namespace dataset {

template <typename T>
class processor : public operation,
                  public column::computation<T>,
                  public aggregation::experiment {

public:
  processor(const dataset::range &part, dataset::reader<T> &reader,
            double scale);
  virtual ~processor() = default;

public:
  virtual void initialize(const dataset::range &part) override;
  virtual void execute(const dataset::range &part,
                       unsigned long long entry) override;
  virtual void finalize(const dataset::range &part) override;

  void process();
};

} // namespace dataset

} // namespace ana

template <typename T>
ana::dataset::processor<T>::processor(const ana::dataset::range &part,
                                      dataset::reader<T> &reader, double scale)
    : operation(), column::computation<T>(part, reader),
      aggregation::experiment(scale) {}

template <typename T>
void ana::dataset::processor<T>::initialize(const ana::dataset::range &part) {
  this->m_reader->start_part(this->m_part);

  for (auto const &col : this->m_columns) {
    col->initialize(part);
  }
  for (auto const &sel : this->m_selections) {
    sel->initialize(part);
  }
  for (auto const &cnt : this->m_aggregations) {
    cnt->initialize(part);
  }
}

template <typename T>
void ana::dataset::processor<T>::execute(const ana::dataset::range &part,
                                         unsigned long long entry) {
  this->m_reader->read_entry(part, entry);
  for (auto const &col : this->m_columns) {
    col->execute(part, entry);
  }
  for (auto const &sel : this->m_selections) {
    sel->execute(part, entry);
  }
  for (auto const &cnt : this->m_aggregations) {
    cnt->execute(part, entry);
  }
}

template <typename T>
void ana::dataset::processor<T>::finalize(const ana::dataset::range &part) {
  for (auto const &col : this->m_columns) {
    col->finalize(part);
  }
  for (auto const &sel : this->m_selections) {
    sel->finalize(part);
  }
  for (auto const &cnt : this->m_aggregations) {
    cnt->finalize(part);
  }
  this->m_reader->finish_part(this->m_part);
}

template <typename T> void ana::dataset::processor<T>::process() {
  // start processing
  this->initialize(this->m_part);

  // processing
  for (unsigned long long entry = this->m_part.begin; entry < this->m_part.end;
       ++entry) {
    this->execute(this->m_part, entry);
  }

  // finish processing
  this->finalize(this->m_part);
}

namespace ana {

template <typename T> class sample {

public:
  using dataset_reader_type = read_dataset_t<T>;
  using dataset_processor_type =
      typename dataset::processor<dataset_reader_type>;

public:
  sample();
  virtual ~sample() = default;

  sample(sample const &) = delete;
  sample &operator=(sample const &) = delete;

  sample(sample &&) = default;
  sample &operator=(sample &&) = default;

  void limit_entries(long long max_entries = -1);
  void scale_weights(double scale);
  unsigned long long get_processed_entries() const;
  double get_normalized_scale() const;

protected:
  template <typename... Args> void prepare(Args &&...args);
  void initialize();

protected:
  std::unique_ptr<T> m_dataset;
  bool m_initialized;

  long long m_max_entries;
  double m_scale;

  dataset::partition m_partition;
  lockstep::node<dataset_reader_type> m_readers;
  lockstep::node<dataset_processor_type> m_processors;
};

} // namespace ana

template <typename T>
ana::sample<T>::sample()
    : m_dataset(nullptr), m_initialized(false), m_max_entries(-1),
      m_scale(1.0) {}

template <typename T>
template <typename... Args>
void ana::sample<T>::prepare(Args &&...args) {
  if (m_dataset)
    throw std::logic_error("dataset already prepared");
  m_dataset = std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T>
void ana::sample<T>::limit_entries(long long max_entries) {
  if (m_initialized)
    throw std::logic_error("sample already initialized");
  m_max_entries = max_entries;
}

template <typename T> void ana::sample<T>::scale_weights(double scale) {
  if (m_initialized)
    throw std::logic_error("sample already initialized");
  m_scale *= scale;
}

template <typename T> void ana::sample<T>::initialize() {
  // can never be initialized twice
  if (m_initialized)
    return;

  // dataset must exist
  if (!m_dataset)
    throw std::runtime_error("no sample dataset opened");

  // 1. allocate the dataset partition
  m_partition = m_dataset->allocate_partition();
  // 2. truncate entries to limit
  m_partition.truncate(m_max_entries);
  // 3. merge parts to concurrency limit
  m_partition.merge(ana::multithread::concurrency());

  // calculate a normalization factor
  // scale the sample by its inverse
  m_scale /= m_dataset->normalize_scale();

  // open dataset reader and processor for each thread
  // model reprents whole dataset
  auto part = m_partition.total();
  auto rdr = m_dataset->read_dataset(part);
  auto proc = std::make_unique<dataset_processor_type>(part, *rdr, m_scale);
  m_readers.set_model(std::move(rdr));
  m_processors.set_model(std::move(proc));
  // slot for each partition range
  m_readers.clear_slots();
  m_processors.clear_slots();
  for (unsigned int islot = 0; islot < m_partition.size(); ++islot) {
    auto part = m_partition.get_part(islot);
    auto rdr = m_dataset->read_dataset(part);
    auto proc = std::make_unique<dataset_processor_type>(part, *rdr, m_scale);
    m_readers.add_slot(std::move(rdr));
    m_processors.add_slot(std::move(proc));
  }

  // sample is initialized
  m_initialized = true;
}

template <typename T>
unsigned long long ana::sample<T>::get_processed_entries() const {
  this->initialize();
  return m_partition.total().entries();
}

template <typename T> double ana::sample<T>::get_normalized_scale() const {
  this->initialize();
  return m_scale;
}

namespace ana {

template <typename T> class dataflow : public sample<T> {

public:
  template <typename U> class systematic;

  template <typename U> class delayed;

  template <typename U> class lazy;

public:
  using dataset_reader_type = typename sample<T>::dataset_reader_type;
  using dataset_processor_type = typename sample<T>::dataset_processor_type;

  template <typename U>
  static constexpr std::true_type
  check_lazy(typename dataflow<T>::template lazy<U> const &);
  static constexpr std::false_type check_lazy(...);
  template <typename U>
  static constexpr std::true_type
  check_delayed(typename dataflow<T>::template delayed<U> const &);
  static constexpr std::false_type check_delayed(...);

  template <typename V>
  static constexpr bool is_nominal_v =
      (decltype(check_lazy(std::declval<V>()))::value ||
       decltype(check_delayed(std::declval<V>()))::value);
  template <typename V> static constexpr bool is_varied_v = !is_nominal_v<V>;

  template <typename... Args>
  static constexpr bool has_no_variation_v = (is_nominal_v<Args> && ...);
  template <typename... Args>
  static constexpr bool has_variation_v = (is_varied_v<Args> || ...);

public:
  virtual ~dataflow() = default;

  template <typename... Args> dataflow(Args &&...args);

  template <typename U = T, typename = std::enable_if_t<std::is_constructible_v<
                                U, std::string, std::vector<std::string>>>>
  dataflow(const std::string &key, const std::vector<std::string> &file_paths);

  template <typename U = T, typename = std::enable_if_t<std::is_constructible_v<
                                U, std::vector<std::string>, std::string>>>
  dataflow(const std::vector<std::string> &file_paths, const std::string &key);

  dataflow(dataflow const &) = delete;
  dataflow &operator=(dataflow const &) = delete;

  dataflow(dataflow &&) = default;
  dataflow &operator=(dataflow &&) = default;

  /**
   * @brief Read a column from the dataset.
   * @tparam Val Column data type.
   * @param name Column name.
   * @return The `lazy` read column.
   */
  template <typename Val>
  auto read(const std::string &name)
      -> lazy<read_column_t<read_dataset_t<T>, Val>>;

  /**
   * @brief Define a constant.
   * @tparam Val Constant data type.
   * @param value Constant data value.
   * @return The `lazy` defined constant.
   */
  template <typename Val>
  auto constant(const Val &value) -> lazy<column::constant<Val>>;

  /**
   * @brief Define a custom definition or representation.
   * @tparam Def The full definition/representation user-implementation.
   * @param args Constructor arguments for the definition/representation.
   * @return The `lazy` definition "evaluator" to be evaluated with input
   * columns.
   */
  template <typename Def, typename... Args>
  auto define(Args &&...args) -> delayed<column::template evaluator_t<Def>>;

  /**
   * @brief Define an equation.
   * @tparam F Any function/functor/callable type.
   * @param callable The function/functor/callable object used as the
   * expression.
   * @return The `lazy` equation "evaluator" to be evaluated with input columns.
   */
  template <typename F>
  auto define(F callable) -> delayed<column::template evaluator_t<F>>;

  auto filter(const std::string &name);
  template <typename Sel> auto filter(const std::string &name);
  template <typename F> auto filter(const std::string &name, F callable);
  template <typename Sel, typename F>
  auto filter(const std::string &name, F callable)
      -> delayed<selection::template custom_applicator_t<F>>;

  auto channel(const std::string &name);
  template <typename Sel> auto channel(const std::string &name);
  template <typename F> auto channel(const std::string &name, F callable);
  template <typename Sel, typename F>
  auto channel(const std::string &name, F callable)
      -> delayed<selection::template custom_applicator_t<F>>;

  /**
   * @brief Book a aggregation
   * @tparam Cnt Any full user-implementation of `aggregation`.
   * @param args Constructor arguments for the **Cnt**.
   * @return The `lazy` aggregation "booker" to be filled with input column(s)
   * and booked at selection(s).
   */
  template <typename Cnt, typename... Args>
  auto book(Args &&...args) -> delayed<aggregation::booker<Cnt>>;

protected:
  dataflow();
  void analyze();
  void reset();

  template <typename Def, typename... Cols>
  auto evaluate_column(delayed<column::evaluator<Def>> const &calc,
                       lazy<Cols> const &...columns) -> lazy<Def>;
  template <typename Eqn, typename... Cols>
  auto apply_selection(delayed<selection::applicator<Eqn>> const &calc,
                       lazy<Cols> const &...columns) -> lazy<selection>;
  template <typename Cnt>
  auto select_aggregation(delayed<aggregation::booker<Cnt>> const &bkr,
                          lazy<selection> const &sel) -> lazy<Cnt>;
  template <typename Cnt, typename... Sels>
  auto select_aggregations(delayed<aggregation::booker<Cnt>> const &bkr,
                           lazy<Sels> const &...sels)
      -> delayed<aggregation::bookkeeper<Cnt>>;

  template <typename Sel>
  auto filter(lazy<selection> const &prev, const std::string &name);
  template <typename Sel, typename F>
  auto filter(lazy<selection> const &prev, const std::string &name, F callable)
      -> delayed<selection::template custom_applicator_t<F>>;

  template <typename Sel, typename F>
  auto channel(lazy<selection> const &prev, const std::string &name, F callable)
      -> delayed<selection::template custom_applicator_t<F>>;
  template <typename Sel>
  auto channel(lazy<selection> const &prev, const std::string &name);

  // recreate a lazy node as a variation under new arguments
  template <typename V, std::enable_if_t<ana::column::template is_reader_v<V>,
                                         bool> = false>
  auto vary_column(lazy<V> const &nom, const std::string &colname) -> lazy<V>;

  template <
      typename Val, typename V,
      std::enable_if_t<ana::column::template is_constant_v<V>, bool> = false>
  auto vary_column(lazy<V> const &nom, Val const &val) -> lazy<V>;

  template <typename... Args, typename V,
            std::enable_if_t<ana::column::template is_definition_v<V> &&
                                 !ana::column::template is_equation_v<V>,
                             bool> = false>
  auto vary_evaluator(delayed<column::evaluator<V>> const &nom, Args &&...args)
      -> delayed<column::evaluator<V>>;

  template <
      typename F, typename V,
      std::enable_if_t<ana::column::template is_equation_v<V>, bool> = false>
  auto vary_evaluator(delayed<column::evaluator<V>> const &nom, F callable)
      -> delayed<column::evaluator<V>>;

  void add_operation(lockstep::node<operation> act);
  void add_operation(std::unique_ptr<operation> act);

protected:
  bool m_analyzed;
  std::vector<std::unique_ptr<operation>> m_operations;
};

template <typename T> using dataflow_t = typename T::dataflow_type;
template <typename T> using operation_t = typename T::nominal_type;

} // namespace ana

/** @file */

#include <iostream>
#include <set>
#include <type_traits>

/** @file */

#include <set>
#include <string>
#include <type_traits>

namespace ana {

/**
 * @class dataflow::systematic dataflow_systematic.h
 * interface/dataflow_systematic.h
 * @brief Systematic of a dataflow operation.
 */
template <typename T> template <typename U> class dataflow<T>::systematic {

public:
  using dataflow_type = dataflow<T>;
  using dataset_type = T;
  using nominal_type = U;

public:
  friend class dataflow<T>;
  template <typename> friend class systematic;

public:
  systematic(dataflow<T> &dataflow);

  virtual ~systematic() = default;

public:
  virtual void set_variation(const std::string &var_name, U &&nom) = 0;

  virtual U const &nominal() const = 0;
  virtual U const &variation(const std::string &var_name) const = 0;

  virtual bool has_variation(const std::string &var_name) const = 0;
  virtual std::set<std::string> list_variation_names() const = 0;

protected:
  dataflow<T> *m_df;
};

template <typename... Nodes>
auto list_all_variation_names(Nodes const &...nodes) -> std::set<std::string>;

} // namespace ana

template <typename... Nodes>
auto ana::list_all_variation_names(Nodes const &...nodes)
    -> std::set<std::string> {
  std::set<std::string> variation_names;
  (variation_names.merge(nodes.list_variation_names()), ...);
  return variation_names;
}

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
  template <                                                                   \
      typename Arg, typename V = U,                                            \
      std::enable_if_t<ana::is_column_v<V> &&                                  \
                           ana::is_column_v<typename Arg::operation_type> &&   \
                           op_check::has_##op_name##_v<                        \
                               cell_value_t<V>,                                \
                               cell_value_t<typename Arg::operation_type>>,    \
                       bool> = false>                                          \
  auto operator op_symbol(Arg const &arg) const {                              \
    return this->m_df                                                          \
        ->define([](cell_value_t<V> const &me,                                 \
                    cell_value_t<typename Arg::operation_type> const &you) {   \
          return me op_symbol you;                                             \
        })                                                                     \
        .evaluate(*this, arg);                                                 \
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
  template <typename V = U,                                                    \
            std::enable_if_t<ana::is_column_v<V> &&                            \
                                 op_check::has_##op_name##_v<cell_value_t<V>>, \
                             bool> = false>                                    \
  auto operator op_symbol() const {                                            \
    return this->m_df                                                          \
        ->define([](cell_value_t<V> const &me) { return (op_symbol me); })     \
        .evaluate(*this);                                                      \
  }

#define CHECK_FOR_SUBSCRIPT_OP()                                               \
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

#define DEFINE_LAZY_SUBSCRIPT_OP()                                             \
  template <                                                                   \
      typename Arg, typename V = U,                                            \
      std::enable_if_t<is_column_v<V> &&                                       \
                           op_check::has_subscript_v<                          \
                               cell_value_t<V>,                                \
                               cell_value_t<typename Arg::operation_type>>,    \
                       bool> = false>                                          \
  auto operator[](Arg const &arg) const {                                      \
    return this->m_df->define(                                                 \
        [](cell_value_t<V> me,                                                 \
           cell_value_t<typename Arg::operation_type> index) {                 \
          return me[index];                                                    \
        })(*this, arg);                                                        \
  }

namespace ana {

namespace op_check {
CHECK_FOR_UNARY_OP(logical_not, !)
CHECK_FOR_UNARY_OP(minus, -)
CHECK_FOR_BINARY_OP(addition, +)
CHECK_FOR_BINARY_OP(subtroperation, -)
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
CHECK_FOR_SUBSCRIPT_OP()
} // namespace op_check

/**
 * @brief Node representing a operation to be performed in an analysis.
 * @details Lazy nodes represent the final operation to be performed in a
 * dataset, pending its processing. It can be provided to other dataflow
 * operations as inputs.
 * @tparam T Input dataset type
 * @tparam U Action to be performed lazily
 */
template <typename T>
template <typename U>
class dataflow<T>::lazy : public systematic<lazy<U>>, public lockstep::view<U> {

public:
  class varied;

public:
  using dataflow_type = typename systematic<lazy<U>>::dataflow_type;
  using dataset_type = typename systematic<lazy<U>>::dataset_type;
  using operation_type = U;

  template <typename Sel, typename... Args>
  using delayed_selection_applicator_t =
      decltype(std::declval<dataflow<T>>().template filter<Sel>(
          std::declval<std::string>(), std::declval<Args>()...));

public:
  // friends with the main dataflow graph & any other lazy nodes
  friend class dataflow<T>;
  template <typename> friend class lazy;

public:
  lazy(dataflow<T> &dataflow, const lockstep::view<U> &operation)
      : systematic<lazy<U>>::systematic(dataflow),
        lockstep::view<U>::view(operation) {}
  lazy(dataflow<T> &dataflow, const lockstep::node<U> &operation)
      : systematic<lazy<U>>::systematic(dataflow),
        lockstep::view<U>::view(operation) {}

  lazy(const lazy &) = default;
  lazy &operator=(const lazy &) = default;

  virtual ~lazy() = default;

  virtual void set_variation(const std::string &var_name, lazy &&var) override;

  virtual lazy const &nominal() const override;
  virtual lazy const &variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> list_variation_names() const override;

  /**
   * @brief Apply a systematic variation to a `reader` or `constant` column.
   * @param var_name Name of the systematic variation.
   * @param args... Alternate column name (`reader`) or value (`constant`).
   * @return Varied column.
   * @details Creates a `varied` operation whose `.nominal()` is the original
   * lazy one, and `variation(var_name)` is the newly-constructed one.
   */
  template <typename... Args, typename V = U,
            std::enable_if_t<ana::column::template is_reader_v<V> ||
                                 ana::column::template is_constant_v<V>,
                             bool> = false>
  auto vary(const std::string &var_name, Args &&...args) -> varied;

  template <typename Sel, typename... Args>
  auto filter(const std::string &name, Args &&...args) const
      -> delayed_selection_applicator_t<Sel, Args...>;

  template <typename Sel, typename... Args>
  auto channel(const std::string &name, Args &&...args) const
      -> delayed_selection_applicator_t<Sel, Args...>;

  template <typename V = U,
            std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  std::string path() const {
    return this->get_model_value(
        [](const selection &me) { return me.get_path(); });
  }

  /**
   * @brief Retrieve the result of a aggregation.
   * @details Triggers processing of the dataset if that the result is not
   * already available.
   * @return The result of the implemented aggregation.
   */
  template <typename V = U,
            std::enable_if_t<ana::aggregation::template has_output_v<V>, bool> =
                false>
  auto result() const -> decltype(std::declval<V>().get_result()) {
    this->m_df->analyze();
    this->merge_results();
    return this->get_model()->get_result();
  }

  /**
   * @brief Shorthand for `result` of aggregation.
   * @return `Result` the result of the implemented aggregation.
   */
  template <typename V = U,
            std::enable_if_t<ana::aggregation::template has_output_v<V>, bool> =
                false>
  auto operator->() const -> decltype(std::declval<V>().get_result()) {
    return this->result();
  }

  DEFINE_LAZY_SUBSCRIPT_OP()
  DEFINE_LAZY_UNARY_OP(logical_not, !)
  DEFINE_LAZY_UNARY_OP(minus, -)
  DEFINE_LAZY_BINARY_OP(equality, ==)
  DEFINE_LAZY_BINARY_OP(inequality, !=)
  DEFINE_LAZY_BINARY_OP(addition, +)
  DEFINE_LAZY_BINARY_OP(subtroperation, -)
  DEFINE_LAZY_BINARY_OP(multiplication, *)
  DEFINE_LAZY_BINARY_OP(division, /)
  DEFINE_LAZY_BINARY_OP(logical_or, ||)
  DEFINE_LAZY_BINARY_OP(logical_and, &&)
  DEFINE_LAZY_BINARY_OP(greater_than, >)
  DEFINE_LAZY_BINARY_OP(less_than, <)
  DEFINE_LAZY_BINARY_OP(greater_than_or_equal_to, >=)
  DEFINE_LAZY_BINARY_OP(less_than_or_equal_to, <=)

protected:
  template <typename V = U,
            std::enable_if_t<ana::aggregation::template has_output_v<V>, bool> =
                false>
  void merge_results() const {
    auto model = this->get_model();
    if (!model->is_merged()) {
      std::vector<std::decay_t<decltype(model->get_result())>> results;
      results.reserve(this->concurrency());
      for (size_t islot = 0; islot < this->concurrency(); ++islot) {
        results.push_back(this->get_slot(islot)->get_result());
      }
      model->merge_results(results);
    }
  }
};

} // namespace ana

#include <set>
#include <type_traits>
#include <unordered_map>
#include <utility>

#define DECLARE_LAZY_VARIED_BINARY_OP(op_symbol)                               \
  template <typename Arg>                                                      \
  auto operator op_symbol(Arg &&b) const->typename lazy<                       \
      typename decltype(std::declval<lazy<Act>>().operator op_symbol(          \
          std::forward<Arg>(b).nominal()))::operation_type>::varied;
#define DEFINE_LAZY_VARIED_BINARY_OP(op_symbol)                                \
  template <typename T>                                                        \
  template <typename Act>                                                      \
  template <typename Arg>                                                      \
  auto ana::dataflow<T>::lazy<Act>::varied::operator op_symbol(Arg &&b)        \
      const->typename lazy<                                                    \
          typename decltype(std::declval<lazy<Act>>().operator op_symbol(      \
              std::forward<Arg>(b).nominal()))::operation_type>::varied {      \
    auto syst = typename lazy<                                                 \
        typename decltype(std::declval<lazy<Act>>().operator op_symbol(        \
            std::forward<Arg>(b).nominal()))::operation_type>::                \
        varied(this->nominal().operator op_symbol(                             \
            std::forward<Arg>(b).nominal()));                                  \
    for (auto const &var_name :                                                \
         list_all_variation_names(*this, std::forward<Arg>(b))) {              \
      syst.set_variation(var_name,                                             \
                         variation(var_name).operator op_symbol(               \
                             std::forward<Arg>(b).variation(var_name)));       \
    }                                                                          \
    return syst;                                                               \
  }
#define DECLARE_LAZY_VARIED_UNARY_OP(op_symbol)                                \
  template <typename V = Act,                                                  \
            std::enable_if_t<ana::is_column_v<V>, bool> = false>               \
  auto operator op_symbol() const->typename lazy<                              \
      typename decltype(std::declval<lazy<V>>().                               \
                        operator op_symbol())::operation_type>::varied;
#define DEFINE_LAZY_VARIED_UNARY_OP(op_name, op_symbol)                        \
  template <typename T>                                                        \
  template <typename Act>                                                      \
  template <typename V, std::enable_if_t<ana::is_column_v<V>, bool>>           \
  auto ana::dataflow<T>::lazy<Act>::varied::operator op_symbol() const->       \
      typename lazy<                                                           \
          typename decltype(std::declval<lazy<V>>().                           \
                            operator op_symbol())::operation_type>::varied {   \
    auto syst = typename lazy<                                                 \
        typename decltype(std::declval<lazy<V>>().operator op_symbol())::      \
            operation_type>::varied(this->nominal().operator op_symbol());     \
    for (auto const &var_name : list_all_variation_names(*this)) {             \
      syst.set_variation(var_name, variation(var_name).operator op_symbol());  \
    }                                                                          \
    return syst;                                                               \
  }

namespace ana {

/**
 * @brief Variations of a lazy operation to be performed in an dataflow.
 * @tparam T Input dataset type
 * @tparam U Actions to be performed lazily.
 * @details A `varied` node can be treated identical to a `lazy` one, except
 * that it contains multiple variations of the operation as dictated by the
 * analyzer that propagate through the rest of the analysis.
 */
template <typename T>
template <typename Act>
class dataflow<T>::lazy<Act>::varied : public systematic<lazy<Act>> {

public:
  using dataflow_type = typename lazy<Act>::dataflow_type;
  using dataset_type = typename lazy<Act>::dataset_type;
  using operation_type = typename lazy<Act>::operation_type;

  template <typename Sel, typename... Args>
  using delayed_varied_selection_applicator_t =
      typename decltype(std::declval<dataflow<T>>().template filter<Sel>(
          std::declval<std::string>(), std::declval<Args>()...))::varied;

public:
  varied(lazy<Act> const &nom);
  ~varied() = default;

  virtual void set_variation(const std::string &var_name, lazy &&var) override;

  virtual lazy const &nominal() const override;
  virtual lazy const &variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> list_variation_names() const override;

  template <typename Sel, typename... Args, typename V = Act,
            std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  auto filter(const std::string &name, Args &&...arguments)
      -> delayed_varied_selection_applicator_t<Sel, Args...>;

  template <typename Sel, typename... Args, typename V = Act,
            std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  auto channel(const std::string &name, Args &&...arguments)
      -> delayed_varied_selection_applicator_t<Sel, Args...>;

  template <typename Node, typename V = Act,
            std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  auto operator||(const Node &b) const -> typename lazy<selection>::varied;

  template <typename Node, typename V = Act,
            std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  auto operator&&(const Node &b) const -> typename lazy<selection>::varied;

  template <typename Node, typename V = Act,
            std::enable_if_t<ana::is_selection_v<V>, bool> = false>
  auto operator*(const Node &b) const -> typename lazy<selection>::varied;

  template <typename V = Act,
            std::enable_if_t<ana::is_column_v<V> ||
                                 ana::aggregation::template has_output_v<V>,
                             bool> = false>
  auto operator[](const std::string &var_name) const -> lazy<V>;

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

} // namespace ana

template <typename T>
template <typename Act>
ana::dataflow<T>::lazy<Act>::varied::varied(lazy<Act> const &nom)
    : systematic<lazy<Act>>::systematic(*nom.m_df), m_nom(nom) {}

template <typename T>
template <typename Act>
void ana::dataflow<T>::lazy<Act>::varied::set_variation(
    const std::string &var_name, lazy &&var) {
  m_var_map.insert(std::make_pair(var_name, std::move(var)));
  m_var_names.insert(var_name);
}

template <typename T>
template <typename Act>
auto ana::dataflow<T>::lazy<Act>::varied::nominal() const -> lazy const & {
  return m_nom;
}

template <typename T>
template <typename Act>
auto ana::dataflow<T>::lazy<Act>::varied::variation(
    const std::string &var_name) const -> lazy const & {
  return (this->has_variation(var_name) ? m_var_map.at(var_name) : m_nom);
}

template <typename T>
template <typename Act>
bool ana::dataflow<T>::lazy<Act>::varied::has_variation(
    const std::string &var_name) const {
  return m_var_map.find(var_name) != m_var_map.end();
}

template <typename T>
template <typename Act>
std::set<std::string>
ana::dataflow<T>::lazy<Act>::varied::list_variation_names() const {
  return m_var_names;
}

template <typename T>
template <typename Act>
template <typename Sel, typename... Args, typename V,
          std::enable_if_t<ana::is_selection_v<V>, bool>>
auto ana::dataflow<T>::lazy<Act>::varied::filter(const std::string &name,
                                                 Args &&...arguments)
    -> delayed_varied_selection_applicator_t<Sel, Args...> {

  using varied_type = delayed_varied_selection_applicator_t<Sel, Args...>;

  auto syst = varied_type(this->nominal().template filter<Sel>(
      name, std::forward<Args>(arguments)...));

  for (auto const &var_name : this->list_variation_names()) {
    syst.set_variation(var_name, this->variation(var_name).template filter<Sel>(
                                     name, std::forward<Args>(arguments)...));
  }
  return syst;
}

template <typename T>
template <typename Act>
template <typename Sel, typename... Args, typename V,
          std::enable_if_t<ana::is_selection_v<V>, bool>>
auto ana::dataflow<T>::lazy<Act>::varied::channel(const std::string &name,
                                                  Args &&...arguments)
    -> delayed_varied_selection_applicator_t<Sel, Args...> {
  using varied_type = delayed_varied_selection_applicator_t<Sel, Args...>;
  auto syst = varied_type(this->nominal().template channel<Sel>(
      name, std::forward<Args>(arguments)...));
  for (auto const &var_name : this->list_variation_names()) {
    syst.set_variation(var_name,
                       this->variation(var_name).template channel<Sel>(
                           name, std::forward<Args>(arguments)...));
  }
  return syst;
}

template <typename T>
template <typename Act>
template <typename Node, typename V,
          std::enable_if_t<ana::is_selection_v<V>, bool>>
auto ana::dataflow<T>::lazy<Act>::varied::operator&&(const Node &b) const ->
    typename lazy<selection>::varied {
  using varied_type = typename lazy<selection>::varied;
  auto syst = varied_type(this->nominal().operator&&(b.nominal()));
  for (auto const &var_name : list_all_variation_names(*this, b)) {
    syst.set_variation(
        var_name, this->variation(var_name).operator&&(b.variation(var_name)));
  }
  return syst;
}

template <typename T>
template <typename Act>
template <typename Node, typename V,
          std::enable_if_t<ana::is_selection_v<V>, bool>>
auto ana::dataflow<T>::lazy<Act>::varied::operator*(const Node &b) const ->
    typename lazy<selection>::varied {
  using varied_type = typename lazy<selection>::varied;
  auto syst = varied_type(this->nominal().operator*(b.nominal()));
  for (auto const &var_name : list_all_variation_names(*this, b)) {
    syst.set_variation(
        var_name, this->variation(var_name).operator*(b.variation(var_name)));
  }
  return syst;
}

template <typename T>
template <typename Act>
template <typename Node, typename V,
          std::enable_if_t<ana::is_selection_v<V>, bool>>
auto ana::dataflow<T>::lazy<Act>::varied::operator||(const Node &b) const ->
    typename lazy<selection>::varied {
  using varied_type = typename lazy<selection>::varied;
  auto syst = varied_type(this->nominal().operator||(b.nominal()));
  for (auto const &var_name : list_all_variation_names(*this, b)) {
    syst.set_variation(
        var_name, this->variation(var_name).operator||(b.variation(var_name)));
  }
  return syst;
}

template <typename DS>
template <typename Act>
template <typename V,
          std::enable_if_t<ana::is_column_v<V> ||
                               ana::aggregation::template has_output_v<V>,
                           bool>>
auto ana::dataflow<DS>::lazy<Act>::varied::operator[](
    const std::string &var_name) const -> lazy<V> {
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

template <typename T>
template <typename Act>
void ana::dataflow<T>::lazy<Act>::set_variation(const std::string &, lazy &&) {
  // should never be called
  throw std::logic_error("cannot set variation to a lazy operation");
}

template <typename T>
template <typename Act>
auto ana::dataflow<T>::lazy<Act>::nominal() const -> lazy const & {
  // this is nominal
  return *this;
}

template <typename T>
template <typename Act>
auto ana::dataflow<T>::lazy<Act>::variation(const std::string &) const
    -> lazy const & {
  // propagation of variations must occur "transparently"
  return *this;
}

template <typename T>
template <typename Act>
std::set<std::string>
ana::dataflow<T>::lazy<Act>::list_variation_names() const {
  // no variations to list
  return std::set<std::string>();
}

template <typename T>
template <typename Act>
bool ana::dataflow<T>::lazy<Act>::has_variation(const std::string &) const {
  // always false
  return false;
}

template <typename T>
template <typename Act>
template <typename Sel, typename... Args>
auto ana::dataflow<T>::lazy<Act>::filter(const std::string &name,
                                         Args &&...args) const
    -> delayed_selection_applicator_t<Sel, Args...> {
  if constexpr (std::is_base_of_v<selection, Act>) {
    return this->m_df->template filter<Sel>(*this, name,
                                            std::forward<Args>(args)...);
  } else {
    static_assert(std::is_base_of_v<selection, Act>,
                  "filter must be called from a selection");
  }
}

template <typename T>
template <typename Act>
template <typename Sel, typename... Args>
auto ana::dataflow<T>::lazy<Act>::channel(const std::string &name,
                                          Args &&...args) const
    -> delayed_selection_applicator_t<Sel, Args...> {
  if constexpr (std::is_base_of_v<selection, Act>) {
    return this->m_df->template channel<Sel>(*this, name,
                                             std::forward<Args>(args)...);
  } else {
    static_assert(std::is_base_of_v<selection, Act>,
                  "channel must be called from a selection");
  }
}

template <typename T>
template <typename Act>
template <typename... Args, typename V,
          std::enable_if_t<ana::column::template is_reader_v<V> ||
                               ana::column::template is_constant_v<V>,
                           bool>>
auto ana::dataflow<T>::lazy<Act>::vary(const std::string &var_name,
                                       Args &&...args) -> varied {
  // create a lazy varied with the this as nominal
  auto syst = varied(std::move(*this));
  // set variation of the column according to new constructor arguments
  syst.set_variation(
      var_name,
      this->m_df->vary_column(syst.nominal(), std::forward<Args>(args)...));
  // done
  return syst;
}

namespace ana {

/**
 * @brief Node representing a operation to be performed in an analysis.
 * @details A delayed operation is not yet fully-specified to be considered
 * lazy, as they require existing lazy operations as inputs to create a lazy one
 * of itself.
 * @tparam T Input dataset type.
 * @tparam U Action for which a lazy one will be created.
 */
template <typename DS>
template <typename Bld>
class dataflow<DS>::delayed : public systematic<delayed<Bld>>,
                              public lockstep::node<Bld> {

public:
  class varied;

public:
  delayed(dataflow<DS> &dataflow, lockstep::node<Bld> &&operation)
      : systematic<delayed<Bld>>::systematic(dataflow),
        lockstep::node<Bld>::node(std::move(operation)) {}

  virtual ~delayed() = default;

  template <typename V>
  delayed(delayed<V> &&other)
      : systematic<delayed<Bld>>::systematic(*other.m_df),
        lockstep::node<Bld>::node(std::move(other)) {}

  template <typename V> delayed &operator=(delayed<V> &&other) {
    this->m_df = other.m_df;
    lockstep::node<Bld>::operator=(std::move(other));
    return *this;
  }

  virtual void set_variation(const std::string &var_name,
                             delayed &&var) override;

  virtual delayed const &nominal() const override;
  virtual delayed const &variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> list_variation_names() const override;

  /**
   * @brief Apply a systematic variation to a column evaluator.
   * @param var_name Name of the systematic variation.
   * @param args Constructor arguments column.
   * @return Varied column evaluator.
   * @details This method is to vary the instantiation of columns that must be
   * `evaluated()`ed from input columns, which the `vary()` call must precede:
   * ```cpp
   * auto energy = df.read<float>("energy");
   * auto e_pm_1pc = df.define([](float x){return
   * x*1.0;}).vary("plus_1pc",[](double x){return
   * x*1.01;}).vary("minus_1pc",[](double x){return x*0.99;}).evaluate(energy);
   * ```
   */
  template <
      typename... Args, typename V = Bld,
      std::enable_if_t<ana::column::template is_evaluator_v<V>, bool> = false>
  auto vary(const std::string &var_name, Args &&...args) ->
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

    using varied_type = typename lazy<column::template evaluated_t<V>>::varied;

    auto nom = this->m_df->evaluate_column(*this, columns.nominal()...);
    auto syst = varied_type(std::move(nom));

    for (auto const &var_name : list_all_variation_names(columns...)) {
      auto var =
          this->m_df->evaluate_column(*this, columns.variation(var_name)...);
      syst.set_variation(var_name, std::move(var));
    }

    return syst;
  }

  /**
   * @brief Apply the selection's expression based on input columns.
   * @param columns Input columns.
   * @return Applied selection.
   */
  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<ana::selection::template is_applicator_v<V>,
                             bool> = false>
  auto apply(Nodes &&...columns) const
      -> decltype(std::declval<delayed<V>>().apply_selection(
          std::forward<Nodes>(columns)...)) {
    return this->apply_selection(std::forward<Nodes>(columns)...);
  }

  /**
   * @brief Fill the aggregation with input columns.
   * @param columns Input columns
   * @return The aggregation filled with input columns.
   */
  template <
      typename... Nodes, typename V = Bld,
      std::enable_if_t<ana::aggregation::template is_booker_v<V>, bool> = false>
  auto fill(Nodes &&...columns) const
      -> decltype(std::declval<delayed<V>>().fill_aggregation(
          std::declval<Nodes>()...)) {
    return this->fill_aggregation(std::forward<Nodes>(columns)...);
  }

  /**
   * @brief Book the aggregation at a selection.
   * @param selection Selection to be counted.
   * @return The aggregation booked at the selection.
   */
  template <typename Node> auto at(Node &&selection) const {
    return this->select_aggregation(std::forward<Node>(selection));
  }

  template <typename Node, typename V = Bld,
            std::enable_if_t<ana::aggregation::template is_booker_v<V> &&
                                 ana::dataflow<DS>::template is_nominal_v<Node>,
                             bool> = false>
  auto select_aggregation(Node const &sel) const
      -> lazy<aggregation::booked_t<V>> {
    // nominal
    return this->m_df->select_aggregation(*this, sel);
  }

  template <typename Node, typename V = Bld,
            std::enable_if_t<ana::aggregation::template is_booker_v<V> &&
                                 ana::dataflow<DS>::template is_varied_v<Node>,
                             bool> = false>
  auto select_aggregation(Node const &sel) const ->
      typename lazy<aggregation::booked_t<V>>::varied {
    using varied_type = typename lazy<aggregation::booked_t<V>>::varied;
    auto syst =
        varied_type(this->m_df->select_aggregation(*this, sel.nominal()));
    for (auto const &var_name : list_all_variation_names(sel)) {
      syst.set_variation(var_name, this->m_df->select_aggregation(
                                       *this, sel.variation(var_name)));
    }
    return syst;
  }

  /**
   * @brief Book multiple aggregations, one at each selection.
   * @param selections The selections.
   * @return The aggregations booked at selections.
   */
  template <typename... Nodes> auto at(Nodes &&...nodes) const {
    static_assert(aggregation::template is_booker_v<Bld>,
                  "not a aggregation (booker)");
    return this->select_aggregations(std::forward<Nodes>(nodes)...);
  }

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<
                ana::aggregation::template is_booker_v<V> &&
                    ana::dataflow<DS>::template has_no_variation_v<Nodes...>,
                bool> = false>
  auto select_aggregations(Nodes const &...sels) const
      -> delayed<aggregation::bookkeeper<aggregation::booked_t<V>>> {
    // nominal
    return this->m_df->select_aggregations(*this, sels...);
  }

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<ana::aggregation::template is_booker_v<V> &&
                                 has_variation_v<Nodes...>,
                             bool> = false>
  auto select_aggregations(Nodes const &...sels) const -> typename delayed<
      aggregation::bookkeeper<aggregation::booked_t<V>>>::varied {
    // variations
    using varied_type = typename delayed<
        aggregation::bookkeeper<aggregation::booked_t<V>>>::varied;
    auto syst =
        varied_type(this->m_df->select_aggregations(*this, sels.nominal()...));
    for (auto const &var_name : list_all_variation_names(sels...)) {
      syst.set_variation(var_name, this->m_df->select_aggregations(
                                       *this, sels.variation(var_name)...));
    }
    return syst;
  }

  /**
   * @return The list of booked selection paths.
   */
  template <typename V = Bld,
            std::enable_if_t<ana::aggregation::template is_bookkeeper_v<V>,
                             bool> = false>
  auto list_selection_paths() const -> std::set<std::string> {
    return this->get_model_value(
        [](Bld const &bkpr) { return bkpr.list_selection_paths(); });
  }

  /**
   * @brief Shorthand for `evaluate()` and `apply()`
   * for column and selection respectively.
   * @param columns The input columns.
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

  /**
   * @brief Access a aggregation booked at a selection path.
   * @param selection_path The selection path.
   * @return The aggregation.
   */
  template <
      typename... Args, typename V = Bld,
      std::enable_if_t<aggregation::template is_bookkeeper_v<V>, bool> = false>
  auto operator[](const std::string &selection_path) const
      -> lazy<aggregation::booked_t<V>> {
    return this->get_aggregation(selection_path);
  }

protected:
  template <typename V = Bld,
            std::enable_if_t<ana::aggregation::template is_bookkeeper_v<V>,
                             bool> = false>
  auto get_aggregation(const std::string &selection_path) const
      -> lazy<aggregation::booked_t<V>> {
    return lazy<aggregation::booked_t<V>>(
        *this->m_df,
        this->get_lockstep_view([selection_path = selection_path](V &bkpr) {
          return bkpr.get_aggregation(selection_path);
        }));
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

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<
                ana::aggregation::template is_booker_v<V> &&
                    ana::dataflow<DS>::template has_no_variation_v<Nodes...>,
                bool> = false>
  auto fill_aggregation(Nodes const &...columns) const -> delayed<V> {
    // nominal
    return delayed<V>(
        *this->m_df,
        this->get_lockstep_node(
            [](V &fillable, typename Nodes::operation_type &...cols) {
              return fillable.book_fill(cols...);
            },
            columns...));
  }

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<ana::aggregation::template is_booker_v<V> &&
                                 has_variation_v<Nodes...>,
                             bool> = false>
  auto fill_aggregation(Nodes const &...columns) const -> varied {
    auto syst = varied(std::move(this->fill_aggregation(columns.nominal()...)));
    for (auto const &var_name : list_all_variation_names(columns...)) {
      syst.set_variation(var_name, std::move(this->fill_aggregation(
                                       columns.variation(var_name)...)));
    }
    return syst;
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
    using varied_type = typename lazy<selection>::varied;
    auto syst =
        varied_type(this->nominal().apply_selection(columns.nominal()...));
    auto var_names = list_all_variation_names(columns...);
    for (auto const &var_name : var_names) {
      syst.set_variation(var_name, this->variation(var_name).apply_selection(
                                       columns.variation(var_name)...));
    }
    return syst;
  }
};

} // namespace ana

template <typename DS>
template <typename Bld>
void ana::dataflow<DS>::delayed<Bld>::set_variation(const std::string &,
                                                    delayed<Bld> &&) {
  // should never be called
  throw std::logic_error("cannot set variation to a lazy operation");
}

template <typename DS>
template <typename Bld>
auto ana::dataflow<DS>::delayed<Bld>::nominal() const -> delayed const & {
  // this is nominal
  return *this;
}

template <typename DS>
template <typename Bld>
auto ana::dataflow<DS>::delayed<Bld>::variation(const std::string &) const
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
template <typename... Args, typename V,
          std::enable_if_t<ana::column::template is_evaluator_v<V>, bool>>
auto ana::dataflow<DS>::delayed<Bld>::vary(const std::string &var_name,
                                           Args &&...args) ->
    typename delayed<V>::varied {
  auto syst = varied(std::move(*this));
  syst.set_variation(var_name,
                     std::move(syst.m_df->vary_evaluator(
                         syst.nominal(), std::forward<Args>(args)...)));
  return syst;
}

template <typename T>
template <typename U>
ana::dataflow<T>::systematic<U>::systematic(dataflow<T> &df) : m_df(&df) {}

template <typename T> ana::dataflow<T>::dataflow() : m_analyzed(false) {}

template <typename T>
template <typename... Args>
ana::dataflow<T>::dataflow(Args &&...args) : dataflow<T>::dataflow() {
  this->prepare(std::forward<Args>(args)...);
}

template <typename T>
template <typename U, typename>
ana::dataflow<T>::dataflow(const std::string &key,
                           const std::vector<std::string> &file_paths)
    : dataflow<T>::dataflow() {
  this->prepare(key, file_paths);
}

template <typename T>
template <typename U, typename>
ana::dataflow<T>::dataflow(const std::vector<std::string> &file_paths,
                           const std::string &key)
    : dataflow<T>::dataflow() {
  this->prepare(file_paths, key);
}

template <typename T>
template <typename Val>
auto ana::dataflow<T>::read(const std::string &name)
    -> lazy<read_column_t<read_dataset_t<T>, Val>> {
  this->initialize();
  auto act = this->m_processors.get_lockstep_node(
      [name = name](dataset_processor_type &proc) {
        return proc.template read<Val>(name);
      });
  auto lzy = lazy<read_column_t<read_dataset_t<T>, Val>>(*this, act);
  this->add_operation(std::move(act));
  return lzy;
}

template <typename T>
template <typename Val>
auto ana::dataflow<T>::constant(const Val &val)
    -> lazy<ana::column::constant<Val>> {
  this->initialize();
  auto act = this->m_processors.get_lockstep_node(
      [val = val](dataset_processor_type &proc) {
        return proc.template constant<Val>(val);
      });
  auto lzy = lazy<column::constant<Val>>(*this, act);
  this->add_operation(std::move(act));
  return lzy;
}

template <typename T>
template <typename Def, typename... Args>
auto ana::dataflow<T>::define(Args &&...args)
    -> delayed<ana::column::template evaluator_t<Def>> {
  this->initialize();
  return delayed<ana::column::template evaluator_t<Def>>(
      *this,
      this->m_processors.get_lockstep_node(
          [&args...](dataset_processor_type &proc) {
            return proc.template define<Def>(std::forward<Args>(args)...);
          }));
}

template <typename T>
template <typename F>
auto ana::dataflow<T>::define(F callable)
    -> delayed<column::template evaluator_t<F>> {
  this->initialize();
  return delayed<ana::column::template evaluator_t<F>>(
      *this, this->m_processors.get_lockstep_node(
                 [callable = callable](dataset_processor_type &proc) {
                   return proc.template define(callable);
                 }));
}

template <typename T> auto ana::dataflow<T>::filter(const std::string &name) {
  auto callable = [](double x) { return x; };
  return this->template filter<selection::cut, decltype(callable)>(name,
                                                                   callable);
}

template <typename T>
template <typename Sel>
auto ana::dataflow<T>::filter(const std::string &name) {
  auto callable = [](double x) { return x; };
  return this->template filter<Sel, decltype(callable)>(name, callable);
}

template <typename T>
template <typename F>
auto ana::dataflow<T>::filter(const std::string &name, F callable) {
  return this->template filter<selection::cut, F>(name, callable);
}

template <typename T>
template <typename Sel, typename F>
auto ana::dataflow<T>::filter(const std::string &name, F callable)
    -> delayed<selection::template custom_applicator_t<F>> {
  this->initialize();
  return delayed<selection::template custom_applicator_t<F>>(
      *this,
      this->m_processors.get_lockstep_node(
          [name = name, callable = callable](dataset_processor_type &proc) {
            return proc.template filter<Sel>(nullptr, name, callable);
          }));
}

template <typename T> auto ana::dataflow<T>::channel(const std::string &name) {
  auto callable = [](double x) { return x; };
  return this->template channel<selection::cut, decltype(callable)>(name,
                                                                    callable);
}

template <typename T>
template <typename Sel>
auto ana::dataflow<T>::channel(const std::string &name) {
  auto callable = [](double x) { return x; };
  return this->template channel<Sel, decltype(callable)>(name, callable);
}

template <typename T>
template <typename F>
auto ana::dataflow<T>::channel(const std::string &name, F callable) {
  return this->template channel<selection::cut, F>(name, callable);
}

template <typename T>
template <typename Sel, typename F>
auto ana::dataflow<T>::channel(const std::string &name, F callable)
    -> delayed<selection::template custom_applicator_t<F>> {
  this->initialize();
  return delayed<selection::template custom_applicator_t<F>>(
      *this,
      this->m_processors.get_lockstep_node(
          [name = name, callable = callable](dataset_processor_type &proc) {
            return proc.template channel<Sel>(nullptr, name, callable);
          }));
}

template <typename T>
template <typename Cnt, typename... Args>
auto ana::dataflow<T>::book(Args &&...args)
    -> delayed<aggregation::booker<Cnt>> {
  return delayed<aggregation::booker<Cnt>>(
      *this, this->m_processors.get_lockstep_node(
                 [&args...](dataset_processor_type &proc) {
                   return proc.template book<Cnt>(std::forward<Args>(args)...);
                 }));
}

template <typename T>
template <typename Sel>
auto ana::dataflow<T>::filter(lazy<selection> const &prev,
                              const std::string &name) {
  auto callable = [](double x) { return x; };
  return this->template filter<Sel, decltype(callable)>(prev, name, callable);
}

template <typename T>
template <typename Sel>
auto ana::dataflow<T>::channel(lazy<selection> const &prev,
                               const std::string &name) {
  auto callable = [](double x) { return x; };
  return this->template channel<Sel, decltype(callable)>(prev, name, callable);
}

template <typename T>
template <typename Sel, typename F>
auto ana::dataflow<T>::filter(lazy<selection> const &prev,
                              const std::string &name, F callable)
    -> delayed<selection::template custom_applicator_t<F>> {
  this->initialize();
  return delayed<selection::template custom_applicator_t<F>>(
      *this, this->m_processors.get_lockstep_node(
                 [name = name, callable = callable](
                     dataset_processor_type &proc, selection const &prev) {
                   return proc.template filter<Sel>(&prev, name, callable);
                 },
                 prev));
}

template <typename T>
template <typename Sel, typename F>
auto ana::dataflow<T>::channel(lazy<selection> const &prev,
                               const std::string &name, F callable)
    -> delayed<selection::template custom_applicator_t<F>> {
  this->initialize();
  return delayed<selection::template custom_applicator_t<F>>(
      *this, this->m_processors.get_lockstep_node(
                 [name = name, callable = callable](
                     dataset_processor_type &proc, selection const &prev) {
                   return proc.template channel<Sel>(&prev, name, callable);
                 },
                 prev));
}

template <typename T>
template <typename Def, typename... Cols>
auto ana::dataflow<T>::evaluate_column(
    delayed<column::evaluator<Def>> const &calc, lazy<Cols> const &...columns)
    -> lazy<Def> {
  auto act = this->m_processors.get_lockstep_node(
      [](dataset_processor_type &proc, column::evaluator<Def> &calc,
         Cols const &...cols) {
        return proc.template evaluate_column(calc, cols...);
      },
      lockstep::view<column::evaluator<Def>>(calc), columns...);
  auto lzy = lazy<Def>(*this, act);
  this->add_operation(std::move(act));
  return lzy;
}

template <typename T>
template <typename Eqn, typename... Cols>
auto ana::dataflow<T>::apply_selection(
    delayed<selection::applicator<Eqn>> const &calc,
    lazy<Cols> const &...columns) -> lazy<selection> {
  auto act = this->m_processors.get_lockstep_node(
      [](dataset_processor_type &proc, selection::applicator<Eqn> &calc,
         Cols &...cols) {
        return proc.template apply_selection(calc, cols...);
      },
      lockstep::view<selection::applicator<Eqn>>(calc), columns...);
  auto lzy = lazy<selection>(*this, act);
  this->add_operation(std::move(act));
  return lzy;
}

template <typename T>
template <typename Cnt>
auto ana::dataflow<T>::select_aggregation(
    delayed<aggregation::booker<Cnt>> const &bkr, lazy<selection> const &sel)
    -> lazy<Cnt> {
  // any time a new aggregation is booked, means the dataflow must run: so reset
  // its status
  this->reset();
  auto act = this->m_processors.get_lockstep_node(
      [](dataset_processor_type &proc, aggregation::booker<Cnt> &bkr,
         const selection &sel) { return proc.select_aggregation(bkr, sel); },
      lockstep::view<aggregation::booker<Cnt>>(bkr), sel);
  auto lzy = lazy<Cnt>(*this, act);
  this->add_operation(std::move(act));
  return lzy;
}

template <typename T>
template <typename Cnt, typename... Sels>
auto ana::dataflow<T>::select_aggregations(
    delayed<aggregation::booker<Cnt>> const &bkr, lazy<Sels> const &...sels)
    -> delayed<aggregation::bookkeeper<Cnt>> {
  // any time a new aggregation is booked, means the dataflow must run: so reset
  // its status
  this->reset();

  using delayed_bookkeeper_type = delayed<aggregation::bookkeeper<Cnt>>;
  auto bkpr = delayed_bookkeeper_type(
      *this, this->m_processors.get_lockstep_node(
                 [this](dataset_processor_type &proc,
                        aggregation::booker<Cnt> &bkr, Sels const &...sels) {
                   // get bookkeeper and aggregations
                   auto bkpr_and_cntrs = proc.select_aggregations(bkr, sels...);

                   // add each aggregation to this dataflow
                   for (auto &&cntr : bkpr_and_cntrs.second) {
                     this->add_operation(std::move(cntr));
                   }

                   // take the bookkeeper
                   return std::move(bkpr_and_cntrs.first);
                 },
                 lockstep::view<aggregation::booker<Cnt>>(bkr), sels...));
  //  lockstep::node<aggregation::booker<Cnt>>(bkr), sels...));

  return bkpr;
}

template <typename T> void ana::dataflow<T>::analyze() {
  // do not analyze if already done
  if (m_analyzed)
    return;

  // ignore future analyze() requests until reset() is called
  m_analyzed = true;

  this->m_dataset->start_dataset();

  // multithread (if enabled)
  this->m_processors.run_slots(
      [](dataset_processor_type &proc) { proc.process(); });

  this->m_dataset->finish_dataset();

  // clear aggregations in aggregation::experiment
  // if they are not, they will be repeated in future runs
  this->m_processors.call_all_slots(
      [](dataset_processor_type &proc) { proc.clear_aggregations(); });
}

template <typename T> void ana::dataflow<T>::reset() { m_analyzed = false; }

template <typename T>
template <typename V,
          std::enable_if_t<ana::column::template is_reader_v<V>, bool>>
auto ana::dataflow<T>::vary_column(lazy<V> const &, const std::string &colname)
    -> lazy<V> {
  return this->read<cell_value_t<std::decay_t<V>>>(colname);
}

template <typename T>
template <typename Val, typename V,
          std::enable_if_t<ana::column::template is_constant_v<V>, bool>>
auto ana::dataflow<T>::vary_column(lazy<V> const &, Val const &val) -> lazy<V> {
  return this->constant<Val>(val);
}

template <typename T>
template <typename... Args, typename V,
          std::enable_if_t<ana::column::template is_definition_v<V> &&
                               !ana::column::template is_equation_v<V>,
                           bool>>
auto ana::dataflow<T>::vary_evaluator(delayed<column::evaluator<V>> const &,
                                      Args &&...args)
    -> delayed<column::evaluator<V>> {
  return this->define<V>(std::forward<Args>(args)...);
}

template <typename T>
template <typename F, typename V,
          std::enable_if_t<ana::column::template is_equation_v<V>, bool>>
auto ana::dataflow<T>::vary_evaluator(delayed<column::evaluator<V>> const &,
                                      F callable)
    -> delayed<column::evaluator<V>> {
  return this->define(typename V::function_type(callable));
}

template <typename T>
void ana::dataflow<T>::add_operation(lockstep::node<operation> operation) {
  m_operations.emplace_back(std::move(operation.m_model));
  for (unsigned int i = 0; i < operation.concurrency(); ++i) {
    m_operations.emplace_back(std::move(operation.m_slots[i]));
  }
}

template <typename T>
void ana::dataflow<T>::add_operation(std::unique_ptr<operation> operation) {
  m_operations.emplace_back(std::move(operation));
}

namespace ana {

/**
 * @brief Varied version of a delayed operation.
 * @details A delayed varied operation can be considered to be functionally
 * equivalent to a delayed operation, except that it contains multipled delayed
 * ones for which the operation is applied. The nominal delayed operation can be
 * accessed by `nominal()`, and a systematic variation by `["variation name"]`.
 */
template <typename T>
template <typename Bld>
class dataflow<T>::delayed<Bld>::varied : public systematic<delayed<Bld>> {

public:
  varied(delayed<Bld> &&nom);
  ~varied() = default;

  varied(varied &&) = default;
  varied &operator=(varied &&) = default;

  virtual void set_variation(const std::string &var_name,
                             delayed &&var) override;

  virtual delayed const &nominal() const override;
  virtual delayed const &variation(const std::string &var_name) const override;

  virtual bool has_variation(const std::string &var_name) const override;
  virtual std::set<std::string> list_variation_names() const override;

public:
  template <
      typename... Args, typename V = Bld,
      std::enable_if_t<ana::column::template is_evaluator_v<V>, bool> = false>
  auto vary(const std::string &var_name, Args &&...args) -> varied;

  template <
      typename... Args, typename V = Bld,
      std::enable_if_t<ana::column::template is_evaluator_v<V>, bool> = false>
  auto evaluate(Args &&...args) -> typename ana::dataflow<T>::template lazy<
      column::template evaluated_t<V>>::varied;

  template <typename... Nodes, typename V = Bld,
            std::enable_if_t<ana::selection::template is_applicator_v<V>,
                             bool> = false>
  auto apply(Nodes const &...columns) -> typename lazy<selection>::varied;

  template <
      typename... Nodes, typename V = Bld,
      std::enable_if_t<ana::aggregation::template is_booker_v<V>, bool> = false>
  auto fill(Nodes const &...columns) -> varied;

  template <
      typename Node, typename V = Bld,
      std::enable_if_t<ana::aggregation::template is_booker_v<V>, bool> = false>
  auto at(Node const &selection) ->
      typename lazy<aggregation::booked_t<V>>::varied;

  template <
      typename... Nodes, typename V = Bld,
      std::enable_if_t<ana::aggregation::template is_booker_v<V>, bool> = false>
  auto at(Nodes const &...selections) -> typename delayed<
      aggregation::bookkeeper<aggregation::booked_t<V>>>::varied;

  template <typename... Args>
  auto operator()(Args &&...args) ->
      typename lazy<typename decltype(std::declval<delayed<Bld>>().operator()(
          std::forward<Args>(args).nominal()...))::operation_type>::varied;

  /**
   * @brief Access the delayed operation of a specific systematic variation.
   * @param var_name The name of the systematic variation.
   */
  template <typename V = Bld,
            std::enable_if_t<ana::aggregation::template is_bookkeeper_v<V>,
                             bool> = false>
  auto operator[](const std::string &var_name) const -> delayed<V> const &;

protected:
  delayed<Bld> m_nominal;
  std::unordered_map<std::string, delayed<Bld>> m_variation_map;
  std::set<std::string> m_variation_names;
};

} // namespace ana

template <typename T>
template <typename Bld>
ana::dataflow<T>::delayed<Bld>::varied::varied(delayed<Bld> &&nom)
    : systematic<delayed<Bld>>::systematic(*nom.m_df),
      m_nominal(std::move(nom)) {}

template <typename T>
template <typename Bld>
void ana::dataflow<T>::delayed<Bld>::varied::set_variation(
    const std::string &var_name, delayed &&var) {
  m_variation_map.insert(std::move(std::make_pair(var_name, std::move(var))));
  m_variation_names.insert(var_name);
}

template <typename T>
template <typename Bld>
auto ana::dataflow<T>::delayed<Bld>::varied::nominal() const
    -> delayed const & {
  return m_nominal;
}

template <typename T>
template <typename Bld>
auto ana::dataflow<T>::delayed<Bld>::varied::variation(
    const std::string &var_name) const -> delayed const & {
  return (this->has_variation(var_name) ? m_variation_map.at(var_name)
                                        : m_nominal);
}

template <typename T>
template <typename Bld>
bool ana::dataflow<T>::delayed<Bld>::varied::has_variation(
    const std::string &var_name) const {
  return m_variation_map.find(var_name) != m_variation_map.end();
}

template <typename T>
template <typename Bld>
std::set<std::string>
ana::dataflow<T>::delayed<Bld>::varied::list_variation_names() const {
  return m_variation_names;
}

template <typename T>
template <typename Bld>
template <typename V,
          std::enable_if_t<ana::aggregation::template is_bookkeeper_v<V>, bool>>
auto ana::dataflow<T>::delayed<Bld>::varied::operator[](
    const std::string &var_name) const -> delayed<V> const & {
  if (!this->has_variation(var_name)) {
    throw std::out_of_range("variation does not exist");
  }
  return this->variation(var_name);
}

template <typename T>
template <typename Bld>
template <typename... Args, typename V,
          std::enable_if_t<ana::column::template is_evaluator_v<V>, bool>>
auto ana::dataflow<T>::delayed<Bld>::varied::evaluate(Args &&...args) ->
    typename ana::dataflow<T>::template lazy<
        column::template evaluated_t<V>>::varied {
  using varied_type = typename ana::dataflow<T>::template lazy<
      column::template evaluated_t<V>>::varied;
  auto syst = varied_type(
      this->nominal().evaluate(std::forward<Args>(args).nominal()...));
  for (auto const &var_name :
       list_all_variation_names(*this, std::forward<Args>(args)...)) {
    syst.set_variation(var_name,
                       variation(var_name).evaluate(
                           std::forward<Args>(args).variation(var_name)...));
  }
  return syst;
}

template <typename T>
template <typename Bld>
template <typename... Nodes, typename V,
          std::enable_if_t<ana::selection::template is_applicator_v<V>, bool>>
auto ana::dataflow<T>::delayed<Bld>::varied::apply(Nodes const &...columns) ->
    typename lazy<selection>::varied {

  using varied_type = typename lazy<selection>::varied;
  auto syst = varied_type(this->nominal().apply(columns.nominal()...));

  for (auto const &var_name : list_all_variation_names(*this, columns...)) {
    syst.set_variation(
        var_name, variation(var_name).apply(columns.variation(var_name)...));
  }

  return syst;
}

template <typename T>
template <typename Bld>
template <typename... Nodes, typename V,
          std::enable_if_t<ana::aggregation::template is_booker_v<V>, bool>>
auto ana::dataflow<T>::delayed<Bld>::varied::fill(Nodes const &...columns)
    -> varied {
  auto syst = varied(std::move(this->nominal().fill(columns.nominal()...)));
  for (auto const &var_name : list_all_variation_names(*this, columns...)) {
    syst.set_variation(var_name, std::move(variation(var_name).fill(
                                     columns.variation(var_name)...)));
  }
  return syst;
}

template <typename T>
template <typename Bld>
template <typename Node, typename V,
          std::enable_if_t<ana::aggregation::template is_booker_v<V>, bool>>
auto ana::dataflow<T>::delayed<Bld>::varied::at(Node const &selection) ->
    typename lazy<aggregation::booked_t<V>>::varied {
  using varied_type = typename lazy<aggregation::booked_t<V>>::varied;
  auto syst = varied_type(this->nominal().at(selection.nominal()));
  for (auto const &var_name : list_all_variation_names(*this, selection)) {
    syst.set_variation(var_name,
                       variation(var_name).at(selection.variation(var_name)));
  }
  return syst;
}

template <typename T>
template <typename Bld>
template <typename... Nodes, typename V,
          std::enable_if_t<ana::aggregation::template is_booker_v<V>, bool>>
auto ana::dataflow<T>::delayed<Bld>::varied::at(Nodes const &...selections) ->
    typename delayed<
        aggregation::bookkeeper<aggregation::booked_t<V>>>::varied {
  using varied_type = typename delayed<
      aggregation::bookkeeper<aggregation::booked_t<V>>>::varied;
  auto syst = varied_type(this->nominal().at(selections.nominal()...));
  for (auto const &var_name : list_all_variation_names(*this, selections...)) {
    syst.set_variation(
        var_name, variation(var_name).at(selections.variation(var_name)...));
  }
  return syst;
}

template <typename T>
template <typename Bld>
template <typename... Args, typename V,
          std::enable_if_t<ana::column::template is_evaluator_v<V>, bool>>
auto ana::dataflow<T>::delayed<Bld>::varied::vary(const std::string &var_name,
                                                  Args &&...args) -> varied {
  auto syst = varied(std::move(*this));
  syst.set_variation(var_name,
                     std::move(syst.m_df->vary_evaluator(
                         syst.nominal(), std::forward<Args>(args)...)));
  return syst;
}

template <typename T>
template <typename Bld>
template <typename... Args>
auto ana::dataflow<T>::delayed<Bld>::varied::operator()(Args &&...args) ->
    typename lazy<typename decltype(std::declval<delayed<Bld>>().operator()(
        std::forward<Args>(args).nominal()...))::operation_type>::varied {

  using varied_type =
      typename lazy<typename decltype(std::declval<delayed<Bld>>().operator()(
          std::forward<Args>(args).nominal()...))::operation_type>::varied;

  auto syst = varied_type(
      this->nominal().operator()(std::forward<Args>(args).nominal()...));
  for (auto const &var_name :
       list_all_variation_names(*this, std::forward<Args>(args)...)) {
    syst.set_variation(var_name,
                       variation(var_name).operator()(
                           std::forward<Args>(args).variation(var_name)...));
  }
  return syst;
}

namespace ana {

template <typename T> class aggregation::summary {

public:
  // version for lazy<aggregation>
  template <typename Res>
  void record(const std::string &selection_path,
              std::decay_t<Res> aggregation_result) {
    static_cast<T *>(this)->record(selection_path, aggregation_result);
  }

  // version for varied<aggregation>
  template <typename Res>
  void record(const std::string &variation_name,
              const std::string &selection_path,
              std::decay_t<Res> aggregation_result) {
    static_cast<T *>(this)->record(variation_name, selection_path,
                                   aggregation_result);
  }

  template <typename Dest> void output(Dest &destination) {
    static_cast<T *>(this)->output(destination);
  }
};

} // namespace ana

#include <utility>

namespace ana {

namespace output {

template <typename Sum, typename Node, typename Dest, typename... Args>
void dump(Node const &node, Dest &&dest, Args &&...args);

}

} // namespace ana

template <typename Sum, typename Node, typename Dest, typename... Args>
void ana::output::dump(Node const &node, Dest &&dest, Args &&...args) {
  // instantiate summary
  Sum summary(std::forward<Args>(args)...);

  // get selection paths
  auto selection_paths = node.nominal().get_model_value(
      [](typename Node::nominal_type const &node) {
        return node.list_selection_paths();
      });

  // record all results
  if constexpr (dataflow_t<Node>::template is_nominal_v<Node>) {
    // if node is nominal-only
    for (auto const &selection_path : selection_paths) {
      summary.record(selection_path, node[selection_path].result());
    }
  } else {
    // if it has variations
    for (auto const &selection_path : selection_paths) {
      summary.record(selection_path, node.nominal()[selection_path].result());
    }
    for (auto const &var_name : list_all_variation_names(node)) {
      for (auto const &selection_path : selection_paths) {
        summary.record(var_name, selection_path,
                       node[var_name][selection_path].result());
      }
    }
  }
  // dump all results to destination
  summary.output(std::forward<Dest>(dest));
}