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

  template <typename U>
  concurrent(const concurrent<U>& other);
  template <typename U>
  concurrent& operator=(const concurrent<U>& other);

public:
  // main lazy = slot(0)
  std::shared_ptr<T> model() const;

  // add/get slots
  void add_slot(std::shared_ptr<T> slot);
  std::shared_ptr<T> get_slot(size_t i) const;
  std::vector<std::shared_ptr<T>> slots() const;

  // downsize slots
  void clear();

  // downsize slots
  void downsize(size_t n);

  // number of slots
  size_t concurrency() const;

  // apply a method to all lazys
  template <typename Lmbd, typename... Args>
  void to_slots(Lmbd const& lmbd, const concurrent<Args>&... args) const;

  // check common value of function call from all lazys
  template <typename Lmbd, typename... Args>
  auto from_model(Lmbd const& lmbd, const Args&... args) const -> std::invoke_result_t<Lmbd,T&,const Args&...>;

  // return (concurrent) result of function call from all lazys
  template <typename Lmbd, typename... Args>
  auto from_slots(Lmbd const& lmbd, const concurrent<Args>&... args) const -> concurrent<typename std::invoke_result_t<Lmbd,T&,Args&...>::element_type>;

protected:
  std::vector<std::shared_ptr<T>> m_slots;

};

}

template <typename T>
template <typename U>
ana::concurrent<T>::concurrent(const concurrent<U>& other)
{
  static_assert( std::is_base_of_v<T,U>, "incompatible concurrent types" );
  this->m_slots.clear();
  for(size_t i=0 ; i<other.concurrency() ; ++i) {
    this->m_slots.push_back(other.get_slot(i));
  }
}

template <typename T>
template <typename U>
ana::concurrent<T>& ana::concurrent<T>::operator=(const concurrent<U>& other)
{
  this->m_slots.clear();
  for(size_t i=0 ; i<other.concurrency() ; ++i) {
    this->m_slots.push_back(other.get_slot(i));
  }
  return *this;
}

template <typename T>
void ana::concurrent<T>::add_slot(std::shared_ptr<T> slot)
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
std::shared_ptr<T> ana::concurrent<T>::get_slot(size_t i) const
{
  return m_slots[i];
}

template <typename T>
size_t ana::concurrent<T>::concurrency() const
{
  return m_slots.size();
}

template <typename T>
template <typename Lmbd, typename... Args>
void ana::concurrent<T>::to_slots(Lmbd const& lmbd, const concurrent<Args>&... args) const
{
  assert( ((concurrency()==args.concurrency())&&...) );
  for(size_t i=0 ; i<concurrency() ; ++i) {
    lmbd(*this->get_slot(i),*args.get_slot(i)...);
  }
}

template <typename T>
template <typename Lmbd, typename... Args>
auto ana::concurrent<T>::from_model(Lmbd const& lmbd, const Args&... args) const -> std::invoke_result_t<Lmbd,T&,const Args&...>
{
  auto result = lmbd(*model(),args...);
  for(size_t i=1 ; i<concurrency() ; ++i) {
    assert(result==lmbd(*this->get_slot(i),args...));
  }
  return result;
}

template <typename T>
template <typename Lmbd, typename... Args>
auto ana::concurrent<T>::from_slots(Lmbd const& lmbd, const concurrent<Args>&... args) const -> concurrent<typename std::invoke_result_t<Lmbd,T&,Args&...>::element_type>
{
  assert( ((concurrency()==args.concurrency())&&...) );
  concurrent<typename std::invoke_result_t<Lmbd,T&,Args&...>::element_type> invoked;
  for(size_t i=0 ; i<concurrency() ; ++i) {
    invoked.add_slot(lmbd(*this->get_slot(i),*args.get_slot(i)...));
  }
  return invoked;
}