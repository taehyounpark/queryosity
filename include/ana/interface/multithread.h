#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <thread>
#include <type_traits>
#include <vector>

namespace ana {

class dataflow;

namespace multithread {

struct configuration {
  bool enabled;
  unsigned int concurrency;
};

configuration enable(int suggestion = -1);
configuration disable();

}; // namespace multithread

namespace lockstep {

template <typename T> class slotted;
template <typename T> class node;
template <typename T> class view;

unsigned int check_concurrency();

template <typename T, typename... Args>
unsigned int check_concurrency(T const &first, Args const &...args);

template <typename Fn, typename... Args>
auto check_value(Fn const &fn, view<Args> const &...args)
    -> std::invoke_result_t<Fn, Args &...>;

template <typename Fn, typename... Args>
auto invoke_node(Fn const &fn, view<Args> const &...args) -> lockstep::node<
    typename std::invoke_result_t<Fn, Args &...>::element_type>;

template <typename Fn, typename... Args>
auto invoke_view(Fn const &fn, view<Args> const &...args) -> lockstep::view<
    std::remove_pointer_t<typename std::invoke_result_t<Fn, Args &...>>>;

template <typename Fn, typename... Args>
auto run_parallel(multithread::configuration const &mtcfg, Fn const &fn,
                  view<Args> const &...args)
    -> lockstep::view<
        std::remove_pointer_t<typename std::invoke_result_t<Fn, Args &...>>>;

} // namespace lockstep

template <typename T> class lockstep::slotted {

public:
  virtual T *get_model() const = 0;
  virtual T *get_slot(size_t i) const = 0;
  virtual size_t concurrency() const = 0;
};

template <typename T> class lockstep::node : public lockstep::slotted<T> {

public:
  friend class ana::dataflow;
  template <typename> friend class node;
  template <typename> friend class view;

public:
  node() = default;
  ~node() = default;

  template <typename... Args> node(size_t n, Args &&...args);

  node(const node &) = delete;
  node &operator=(const node &) = delete;

  node(node &&) = default;
  node &operator=(node &&) = default;

  template <typename U> node(node<U> &&derived);
  template <typename U> node &operator=(node<U> &&derived);

  void set_model(std::unique_ptr<T> model);
  void add_slot(std::unique_ptr<T> slot);
  void clear_slots();

  void reserve(size_t n) { this->m_slots.reserve(n); }

  virtual T *get_model() const override;
  virtual T *get_slot(size_t i) const override;
  virtual size_t concurrency() const override;

  view<T> get_view() const;

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
  void run_slots(multithread::configuration const &mtcfg, Fn const &fn,
                 view<Args> const &...args) const;

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

  void set_model(T *model) { this->m_model = model; }
  void add_slot(T *slot) { this->m_slots.push_back(slot); };

  void reserve(size_t n) { this->m_slots.reserve(n); }
  void clear_slots();

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

inline ana::multithread::configuration
ana::multithread::enable(int suggestion) {
  return suggestion ? multithread::disable()
                    : configuration{
                          true, suggestion < 0
                                    ? std::thread::hardware_concurrency()
                                    : std::min<unsigned int>(
                                          std::thread::hardware_concurrency(),
                                          suggestion)};
}

inline ana::multithread::configuration ana::multithread::disable() {
  return configuration{false, 1};
}

template <typename T>
template <typename... Args>
ana::lockstep::node<T>::node(size_t concurrency, Args &&...args) {
  this->set_model(std::make_unique<T>(std::forward<Args>(args)...));
  this->reserve(concurrency);
  for (unsigned int islot = 0; islot < concurrency; ++islot) {
    this->add_slot(std::make_unique<T>(std::forward<Args>(args)...));
  }
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
  this->m_model = std::move(derived.m_model);
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

template <typename T>
ana::lockstep::view<T> ana::lockstep::node<T>::get_view() const {
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
void ana::lockstep::node<T>::run_slots(
    ana::multithread::configuration const &mtcfg, Fn const &fn,
    view<Args> const &...args) const {
  assert(((concurrency() == args.concurrency()) && ...));

  // multi-lockstep
  if (mtcfg.enabled) {
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
  this->m_slots.reserve(derived.concurrency());
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
  this->m_slots.reserve(derived.concurrency());
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

inline unsigned int ana::lockstep::check_concurrency() { return 0; }

template <typename T, typename... Args>
inline unsigned int ana::lockstep::check_concurrency(T const &first,
                                                     Args const &...args) {
  assert(((first.concurrency() == args.concurrency()) && ...));
  return first.concurrency();
}

template <typename Fn, typename... Args>
inline auto ana::lockstep::check_value(Fn const &fn, view<Args> const &...args)
    -> std::invoke_result_t<Fn, Args &...> {
  auto concurrency = check_concurrency(args...);
  // result at each slot must match the model
  auto result = fn(std::cref(*args.get_model())...);
  for (size_t i = 0; i < concurrency; ++i) {
    assert(result == fn(std::cref(*args.get_slot(i))...));
  }
  return result;
}

template <typename Fn, typename... Args>
inline auto ana::lockstep::invoke_node(Fn const &fn, view<Args> const &...args)
    -> lockstep::node<
        typename std::invoke_result_t<Fn, Args &...>::element_type> {
  auto concurrency = check_concurrency(args...);
  typename lockstep::node<
      typename std::invoke_result_t<Fn, Args &...>::element_type>
      invoked;
  invoked.set_model(std::move(fn(*args.get_model()...)));
  invoked.reserve(concurrency);
  for (size_t i = 0; i < concurrency; ++i) {
    invoked.add_slot(std::move((fn(*args.get_slot(i)...))));
  }
  return invoked;
}

template <typename Fn, typename... Args>
inline auto ana::lockstep::invoke_view(Fn const &fn, view<Args> const &...args)
    -> typename lockstep::view<
        std::remove_pointer_t<typename std::invoke_result_t<Fn, Args &...>>> {
  auto concurrency = check_concurrency(args...);
  typename lockstep::view<
      std::remove_pointer_t<typename std::invoke_result_t<Fn, Args &...>>>
      invoked;
  invoked.set_model(fn(*args.get_model()...));
  invoked.reserve(concurrency);
  for (size_t i = 0; i < concurrency; ++i) {
    invoked.add_slot(fn(*args.get_slot(i)...));
  }
  return invoked;
}