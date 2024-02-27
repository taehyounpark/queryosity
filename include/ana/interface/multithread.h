#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <thread>
#include <type_traits>
#include <vector>

namespace ana {

namespace concurrent {

template <typename T> class slotted {
public:
  slotted() = default;
  virtual ~slotted() = default;

public:
  virtual T *get_slot(unsigned int i) const = 0;
  virtual unsigned int concurrency() const = 0;
  std::vector<T *> get_slots() const;
};

template <typename T, typename... Args>
unsigned int check(std::vector<T> const &first,
                   std::vector<Args> const &...args);

template <typename Fn, typename... Args>
void call(Fn const &fn, std::vector<Args> const &...args);

template <typename Fn, typename... Args>
auto invoke(Fn const &fn, std::vector<Args> const &...args)
    -> std::vector<typename std::invoke_result_t<Fn, Args...>>;

} // namespace concurrent

namespace multithread {

class core {

public:
  core(int suggestion);
  ~core() = default;

  bool is_enabled() const;
  unsigned int concurrency() const;

  /**
   * @brief Run the function on the underlying slots, multi-threading if
   * enabled.
   * @param fn Function to be called.
   * @param args (Optional) arguments applied per-slot to function.
   * @details The methods are called on each slot, and the model is left
   * untouched.
   */
  template <typename Fn, typename... Args>
  void run(Fn const &fn, std::vector<Args> const &...args) const;

protected:
  int m_suggestion;
};

core enable(int suggestion = -1);
core disable();

} // namespace multithread

} // namespace ana

inline ana::multithread::core::core(int suggestion)
    : m_suggestion(suggestion) {}

inline ana::multithread::core ana::multithread::enable(int suggestion) {
  return core(suggestion);
}

inline ana::multithread::core ana::multithread::disable() {
  return core(false);
}

inline bool ana::multithread::core::is_enabled() const { return m_suggestion; }

inline unsigned int ana::multithread::core::concurrency() const {
  if (!m_suggestion)
    return 1;
  if (m_suggestion < 0)
    return std::thread::hardware_concurrency();
  else
    return std::min<unsigned int>(std::thread::hardware_concurrency(),
                                  m_suggestion);
}

template <typename Fn, typename... Args>
void ana::multithread::core::run(Fn const &fn,
                                 std::vector<Args> const &...args) const {
  auto nslots = concurrent::check(args...);

  if (m_suggestion) {
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
inline unsigned int ana::concurrent::check(std::vector<T> const &first,
                                           std::vector<Args> const &...args) {
  assert(((first.size() == args.size()) && ...));
  return first.size();
}

template <typename Fn, typename... Args>
inline void ana::concurrent::call(Fn const &fn,
                                  std::vector<Args> const &...args) {
  auto nslots = check(args...);
  // result at each slot must match the model
  for (size_t i = 0; i < nslots; ++i) {
    fn(args.at(i)...);
  }
}

template <typename Fn, typename... Args>
inline auto ana::concurrent::invoke(Fn const &fn,
                                    std::vector<Args> const &...args)
    -> std::vector<typename std::invoke_result_t<Fn, Args...>> {
  auto nslots = check(args...);
  typename std::vector<typename std::invoke_result_t<Fn, Args...>> invoked;
  // invoked.set_model(std::move(fn(args.get_model()...)));
  invoked.reserve(nslots);
  for (size_t i = 0; i < nslots; ++i) {
    invoked.push_back(std::move((fn(args.at(i)...))));
  }
  return invoked;
}

template <typename T>
std::vector<T *> ana::concurrent::slotted<T>::get_slots() const {
  std::vector<T *> slots;
  for (unsigned int i = 0; i < this->concurrency(); ++i) {
    slots.push_back(this->get_slot(i));
  }
  return slots;
}