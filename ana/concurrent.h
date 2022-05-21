#pragma once

#include <vector>
#include <memory>
#include <thread>
#include <algorithm>

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
  template <typename> friend class concurrent;

  using node_type = T;

public:
  concurrent() = default;
  ~concurrent() = default;

  template <typename U>
  concurrent(const concurrent<U>& other);
  template <typename U>
  concurrent& operator=(const concurrent<U>& other);

public:
  // main node = slot(0)
  std::shared_ptr<T> model() const;

  // add/get slots
  void add(std::shared_ptr<T> slot);
  std::shared_ptr<T> slot(size_t i) const;
  std::vector<std::shared_ptr<T>> slots() const;

  // downsize slots
  void clear();

  // downsize slots
  void downsize(size_t n);

  // number of slots
  size_t concurrency() const;

  // apply a method to all nodes
  template <typename F, typename... Args>
  void in_seq(F f, const concurrent<Args>&... args) const;

  // check common value of function call from all nodes
  template <typename F, typename... Args>
  auto check(F f, const Args&... args) const -> std::invoke_result_t<F,T&,const Args&...>;

  // return (concurrent) result of function call from all nodes
  template <typename F, typename... Args>
  auto invoke(F f, const concurrent<Args>&... args) const -> concurrent<typename std::invoke_result_t<F,T&,Args&...>::element_type>;

protected:
  std::vector<std::shared_ptr<T>> m_slots;

};

}

template <typename T>
template <typename U>
ana::concurrent<T>::concurrent(const concurrent<U>& other)
{
  this->m_slots.clear();
  for(size_t i=0 ; i<other.concurrency() ; ++i) {
    this->m_slots.push_back(other.slot(i));
  }
}

template <typename T>
template <typename U>
ana::concurrent<T>& ana::concurrent<T>::operator=(const concurrent<U>& other)
{
  this->m_slots.clear();
  for(size_t i=0 ; i<other.concurrency() ; ++i) {
    this->m_slots.push_back(other.slot(i));
  }
  return *this;
}

template <typename T>
void ana::concurrent<T>::add(std::shared_ptr<T> slot)
{
  m_slots.push_back(slot);
}

template <typename T>
void ana::concurrent<T>::clear()
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
std::shared_ptr<T> ana::concurrent<T>::model() const
{
  return m_slots[0];
}

template <typename T>
std::shared_ptr<T> ana::concurrent<T>::slot(size_t i) const
{
  return m_slots[i];
}

template <typename T>
size_t ana::concurrent<T>::concurrency() const
{
  return m_slots.size();
}

template <typename T>
template <typename F, typename... Args>
void ana::concurrent<T>::in_seq(F f, const concurrent<Args>&... args) const
{
  assert( ((concurrency()==args.concurrency())&&...) );
  for(size_t i=0 ; i<concurrency() ; ++i) {
    f(*slot(i),*args.slot(i)...);
  }
}

template <typename T>
template <typename F, typename... Args>
auto ana::concurrent<T>::check(F f, const Args&... args) const -> std::invoke_result_t<F,T&,const Args&...>
{
  auto result = f(*model(),args...);
  for(size_t i=1 ; i<concurrency() ; ++i) {
    assert(result==f(*slot(i),args...));
  }
  return result;
}

template <typename T>
template <typename F, typename... Args>
auto ana::concurrent<T>::invoke(F f, const concurrent<Args>&... args) const -> concurrent<typename std::invoke_result_t<F,T&,Args&...>::element_type>
{
  assert( ((concurrency()==args.concurrency())&&...) );
  concurrent<typename std::invoke_result_t<F,T&,Args&...>::element_type> invoked;
  for(size_t i=0 ; i<concurrency() ; ++i) {
    invoked.add(f(*slot(i),*args.slot(i)...));
  }
  return invoked;
}