/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
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
 **/

#ifndef NFD_TABLE_PIT_ENTRY_HPP
#define NFD_TABLE_PIT_ENTRY_HPP

#include "pit-in-record.hpp"
#include "pit-out-record.hpp"
#include "core/scheduler.hpp"

namespace nfd {

class NameTree;

namespace name_tree {
class Entry;
}

namespace pit {

/** \class InRecordCollection
 *  \brief represents an unordered collection of InRecords
 */
typedef std::list< InRecord>  InRecordCollection;

/** \class OutRecordCollection
 *  \brief represents an unordered collection of OutRecords
 */
typedef std::list<OutRecord> OutRecordCollection;

/** \brief represents a PIT entry
 */
class Entry : public StrategyInfoHost, noncopyable
{
public:
  explicit
  Entry(const Interest& interest);

  const Interest&
  getInterest() const;

  /** \return Interest Name
   */
  const Name&
  getName() const;

  const InRecordCollection&
  getInRecords() const;

  const OutRecordCollection&
  getOutRecords() const;

  /** \brief determines whether any InRecord is a local Face
   *
   *  \return true if any InRecord is a local Face,
   *          false if all InRecords are non-local Faces
   */
  bool
  hasLocalInRecord() const;

  /** \brief decides whether Interest can be forwarded to face
   *
   *  \return true if OutRecord of this face does not exist or has expired,
   *          and there is an InRecord not of this face,
   *          and scope is not violated
   */
  bool
  canForwardTo(const Face& face) const;

  /** \brief decides whether forwarding Interest to face would violate scope
   *
   *  \return true if scope control would be violated
   *  \note canForwardTo has more comprehensive checks (including scope control)
   *        and should be used by most strategies. Outgoing Interest pipeline
   *        should only check scope because some strategy (eg. vehicular) needs
   *        to retransmit sooner than OutRecord expiry, or forward Interest
   *        back to incoming face
   */
  bool
  violatesScope(const Face& face) const;

  /** \brief records a nonce
   *
   *  \return true if nonce is new; false if nonce is seen before
   */
  bool
  addNonce(uint32_t nonce);

  /** \brief inserts a InRecord for face, and updates it with interest
   *
   *  If InRecord for face exists, the existing one is updated.
   *  This method does not add the Nonce as a seen Nonce.
   *  \return an iterator to the InRecord
   */
  InRecordCollection::iterator
  insertOrUpdateInRecord(shared_ptr<Face> face, const Interest& interest);

  /// deletes all InRecords
  void
  deleteInRecords();

  /** \brief inserts a OutRecord for face, and updates it with interest
   *
   *  If OutRecord for face exists, the existing one is updated.
   *  \return an iterator to the OutRecord
   */
  OutRecordCollection::iterator
  insertOrUpdateOutRecord(shared_ptr<Face> face, const Interest& interest);

  /// deletes one OutRecord for face if exists
  void
  deleteOutRecord(shared_ptr<Face> face);

  /** \return true if there is one or more unexpired OutRecords
   */
  bool
  hasUnexpiredOutRecords() const;

public:
  EventId m_unsatisfyTimer;
  EventId m_stragglerTimer;

private:
  std::set<uint32_t> m_nonces;
  const Interest m_interest;
  InRecordCollection m_inRecords;
  OutRecordCollection m_outRecords;

  static const Name LOCALHOST_NAME;
  static const Name LOCALHOP_NAME;

  shared_ptr<name_tree::Entry> m_nameTreeEntry;

  friend class nfd::NameTree;
  friend class nfd::name_tree::Entry;
};

inline const Interest&
Entry::getInterest() const
{
  return m_interest;
}

} // namespace pit
} // namespace nfd

#endif // NFD_TABLE_PIT_ENTRY_HPP
