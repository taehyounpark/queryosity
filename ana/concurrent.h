#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <algorithm>
#include <functional>

namespace ana
{

struct multithread
{
  static int s_suggestion;
  static void enable(int suggestion = -1);
  static void disable();
  static bool status();
  static unsigned int concurrency();
};

template <typename T>
class concurrent
{

public:
  using model_type = T;

  template <typename> friend class concurrent;

public:
  concurrent() = default;
  ~concurrent() = default;

  concurrent(const concurrent& other) = default;
  concurrent& operator=(const concurrent& other) = default;

  template <typename U>
  concurrent(const concurrent<U>& derived);
  template <typename U>
  concurrent& operator=(const concurrent<U>& derived);

public:
  void set_model(std::shared_ptr<T> model);
  std::shared_ptr<T> get_model() const;

  void add_slot(std::shared_ptr<T> slot);
  std::shared_ptr<T> get_slot(size_t i) const;
  void clear_slots();

  // downsize slots
  void downsize(size_t n);

  // number of slots
  size_t concurrency() const;

  /**
   * @brief Get the result of calling method(s) on the model.
   * @param fn Function to be called. The first argument must accept the slot type by `const&`.
   * @param args Arguments to be applied for all slots to fn.
   * @return result The value of the result from model.
   * @details The result *must* be equal by value between the model and all slots.
  */
  template <typename Fn, typename... Args>
  auto get_model_value(Fn const& fn, Args const&... args) const -> std::invoke_result_t<Fn,T const&,Args const&...>;

  /**
   * @brief Get the result of calling method(s) on each underlying model and slots.
   * @param fn Function to be called. The first argument must accept the slot type by reference.
   * @param args `concurrent<Args>` that holds an `Args const&` applied as the argument for each slot call.
   * @details Call method(s) on all held underlying objects.
  */
  template <typename Fn, typename... Args>
  void call_all(Fn const& fn, concurrent<Args> const&... args) const;

  /**
   * @brief Get the result of calling method(s) on each underlying model and slots.
   * @param fn Function to be called. The first argument must accept the slot type by `&`, and return a `std::shared_ptr<Result>`.
   * @param args (Optional) arguments that are applied per-slot to fn.
   * @return result `concurrent<Result>` where `std::shared_ptr<Result>` is returned by fn.
   * @details Preserve the concurrency of result of operations performed on the underlying objects.
  */
  template <typename Fn, typename... Args>
  auto get_concurrent_result(Fn const& fn, concurrent<Args> const&... args) const -> concurrent<typename std::invoke_result_t<Fn,T&,Args&...>::element_type>;

  /**
   * @brief Run the function on the underlying slots, multi-threading if enabled.
   * @param fn Function to be called. The first argument must accept the slot type by `&`.
   * @param args (Optional) arguments that are applied per-slot to fn.
   * @details The methods are called on slots, while the model is left untouched (as it is meant to represent the "merged" instance of all slots).
  */
  template <typename Fn, typename... Args>
  void run_slots(Fn const& fn, concurrent<Args> const&... args) const;

protected:
  std::shared_ptr<T> m_model;
  std::vector<std::shared_ptr<T>> m_slots;

};

}

inline int ana::multithread::s_suggestion = 0;

inline void ana::multithread::enable(int suggestion)
{
  s_suggestion = suggestion;
}

inline void ana::multithread::disable()
{
  s_suggestion = 0;
}

inline bool ana::multithread::status()
{
  return s_suggestion == 0 ? false : true;
}

inline unsigned int ana::multithread::concurrency()
{
  return std::max<unsigned int>( 1, s_suggestion < 0 ? std::thread::hardware_concurrency() :  s_suggestion );
}

template <typename T>
template <typename U>
ana::concurrent<T>::concurrent(const concurrent<U>& derived)
{
  static_assert( std::is_base_of_v<T,U>, "incompatible concurrent types" );
  this->m_model = derived.m_model;
  this->m_slots.clear();
  for(size_t i=0 ; i<derived.concurrency() ; ++i) {
    this->m_slots.push_back(derived.get_slot(i));
  }
}

template <typename T>
template <typename U>
ana::concurrent<T>& ana::concurrent<T>::operator=(const concurrent<U>& derived)
{
  this->m_model = derived.m_model;
  this->m_slots.clear();
  for(size_t i=0 ; i<derived.concurrency() ; ++i) {
    this->m_slots.push_back(derived.get_slot(i));
  }
  return *this;
}

template <typename T>
void ana::concurrent<T>::add_slot(std::shared_ptr<T> slot)
{
  m_slots.push_back(slot);
}

template <typename T>
void ana::concurrent<T>::clear_slots()
{
  m_slots.clear();
}

template <typename T>
void ana::concurrent<T>::downsize(size_t n)
{
  assert(n <= this->concurrency());
  m_slots.resize(n);
}

template <typename T>
void ana::concurrent<T>::set_model(std::shared_ptr<T> model)
{
  m_model = model;
}

template <typename T>
std::shared_ptr<T> ana::concurrent<T>::get_model() const
{
  return m_model;
}

template <typename T>
std::shared_ptr<T> ana::concurrent<T>::get_slot(size_t i) const
{
  return m_slots.at(i);
}

template <typename T>
size_t ana::concurrent<T>::concurrency() const
{
  return m_slots.size();
}

template <typename T>
template <typename Fn, typename... Args>
auto ana::concurrent<T>::get_model_value(Fn const& fn, Args const&... args) const -> std::invoke_result_t<Fn,T const&,Args const&...>
{
  auto result = fn(static_cast<const T&>(*this->get_model()),args...);
  // result at each slot must match the model's
  for(size_t i=0 ; i<concurrency() ; ++i) {
    assert(result==fn(static_cast<const T&>(*this->get_slot(i)),args...));
  }
  return result;
}

template <typename T>
template <typename Fn, typename... Args>
auto ana::concurrent<T>::get_concurrent_result(Fn const& fn, concurrent<Args> const&... args) const -> concurrent<typename std::invoke_result_t<Fn,T&,Args&...>::element_type>
{
  assert( ((concurrency()==args.concurrency())&&...) );
  concurrent<typename std::invoke_result_t<Fn,T&,Args&...>::element_type> invoked;
  invoked.set_model(fn(*this->get_model(),*args.get_model()...));
  for(size_t i=0 ; i<concurrency() ; ++i) {
    invoked.add_slot(fn(*this->get_slot(i),*args.get_slot(i)...));
  }
  return invoked;
}

template <typename T>
template <typename Fn, typename... Args>
void ana::concurrent<T>::call_all(Fn const& fn, concurrent<Args> const&... args) const
{
  assert( ((concurrency()==args.concurrency())&&...) );
  fn(*this->get_model(),*args.get_model()...);
  for(size_t i=0 ; i<concurrency() ; ++i) {
    fn(*this->get_slot(i),*args.get_slot(i)...);
  }
}

template <typename T>
template <typename Fn, typename... Args>
void ana::concurrent<T>::run_slots(Fn const& fn, concurrent<Args> const&... args) const
{
  assert( ((concurrency()==args.concurrency())&&...) );

  // multi-threaded
  if (multithread::status()) {
		std::vector<std::thread> pool;
		for (size_t islot=0 ; islot<this->concurrency() ; ++islot) {
			pool.emplace_back(
				fn,
				std::ref(*this->get_slot(islot)),
        std::ref(*args.get_slot(islot))...
			);
		}
		for (auto&& thread : pool) {
			thread.join();
		}
	// single-threaded
	} else {
		for (size_t islot=0 ; islot<this->concurrency() ; ++islot) {
			fn(*this->get_slot(islot), *args.get_slot(islot)...);
		}
	}
}