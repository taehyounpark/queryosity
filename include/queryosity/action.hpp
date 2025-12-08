#pragma once

#include "systematic.hpp"

#include <string>

namespace queryosity {

/**
 * @ingroup abc
 * @brief The abstract base class of all dataflow actions (columns, selections, and queries).
 * @details The order of execution of an action's methods are as follows:
 * 1. `vary()` immediately after the instantiation of an action (if it is not nominal).
 * 2. `initialize()` before entering the entry loop.
 * 3. `execute()` for each entry.
 * 4. `finalize()` after exiting the entry loop.
*/
class action {

public:
  action() = default;
  virtual ~action() = default;

  /**
   * @brief Inform this instance that it has been varied by the variation name.
   * @param[in] variation_name Variation name.
   * @details This method is intended to allow systematic variations of a custom action that are triggered by input columns to be handled manually. It is not invoked for nominal actions.
  */
  virtual void vary(const std::string &variation_name);

  /**
   * @brief Initialize the action.
   * @param[in] slot The thread slot index.
   * @param[in] begin First `entry` to be processed by the action.
   * @param[in] end Last `entry - 1` to be processed by the action.
   */
  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) = 0;

  /**
   * @brief Execute the action.
   * @param[in] slot The thread slot index.
   * @param[in] begin The `entry` being processed by the action.
  */
  virtual void execute(unsigned int slot, unsigned long long entry) = 0;

  /**
   * @brief Finalize the action.
   * @param[in] slot The thread slot index.
  */
  virtual void finalize(unsigned int slot) = 0;

};

} // namespace queryosity

inline void queryosity::action::vary(const std::string &) {}