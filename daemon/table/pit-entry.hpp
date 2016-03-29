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

#ifndef NFD_DAEMON_TABLE_PIT_ENTRY_HPP
#define NFD_DAEMON_TABLE_PIT_ENTRY_HPP

#include "pit-in-record.hpp"
#include "pit-out-record.hpp"
#include "core/scheduler.hpp"

namespace nfd {

class NameTree;

namespace name_tree {
class Entry;
} // namespace name_tree

namespace pit {

/** \brief represents an unordered collection of InRecords
 */
typedef std::list< InRecord>  InRecordCollection;

/** \brief represents an unordered collection of OutRecords
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

public: // InRecord
  const InRecordCollection&
  getInRecords() const;

  /** \brief inserts a InRecord for face, and updates it with interest
   *
   *  If InRecord for face exists, the existing one is updated.
   *  This method does not add the Nonce as a seen Nonce.
   *  \return an iterator to the InRecord
   */
  InRecordCollection::iterator
  insertOrUpdateInRecord(shared_ptr<Face> face, const Interest& interest);

  /** \brief get the InRecord for face
   *  \return an iterator to the InRecord, or .end if it does not exist
   */
  InRecordCollection::const_iterator
  getInRecord(const Face& face) const;

  /// deletes one InRecord for face if exists
  void
  deleteInRecord(const Face& face);

  /// deletes all InRecords
  void
  deleteInRecords();

public: // OutRecord
  const OutRecordCollection&
  getOutRecords() const;

  /** \brief inserts a OutRecord for face, and updates it with interest
   *
   *  If OutRecord for face exists, the existing one is updated.
   *  \return an iterator to the OutRecord
   */
  OutRecordCollection::iterator
  insertOrUpdateOutRecord(shared_ptr<Face> face, const Interest& interest);

  /** \brief get the OutRecord for face
   *  \return an iterator to the OutRecord, or .end if it does not exist
   */
  OutRecordCollection::iterator
  getOutRecord(const Face& face);

  /// deletes one OutRecord for face if exists
  void
  deleteOutRecord(const Face& face);

public:
  scheduler::EventId m_unsatisfyTimer;
  scheduler::EventId m_stragglerTimer;

private:
  shared_ptr<const Interest> m_interest;
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
  return *m_interest;
}

inline const InRecordCollection&
Entry::getInRecords() const
{
  return m_inRecords;
}

inline const OutRecordCollection&
Entry::getOutRecords() const
{
  return m_outRecords;
}

} // namespace pit
} // namespace nfd

#endif // NFD_DAEMON_TABLE_PIT_ENTRY_HPP
