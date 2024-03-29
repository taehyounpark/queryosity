#pragma once

#include <iostream>
#include <memory>
#include <type_traits>
#include <vector>

#include "dataflow.h"
#include "systematic_resolver.h"

namespace queryosity {

template <typename Action>
class delayed : public dataflow::node, public ensemble::slotted<Action> {

public:
  using action_type = Action;

public:
  friend class dataflow;
  template <typename> friend class delayed;

public:
  delayed(dataflow &df, std::vector<Action *> const &slots)
      : dataflow::node(df), m_slots(slots) {}
  delayed(dataflow &df, std::vector<std::unique_ptr<Action>> const &slots)
      : dataflow::node(df) {
    this->m_slots.reserve(slots.size());
    for (auto const &slot : slots) {
      this->m_slots.push_back(slot.get());
    }
  }

  delayed(const delayed &) = delete;
  delayed &operator=(const delayed &) = delete;

  delayed(delayed &&) = default;
  delayed &operator=(delayed &&) = default;

  template <typename Derived> delayed(delayed<Derived> const &derived);
  template <typename Derived>
  delayed &operator=(delayed<Derived> const &derived);

  virtual ~delayed() = default;

  virtual Action *get_slot(unsigned int islot) const override;
  virtual unsigned int concurrency() const override;

protected:
  std::vector<Action *> m_slots;
};

} // namespace queryosity