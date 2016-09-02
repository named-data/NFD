/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

  /** \return a Name that represents the strategy program
   */
  const Name&
  getName() const
  {
    return m_name;
  }

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
   *  \warning The strategy must not retain shared_ptr<pit::Entry>, otherwise undefined behavior
   *           may occur. However, the strategy is allowed to store weak_ptr<pit::Entry>.
   */
  virtual void
  afterReceiveInterest(const Face& inFace, const Interest& interest,
                       const shared_ptr<pit::Entry>& pitEntry) = 0;

  /** \brief trigger before PIT entry is satisfied
   *
   *  This trigger is invoked when an incoming Data satisfies the PIT entry.
   *  It can be invoked even if the PIT entry has already been satisfied.
   *
   *  In this base class this method does nothing.
   *
   *  \warning The strategy must not retain shared_ptr<pit::Entry>, otherwise undefined behavior
   *           may occur. However, the strategy is allowed to store weak_ptr<pit::Entry>.
   */
  virtual void
  beforeSatisfyInterest(const shared_ptr<pit::Entry>& pitEntry,
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
   *  \warning The strategy must not retain shared_ptr<pit::Entry>, otherwise undefined behavior
   *           may occur. However, the strategy is allowed to store weak_ptr<pit::Entry>,
   *           although this isn't useful here because PIT entry would be deleted shortly after.
   */
  virtual void
  beforeExpirePendingInterest(const shared_ptr<pit::Entry>& pitEntry);

  /** \brief trigger after Nack is received
   *
   *  This trigger is invoked when an incoming Nack is received in response to
   *  an forwarded Interest.
   *  The Nack has been confirmed to be a response to the last Interest forwarded
   *  to that upstream, i.e. the PIT out-record exists and has a matching Nonce.
   *  The NackHeader has been recorded in the PIT out-record.
   *
   *  In this base class this method does nothing.
   *
   *  \warning The strategy must not retain shared_ptr<pit::Entry>, otherwise undefined behavior
   *           may occur. However, the strategy is allowed to store weak_ptr<pit::Entry>.
   */
  virtual void
  afterReceiveNack(const Face& inFace, const lp::Nack& nack,
                   const shared_ptr<pit::Entry>& pitEntry);

protected: // actions
  /** \brief send Interest to outFace
   *  \param pitEntry PIT entry
   *  \param outFace face through which to send out the Interest
   *  \param wantNewNonce if true, a new Nonce will be generated,
   *                      rather than reusing a Nonce from one of the PIT in-records
   */
  VIRTUAL_WITH_TESTS void
  sendInterest(const shared_ptr<pit::Entry>& pitEntry, Face& outFace,
               bool wantNewNonce = false)
  {
    m_forwarder.onOutgoingInterest(pitEntry, outFace, wantNewNonce);
  }

  /** \brief decide that a pending Interest cannot be forwarded
   *  \param pitEntry PIT entry
   *
   *  This shall not be called if the pending Interest has been
   *  forwarded earlier, and does not need to be resent now.
   */
  VIRTUAL_WITH_TESTS void
  rejectPendingInterest(const shared_ptr<pit::Entry>& pitEntry)
  {
    m_forwarder.onInterestReject(pitEntry);
  }

  /** \brief send Nack to outFace
   *  \param pitEntry PIT entry
   *  \param outFace face through which to send out the Nack
   *  \param header Nack header
   *
   *  The outFace must have a PIT in-record, otherwise this method has no effect.
   */
  VIRTUAL_WITH_TESTS void
  sendNack(const shared_ptr<pit::Entry>& pitEntry, const Face& outFace,
           const lp::NackHeader& header)
  {
    m_forwarder.onOutgoingNack(pitEntry, outFace, header);
  }

  /** \brief send Nack to every face that has an in-record,
   *         except those in \p exceptFaces
   *  \param pitEntry PIT entry
   *  \param header NACK header
   *  \param exceptFaces list of faces that should be excluded from sending Nacks
   *  \note This is not an action, but a helper that invokes the sendNack action.
   */
  void
  sendNacks(const shared_ptr<pit::Entry>& pitEntry, const lp::NackHeader& header,
            std::initializer_list<const Face*> exceptFaces = std::initializer_list<const Face*>());

protected: // accessors
  /** \brief performs a FIB lookup, considering Link object if present
   */
  const fib::Entry&
  lookupFib(const pit::Entry& pitEntry) const;

  MeasurementsAccessor&
  getMeasurements()
  {
    return m_measurements;
  }

  Face*
  getFace(FaceId id) const
  {
    return m_forwarder.getFace(id);
  }

  const FaceTable&
  getFaceTable() const
  {
    return m_forwarder.getFaceTable();
  }

protected: // accessors
  signal::Signal<FaceTable, Face&>& afterAddFace;
  signal::Signal<FaceTable, Face&>& beforeRemoveFace;

private:
  Name m_name;

  /** \brief reference to the forwarder
   *
   *  Triggers can access forwarder indirectly via actions.
   */
  Forwarder& m_forwarder;

  MeasurementsAccessor m_measurements;
};

} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_STRATEGY_HPP
