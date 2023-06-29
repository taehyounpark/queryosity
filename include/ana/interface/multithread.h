#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

namespace ana {

template <typename T> class lockstep;

class multithread {
public:
  static inline int s_suggestion = 0;
  static void enable(int suggestion = -1);
  static void disable();
  static bool status();
  static unsigned int concurrency();
};

template <typename T> class concurrent {

public:
  using model_type = T;

  template <typename> friend class dataflow;
  template <typename> friend class concurrent;
  template <typename> friend class lockstep;

public:
  concurrent() = default;
  ~concurrent() = default;

  concurrent(const concurrent &) = delete;
  concurrent &operator=(const concurrent &) = delete;

  concurrent(concurrent &&) = default;
  concurrent &operator=(concurrent &&) = default;

  template <typename U> concurrent(concurrent<U> &&derived);
  template <typename U> concurrent &operator=(concurrent<U> &&derived);

public:
  void set_model(std::unique_ptr<T> model);
  void add_slot(std::unique_ptr<T> slot);
  void clear_slots();

  T *get_model() const;
  T *get_slot(size_t i) const;

  size_t concurrency() const;

  /**
   * @brief Implicit conversion to lockstep (relinquish ownership).
   */
  operator lockstep<T>() const;

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
  void call_all(Fn const &fn, lockstep<Args> const &...args) const;

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
  auto get_concurrent_result(Fn const &fn, lockstep<Args> const &...args) const
      -> concurrent<
          typename std::invoke_result_t<Fn, T &, Args &...>::element_type>;

  template <typename Fn, typename... Args>
  auto get_lockstep_view(Fn const &fn, lockstep<Args> const &...args) const
      -> lockstep<std::remove_pointer_t<
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
  void run_slots(Fn const &fn, lockstep<Args> const &...args) const;

protected:
  std::unique_ptr<T> m_model;
  std::vector<std::unique_ptr<T>> m_slots;
};

template <typename T> class lockstep {

public:
  using model_type = T;

  template <typename> friend class lockstep;
  template <typename> friend class concurrent;

public:
  lockstep() = default;
  ~lockstep() = default;

  lockstep(const lockstep &other) = default;
  lockstep &operator=(const lockstep &other) = default;

  template <typename U> lockstep(const concurrent<U> &derived);
  template <typename U> lockstep &operator=(const concurrent<U> &derived);

public:
  T *get_model() const;
  T *get_slot(size_t i) const;

  size_t concurrency() const;

  void clear_slots();

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
   * @param args `lockstep<Args>` that holds an `Args const&` applied as the
   * argument for each slot call.
   * @details Call method(s) on all held underlying objects.
   */
  template <typename Fn, typename... Args>
  void call_all(Fn const &fn, lockstep<Args> const &...args) const;

  /**
   * @brief Get the result of calling method(s) on each underlying model and
   * slots.
   * @param fn Function to be called. The first argument must accept the slot
   * type by `&`, and return a `std::unique_ptr<Result>`.
   * @param args (Optional) arguments that are applied per-slot to fn.
   * @return result `lockstep<Result>` where `std::unique_ptr<Result>` is
   * returned by fn.
   * @details Preserve the concurrency of result of operations performed on the
   * underlying objects.
   */
  template <typename Fn, typename... Args>
  auto get_concurrent_result(Fn const &fn, lockstep<Args> const &...args) const
      -> concurrent<
          typename std::invoke_result_t<Fn, T &, Args &...>::element_type>;

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
ana::concurrent<T>::concurrent(concurrent<U> &&derived) {
  static_assert(std::is_base_of_v<T, U>, "incompatible concurrent types");
  this->m_model = std::move(derived.m_model);
  this->m_slots.clear();
  for (size_t i = 0; i < derived.concurrency(); ++i) {
    this->m_slots.emplace_back(std::move(derived.m_slots[i]));
  }
}

template <typename T>
template <typename U>
ana::concurrent<T> &ana::concurrent<T>::operator=(concurrent<U> &&derived) {
  this->m_model = derived.m_model;
  this->m_slots.clear();
  for (size_t i = 0; i < derived.concurrency(); ++i) {
    this->m_slots.emplace_back(std::move(derived.m_slots[i]));
  }
  return *this;
}

template <typename T>
void ana::concurrent<T>::add_slot(std::unique_ptr<T> slot) {
  this->m_slots.emplace_back(std::move(slot));
}

template <typename T> void ana::concurrent<T>::clear_slots() {
  this->m_slots.clear();
}

template <typename T>
void ana::concurrent<T>::set_model(std::unique_ptr<T> model) {
  this->m_model = std::move(model);
}

template <typename T> T *ana::concurrent<T>::get_model() const {
  return this->m_model.get();
}

template <typename T> T *ana::concurrent<T>::get_slot(size_t i) const {
  return this->m_slots.at(i).get();
}

template <typename T> size_t ana::concurrent<T>::concurrency() const {
  return this->m_slots.size();
}

template <typename T> ana::concurrent<T>::operator lockstep<T>() const {
  return lockstep<T>(static_cast<concurrent<T> const &>(*this));
}

template <typename T>
template <typename Fn, typename... Args>
auto ana::concurrent<T>::get_model_value(Fn const &fn,
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
auto ana::concurrent<T>::get_concurrent_result(
    Fn const &fn, lockstep<Args> const &...args) const
    -> concurrent<
        typename std::invoke_result_t<Fn, T &, Args &...>::element_type> {
  assert(((concurrency() == args.concurrency()) && ...));
  concurrent<typename std::invoke_result_t<Fn, T &, Args &...>::element_type>
      invoked;
  invoked.m_model = std::move(fn(*this->get_model(), *args.get_model()...));
  for (size_t i = 0; i < concurrency(); ++i) {
    invoked.m_slots.emplace_back(fn(*this->get_slot(i), *args.get_slot(i)...));
  }
  return invoked;
}

template <typename T>
template <typename Fn, typename... Args>
auto ana::concurrent<T>::get_lockstep_view(Fn const &fn,
                                           lockstep<Args> const &...args) const
    -> lockstep<std::remove_pointer_t<
        typename std::invoke_result_t<Fn, T &, Args &...>>> {
  assert(((concurrency() == args.concurrency()) && ...));
  lockstep<
      std::remove_pointer_t<typename std::invoke_result_t<Fn, T &, Args &...>>>
      invoked;
  invoked.m_model = fn(*this->get_model(), *args.get_model()...);
  for (size_t i = 0; i < concurrency(); ++i) {
    invoked.m_slots.push_back(fn(*this->get_slot(i), *args.get_slot(i)...));
  }
  return invoked;
}

template <typename T>
template <typename Fn, typename... Args>
void ana::concurrent<T>::call_all(Fn const &fn,
                                  lockstep<Args> const &...args) const {
  assert(((concurrency() == args.concurrency()) && ...));
  fn(*this->get_model(), *args.get_model()...);
  for (size_t i = 0; i < concurrency(); ++i) {
    fn(*this->get_slot(i), *args.get_slot(i)...);
  }
}

template <typename T>
template <typename Fn, typename... Args>
void ana::concurrent<T>::run_slots(Fn const &fn,
                                   lockstep<Args> const &...args) const {
  assert(((concurrency() == args.concurrency()) && ...));

  // multi-threaded
  if (multithread::status()) {
    std::vector<std::thread> pool;
    for (size_t islot = 0; islot < this->concurrency(); ++islot) {
      pool.emplace_back(fn, std::ref(*this->get_slot(islot)),
                        std::ref(*args.get_slot(islot))...);
    }
    for (auto &&thread : pool) {
      thread.join();
    }
    // single-threaded
  } else {
    for (size_t islot = 0; islot < this->concurrency(); ++islot) {
      fn(*this->get_slot(islot), *args.get_slot(islot)...);
    }
  }
}

template <typename T>
template <typename U>
ana::lockstep<T>::lockstep(const ana::concurrent<U> &derived) {
  static_assert(std::is_base_of_v<T, U>, "incompatible slot types");
  this->m_model = derived.m_model.get();
  this->m_slots.clear();
  for (size_t i = 0; i < derived.concurrency(); ++i) {
    this->m_slots.push_back(derived.m_slots[i].get());
  }
}

template <typename T>
template <typename U>
ana::lockstep<T> &
ana::lockstep<T>::operator=(const ana::concurrent<U> &derived) {
  this->m_model = derived.m_model.get();
  this->m_slots.clear();
  for (size_t i = 0; i < derived.concurrency(); ++i) {
    this->m_slots.push_back(derived.m_slots[i].get());
  }
  return *this;
}

template <typename T> void ana::lockstep<T>::clear_slots() {
  this->m_slots.clear();
}

template <typename T> T *ana::lockstep<T>::get_model() const {
  return this->m_model;
}

template <typename T> T *ana::lockstep<T>::get_slot(size_t i) const {
  return this->m_slots.at(i);
}

template <typename T> size_t ana::lockstep<T>::concurrency() const {
  return this->m_slots.size();
}

template <typename T>
template <typename Fn, typename... Args>
auto ana::lockstep<T>::get_model_value(Fn const &fn, Args const &...args) const
    -> std::invoke_result_t<Fn, T const &, Args const &...> {
  auto result = fn(static_cast<const T &>(*this->get_model()), args...);
  // result at each slot must match the model's
  for (size_t i = 0; i < concurrency(); ++i) {
    assert(result == fn(static_cast<const T &>(*this->get_slot(i)), args...));
  }
  return result;
}