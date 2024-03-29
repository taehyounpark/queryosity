#pragma once

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
  (args.size(), ...);  // suppress GCC unused parameter warnings
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