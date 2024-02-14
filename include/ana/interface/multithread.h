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

struct core {
  bool enabled;
  unsigned int concurrency;
  /**
   * @brief Run the function on the underlying slots, multi-threading if
   * enabled.
   * @param fn Function to be called. The first argument must accept the slot
   * type by `&`.
   * @param args (Optional) arguments applied per-slot to function.
   * @details The methods are called on each slot, and the model is left
   * untouched.
   */
  template <typename Fn, typename... Nodes>
  void run(Fn const &fn, Nodes const &...args) const;
};

core enable(int suggestion = -1);
core disable();

}; // namespace multithread

namespace lockstep {

template <typename T> class slotted;
template <typename T> class node;
template <typename T> class view;

unsigned int get_concurrency();

template <typename T, typename... Args>
unsigned int get_concurrency(T const &first, Args const &...args);

template <typename Fn, typename... Args>
void call_slots(Fn const &fn, slotted<Args> const &...args);

template <typename Fn, typename... Args>
auto get_value(Fn const &fn, slotted<Args> const &...args)
    -> std::invoke_result_t<Fn, Args *...>;

template <typename Fn, typename... Args>
auto get_node(Fn const &fn, slotted<Args> const &...args) -> lockstep::node<
    typename std::invoke_result_t<Fn, Args *...>::element_type>;

template <typename Fn, typename... Args>
auto get_view(Fn const &fn, slotted<Args> const &...args) -> lockstep::view<
    std::remove_pointer_t<typename std::invoke_result_t<Fn, Args *...>>>;

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

protected:
  std::unique_ptr<T> m_model;
  std::vector<std::unique_ptr<T>> m_slots;
};

template <typename T> class lockstep::view : public lockstep::slotted<T> {

public:
  friend class ana::dataflow;
  template <typename> friend class node;
  template <typename> friend class view;

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

protected:
  T *m_model;
  std::vector<T *> m_slots;
};

} // namespace ana

inline ana::multithread::core ana::multithread::enable(int suggestion) {
  return suggestion ? multithread::disable()
                    : core{true, suggestion < 0
                                     ? std::thread::hardware_concurrency()
                                     : std::min<unsigned int>(
                                           std::thread::hardware_concurrency(),
                                           suggestion)};
}

inline ana::multithread::core ana::multithread::disable() {
  return core{false, 1};
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
template <typename Fn, typename... Args>
void ana::lockstep::node<T>::call_all_slots(Fn const &fn,
                                            view<Args> const &...args) const {
  assert(((concurrency() == args.concurrency()) && ...));
  fn(this->get_model(), args.get_model()...);
  for (size_t i = 0; i < concurrency(); ++i) {
    fn(this->get_slot(i), args.get_slot(i)...);
  }
}

template <typename Fn, typename... Nodes>
void ana::multithread::core::run(Fn const &fn, Nodes const &...args) const {
  assert(((this->concurrency == args.concurrency()) && ...));

  // multi-lockstep
  if (this->enabled) {
    std::vector<std::thread> pool;
    pool.reserve(this->concurrency);
    for (size_t islot = 0; islot < this->concurrency; ++islot) {
      pool.emplace_back(fn, args.get_slot(islot)...);
    }
    for (auto &&thread : pool) {
      thread.join();
    }
    // single-lockstep
  } else {
    for (size_t islot = 0; islot < this->concurrency; ++islot) {
      fn(args.get_slot(islot)...);
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

template <typename T>
template <typename U>
ana::lockstep::view<T>::view(const ana::lockstep::view<U> &derived) {
  static_assert(std::is_base_of_v<T, U>, "incompatible slot types");
  this->m_model = derived.m_model;
  this->m_slots.clear();
  this->m_slots.reserve(derived.concurrency());
  for (size_t i = 0; i < derived.concurrency(); ++i) {
    this->m_slots.push_back(derived.m_slots[i]);
  }
}

template <typename T>
template <typename U>
ana::lockstep::view<T> &
ana::lockstep::view<T>::operator=(const ana::lockstep::view<U> &derived) {
  this->m_model = derived.m_model;
  this->m_slots.clear();
  this->m_slots.reserve(derived.concurrency());
  for (size_t i = 0; i < derived.concurrency(); ++i) {
    this->m_slots.push_back(derived.m_slots[i]);
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

template <typename T, typename... Args>
inline unsigned int ana::lockstep::get_concurrency(T const &first,
                                                   Args const &...args) {
  assert(((first.concurrency() == args.concurrency()) && ...));
  return first.concurrency();
}

template <typename Fn, typename... Args>
inline void ana::lockstep::call_slots(Fn const &fn,
                                      slotted<Args> const &...args) {
  auto concurrency = get_concurrency(args...);
  // result at each slot must match the model
  fn(args.get_model()...);
  for (size_t i = 0; i < concurrency; ++i) {
    fn(args.get_slot(i)...);
  }
}

template <typename Fn, typename... Args>
inline auto ana::lockstep::get_value(Fn const &fn, slotted<Args> const &...args)
    -> std::invoke_result_t<Fn, Args *...> {
  auto concurrency = get_concurrency(args...);
  // result at each slot must match the model
  auto result = fn(args.get_model()...);
  for (size_t i = 0; i < concurrency; ++i) {
    assert(result == fn(args.get_slot(i)...));
  }
  return result;
}

template <typename Fn, typename... Args>
inline auto ana::lockstep::get_node(Fn const &fn, slotted<Args> const &...args)
    -> lockstep::node<
        typename std::invoke_result_t<Fn, Args *...>::element_type> {
  auto concurrency = get_concurrency(args...);
  typename lockstep::node<
      typename std::invoke_result_t<Fn, Args *...>::element_type>
      invoked;
  invoked.set_model(std::move(fn(args.get_model()...)));
  invoked.reserve(concurrency);
  for (size_t i = 0; i < concurrency; ++i) {
    invoked.add_slot(std::move((fn(args.get_slot(i)...))));
  }
  return invoked;
}

template <typename Fn, typename... Args>
inline auto ana::lockstep::get_view(Fn const &fn,
                                    ana::lockstep::slotted<Args> const &...args)
    -> typename lockstep::view<
        std::remove_pointer_t<typename std::invoke_result_t<Fn, Args *...>>> {
  auto concurrency = get_concurrency(args...);
  typename lockstep::view<
      std::remove_pointer_t<typename std::invoke_result_t<Fn, Args *...>>>
      invoked;
  invoked.set_model(fn(args.get_model()...));
  invoked.reserve(concurrency);
  for (size_t i = 0; i < concurrency; ++i) {
    invoked.add_slot(fn(args.get_slot(i)...));
  }
  return invoked;
}