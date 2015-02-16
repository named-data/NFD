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

#ifndef NFD_DAEMON_FW_STRATEGY_HPP
#define NFD_DAEMON_FW_STRATEGY_HPP

#include "forwarder.hpp"
#include "strategy-registry.hpp"
#include "table/measurements-accessor.hpp"

namespace nfd {
namespace fw {

/** \brief represents a forwarding strategy
 */
class Strategy : public enable_shared_from_this<Strategy>, noncopyable
{
public:
  /** \brief construct a strategy instance
   *  \param forwarder a reference to the Forwarder, used to enable actions and accessors.
   *         Strategy subclasses should pass this reference,
   *         and should not keep a reference themselves.
   *  \param name the strategy Name.
   *         It's recommended to include a version number as the last component.
   */
  Strategy(Forwarder& forwarder, const Name& name);

  virtual
  ~Strategy();

  /// a Name that represent the Strategy program
  const Name&
  getName() const;

public: // triggers
  /** \brief trigger after Interest is received
   *
   *  The Interest:
   *  - does not violate Scope
   *  - is not looped
   *  - cannot be satisfied by ContentStore
   *  - is under a namespace managed by this strategy
   *
   *  The strategy should decide whether and where to forward this Interest.
   *  - If the strategy decides to forward this Interest,
   *    invoke this->sendInterest one or more times, either now or shortly after
   *  - If strategy concludes that this Interest cannot be forwarded,
   *    invoke this->rejectPendingInterest so that PIT entry will be deleted shortly
   *
   *  \note The strategy is permitted to store a weak reference to fibEntry.
   *        Do not store a shared reference, because PIT entry may be deleted at any moment.
   *        fibEntry is passed by value to allow obtaining a weak reference from it.
   *  \note The strategy is permitted to store a shared reference to pitEntry.
   *        pitEntry is passed by value to reflect this fact.
   */
  virtual void
  afterReceiveInterest(const Face& inFace,
                       const Interest& interest,
                       shared_ptr<fib::Entry> fibEntry,
                       shared_ptr<pit::Entry> pitEntry) = 0;

  /** \brief trigger before PIT entry is satisfied
   *
   *  This trigger is invoked when an incoming Data satisfies the PIT entry.
   *  It can be invoked even if the PIT entry has already been satisfied.
   *
   *  In this base class this method does nothing.
   *
   *  \note The strategy is permitted to store a shared reference to pitEntry.
   *        pitEntry is passed by value to reflect this fact.
   */
  virtual void
  beforeSatisfyInterest(shared_ptr<pit::Entry> pitEntry,
                        const Face& inFace, const Data& data);

  /** \brief trigger before PIT entry expires
   *
   *  PIT entry expires when InterestLifetime has elapsed for all InRecords,
   *  and it is not satisfied by an incoming Data.
   *
   *  This trigger is not invoked for PIT entry already satisfied.
   *
   *  In this base class this method does nothing.
   *
   *  \note The strategy is permitted to store a shared reference to pitEntry.
   *        pitEntry is passed by value to reflect this fact.
   */
  virtual void
  beforeExpirePendingInterest(shared_ptr<pit::Entry> pitEntry);

protected: // actions
  /// send Interest to outFace
  VIRTUAL_WITH_TESTS void
  sendInterest(shared_ptr<pit::Entry> pitEntry,
               shared_ptr<Face> outFace,
               bool wantNewNonce = false);

  /** \brief decide that a pending Interest cannot be forwarded
   *
   *  This shall not be called if the pending Interest has been
   *  forwarded earlier, and does not need to be resent now.
   */
  VIRTUAL_WITH_TESTS void
  rejectPendingInterest(shared_ptr<pit::Entry> pitEntry);

protected: // accessors
  MeasurementsAccessor&
  getMeasurements();

  shared_ptr<Face>
  getFace(FaceId id);

  const FaceTable&
  getFaceTable();

protected: // accessors
  signal::Signal<FaceTable, shared_ptr<Face>>& afterAddFace;
  signal::Signal<FaceTable, shared_ptr<Face>>& beforeRemoveFace;

private:
  Name m_name;

  /** \brief reference to the forwarder
   *
   *  Triggers can access forwarder indirectly via actions.
   */
  Forwarder& m_forwarder;

  MeasurementsAccessor m_measurements;
};

inline const Name&
Strategy::getName() const
{
  return m_name;
}

inline void
Strategy::sendInterest(shared_ptr<pit::Entry> pitEntry,
                       shared_ptr<Face> outFace,
                       bool wantNewNonce)
{
  m_forwarder.onOutgoingInterest(pitEntry, *outFace, wantNewNonce);
}

inline void
Strategy::rejectPendingInterest(shared_ptr<pit::Entry> pitEntry)
{
  m_forwarder.onInterestReject(pitEntry);
}

inline MeasurementsAccessor&
Strategy::getMeasurements()
{
  return m_measurements;
}

inline shared_ptr<Face>
Strategy::getFace(FaceId id)
{
  return m_forwarder.getFace(id);
}

inline const FaceTable&
Strategy::getFaceTable()
{
  return m_forwarder.getFaceTable();
}

} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_STRATEGY_HPP
