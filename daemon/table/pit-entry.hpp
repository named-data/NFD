/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

namespace name_tree {
class Entry;
} // namespace name_tree

namespace pit {

/** \brief an unordered collection of in-records
 */
typedef std::list<InRecord> InRecordCollection;

/** \brief an unordered collection of out-records
 */
typedef std::list<OutRecord> OutRecordCollection;

/** \brief an Interest table entry
 *
 *  An Interest table entry represents either a pending Interest or a recently satisfied Interest.
 *  Each entry contains a collection of in-records, a collection of out-records,
 *  and two timers used in forwarding pipelines.
 *  In addition, the entry, in-records, and out-records are subclasses of StrategyInfoHost,
 *  which allows forwarding strategy to store arbitrary information on them.
 */
class Entry : public StrategyInfoHost, noncopyable
{
public:
  explicit
  Entry(const Interest& interest);

  /** \return the representative Interest of the PIT entry
   *  \note Every Interest in in-records and out-records should have same Name and Selectors
   *        as the representative Interest.
   *  \todo #3162 require Link field to match the representative Interest
   */
  const Interest&
  getInterest() const
  {
    return *m_interest;
  }

  /** \return Interest Name
   */
  const Name&
  getName() const
  {
    return m_interest->getName();
  }

  /** \return whether interest matches this entry
   *  \param interest the Interest
   *  \param nEqualNameComps number of initial name components guaranteed to be equal
   */
  bool
  canMatch(const Interest& interest, size_t nEqualNameComps = 0) const;

public: // in-record
  /** \return collection of in-records
   */
  const InRecordCollection&
  getInRecords() const
  {
    return m_inRecords;
  }

  /** \retval true There is at least one in-record.
   *               This implies some downstream is waiting for Data or Nack.
   *  \retval false There is no in-record.
   *                This implies the entry is new or has been satisfied or Nacked.
   */
  bool
  hasInRecords() const
  {
    return !m_inRecords.empty();
  }

  InRecordCollection::iterator
  in_begin()
  {
    return m_inRecords.begin();
  }

  InRecordCollection::const_iterator
  in_begin() const
  {
    return m_inRecords.begin();
  }

  InRecordCollection::iterator
  in_end()
  {
    return m_inRecords.end();
  }

  InRecordCollection::const_iterator
  in_end() const
  {
    return m_inRecords.end();
  }

  /** \brief get the in-record for \p face
   *  \return an iterator to the in-record, or .in_end() if it does not exist
   */
  InRecordCollection::iterator
  getInRecord(const Face& face);

  /** \brief insert or update an in-record
   *  \return an iterator to the new or updated in-record
   */
  InRecordCollection::iterator
  insertOrUpdateInRecord(Face& face, const Interest& interest);

  /** \brief delete the in-record for \p face if it exists
   */
  void
  deleteInRecord(const Face& face);

  /** \brief delete all in-records
   */
  void
  clearInRecords();

public: // out-record
  /** \return collection of in-records
   */
  const OutRecordCollection&
  getOutRecords() const
  {
    return m_outRecords;
  }

  /** \retval true There is at least one out-record.
   *               This implies the Interest has been forwarded to some upstream,
   *               and they haven't returned Data, but may have returned Nacks.
   *  \retval false There is no out-record.
   *                This implies the Interest has not been forwarded.
   */
  bool
  hasOutRecords() const
  {
    return !m_outRecords.empty();
  }

  OutRecordCollection::iterator
  out_begin()
  {
    return m_outRecords.begin();
  }

  OutRecordCollection::const_iterator
  out_begin() const
  {
    return m_outRecords.begin();
  }

  OutRecordCollection::iterator
  out_end()
  {
    return m_outRecords.end();
  }

  OutRecordCollection::const_iterator
  out_end() const
  {
    return m_outRecords.end();
  }

  /** \brief get the out-record for \p face
   *  \return an iterator to the out-record, or .out_end() if it does not exist
   */
  OutRecordCollection::iterator
  getOutRecord(const Face& face);

  /** \brief insert or update an out-record
   *  \return an iterator to the new or updated out-record
   */
  OutRecordCollection::iterator
  insertOrUpdateOutRecord(Face& face, const Interest& interest);

  /** \brief delete the out-record for \p face if it exists
   */
  void
  deleteOutRecord(const Face& face);

public:
  /** \brief expiry timer
   *
   *  This timer is used in forwarding pipelines to delete the entry
   */
  scheduler::EventId expiryTimer;

  /** \brief indicate if PIT entry is satisfied
   */
  bool isSatisfied;

  /** \brief Data freshness period
   *  \note This field is meaningful only if isSatisfied is true
   */
  time::milliseconds dataFreshnessPeriod;

private:
  shared_ptr<const Interest> m_interest;
  InRecordCollection m_inRecords;
  OutRecordCollection m_outRecords;

  name_tree::Entry* m_nameTreeEntry;

  friend class name_tree::Entry;
};

} // namespace pit
} // namespace nfd

#endif // NFD_DAEMON_TABLE_PIT_ENTRY_HPP
