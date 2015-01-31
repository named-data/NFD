/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NFD_RIB_PROPAGATED_ENTRY_HPP
#define NFD_RIB_PROPAGATED_ENTRY_HPP

#include "core/scheduler.hpp"

namespace nfd {
namespace rib {

enum class PropagationStatus {
  /// initial status
  NEW,
  /// is being propagated
  PROPAGATING,
  /// has been propagated successfully
  PROPAGATED,
  /// has failed in propagation
  PROPAGATE_FAIL
};

void
operator<<(std::ostream& out, PropagationStatus status);

/**
 * @brief represents an entry for prefix propagation.
 * @sa http://redmine.named-data.net/issues/3211
 *
 * it consists of a PropagationStatus indicates current state of the state machine, as
 * well as an event scheduled for refresh or retry (i.e., the RefreshTimer and the RetryTimer of
 * the state machine respectively). Besides, it stores a copy of signing identity for this entry.
 */
class PropagatedEntry
{
public:
  PropagatedEntry();

  /**
   * @pre other is not in PROPAGATED or PROPAGATE_FAIL state
   */
  PropagatedEntry(const PropagatedEntry& other);

  PropagatedEntry&
  operator=(const PropagatedEntry& other) = delete;

  /**
   * @brief set the signing identity
   */
  PropagatedEntry&
  setSigningIdentity(const Name& identity);

  /**
   * @brief get the signing identity
   *
   * @return the signing identity
   */
  const Name&
  getSigningIdentity() const;

  /**
   * @brief switch the propagation status to PROPAGATING.
   *
   * this is called before start the propagation process of this entry.
   */
  void
  startPropagation();

  /**
   * @brief switch the propagation status to PROPAGATED, and set the
   *        rePropagateEvent to @p event for refresh.
   *
   * this is called just after this entry is successfully propagated.
   */
  void
  succeed(const scheduler::EventId& event);

  /**
   * @brief switch the propagation status to PROPAGATE_FAIL, and then set the
   *        rePropagateEvent to @p event for retry.
   *
   * this is called just after propagation for this entry fails.
   */
  void
  fail(const scheduler::EventId& event);

  /**
   * @brief cancel the events of re-sending propagation commands.
   *
   * switch the propagation status to NEW.
   */
  void
  initialize();

  /**
   * @brief check whether this entry is a new entry.
   *
   * @return true if current status is NEW.
   */
  bool
  isNew() const;

  /**
   * @brief check whether this entry is being propagated.
   *
   * @return true if current status is PROPAGATING.
   */
  bool
  isPropagating() const;

  /**
   * @brief check whether this entry has been successfully propagated.
   *
   * @return true if current status is PROPAGATED.
   */
  bool
  isPropagated() const;

  /**
   * @brief check whether this entry has failed in propagating.
   *
   * @return true if current status is PROPAGATE_FAIL.
   */
  bool
  isPropagateFail() const;

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  Name m_signingIdentity;
  scheduler::ScopedEventId m_rePropagateEvent;
  PropagationStatus m_propagationStatus;
};

} // namespace rib
} // namespace nfd

#endif // NFD_RIB_PROPAGATED_ENTRY_HPP
