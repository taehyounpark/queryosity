#pragma once

namespace ana {

template <typename T> class concurrent;

template <typename T> class lockstep {

public:
  using model_type = T;

  template <typename> friend class lockstep;

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
   * type by `&`, and return a `std::shared_ptr<Result>`.
   * @param args (Optional) arguments that are applied per-slot to fn.
   * @return result `lockstep<Result>` where `std::shared_ptr<Result>` is
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

#include "concurrent.h"

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

template <typename T>
template <typename Fn, typename... Args>
auto ana::lockstep<T>::get_concurrent_result(
    Fn const &fn, lockstep<Args> const &...args) const
    -> concurrent<
        typename std::invoke_result_t<Fn, T &, Args &...>::element_type> {
  assert(((concurrency() == args.concurrency()) && ...));
  concurrent<typename std::invoke_result_t<Fn, T &, Args &...>::element_type>
      invoked;
  invoked.set_model(fn(*this->get_model(), *args.get_model()...));
  for (size_t i = 0; i < concurrency(); ++i) {
    invoked.add_slot(fn(*this->get_slot(i), *args.get_slot(i)...));
  }
  return invoked;
}

template <typename T>
template <typename Fn, typename... Args>
void ana::lockstep<T>::call_all(Fn const &fn,
                                lockstep<Args> const &...args) const {
  assert(((concurrency() == args.concurrency()) && ...));
  fn(*this->get_model(), *args.get_model()...);
  for (size_t i = 0; i < concurrency(); ++i) {
    fn(*this->get_slot(i), *args.get_slot(i)...);
  }
}