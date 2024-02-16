/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024,  Regents of the University of California,
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

#include "strategy-info-host.hpp"

#include <ndn-cxx/util/scheduler.hpp>

#include <list>

namespace nfd {

namespace face {
class Face;
} // namespace face
using face::Face;

namespace name_tree {
class Entry;
} // namespace name_tree

namespace pit {

/**
 * \brief Contains information about an Interest on an incoming or outgoing face.
 * \note This class is an implementation detail to extract common functionality
 *       of InRecord and OutRecord.
 */
class FaceRecord : public StrategyInfoHost
{
public:
  explicit
  FaceRecord(Face& face)
    : m_face(face)
  {
  }

  Face&
  getFace() const noexcept
  {
    return m_face;
  }

  Interest::Nonce
  getLastNonce() const noexcept
  {
    return m_lastNonce;
  }

  time::steady_clock::time_point
  getLastRenewed() const noexcept
  {
    return m_lastRenewed;
  }

  /**
   * \brief Returns the time point at which this record expires.
   * \return getLastRenewed() + InterestLifetime
   */
  time::steady_clock::time_point
  getExpiry() const noexcept
  {
    return m_expiry;
  }

  /**
   * \brief Updates lastNonce, lastRenewed, expiry fields.
   */
  void
  update(const Interest& interest);

private:
  Face& m_face;
  Interest::Nonce m_lastNonce{0, 0, 0, 0};
  time::steady_clock::time_point m_lastRenewed = time::steady_clock::time_point::min();
  time::steady_clock::time_point m_expiry = time::steady_clock::time_point::min();
};

/**
 * \brief Contains information about an Interest from an incoming face.
 */
class InRecord : public FaceRecord
{
public:
  using FaceRecord::FaceRecord;

  const Interest&
  getInterest() const noexcept
  {
    BOOST_ASSERT(m_interest != nullptr);
    return *m_interest;
  }

  void
  update(const Interest& interest);

private:
  shared_ptr<const Interest> m_interest;
};

/**
 * \brief Contains information about an Interest toward an outgoing face.
 */
class OutRecord : public FaceRecord
{
public:
  using FaceRecord::FaceRecord;

  /**
   * \brief Returns the last Nack returned by getFace().
   *
   * A nullptr return value means the Interest is still pending or has timed out.
   * A non-null return value means the last outgoing Interest has been Nacked.
   */
  const lp::NackHeader*
  getIncomingNack() const noexcept
  {
    return m_incomingNack.get();
  }

  /**
   * \brief Sets a Nack received from getFace().
   * \return whether incoming Nack is accepted
   *
   * This is invoked in incoming Nack pipeline.
   *
   * An incoming Nack is accepted if its Nonce matches getLastNonce().
   * If accepted, `nack.getHeader()` will be copied, and any pointers
   * previously returned by getIncomingNack() are invalidated.
   */
  bool
  setIncomingNack(const lp::Nack& nack);

  /**
   * \brief Clears last Nack.
   *
   * This is invoked in outgoing Interest pipeline.
   *
   * Any pointers previously returned by getIncomingNack() are invalidated.
   */
  void
  clearIncomingNack() noexcept
  {
    m_incomingNack.reset();
  }

private:
  unique_ptr<lp::NackHeader> m_incomingNack;
};

/**
 * \brief An unordered collection of in-records.
 */
using InRecordCollection = std::list<InRecord>;

/**
 * \brief An unordered collection of out-records.
 */
using OutRecordCollection = std::list<OutRecord>;

/**
 * \brief Represents an entry in the %Interest table (PIT).
 *
 * An Interest table entry represents either a pending Interest or a recently satisfied Interest.
 * Each entry contains a collection of in-records, a collection of out-records,
 * and two timers used in forwarding pipelines.
 * In addition, the entry, in-records, and out-records are subclasses of StrategyInfoHost,
 * which allows forwarding strategy to store arbitrary information on them.
 *
 * \sa Pit
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
  /**
   * \brief Returns the collection of in-records.
   */
  const InRecordCollection&
  getInRecords() const noexcept
  {
    return m_inRecords;
  }

  /** \retval true There is at least one in-record.
   *               This implies some downstream is waiting for Data or Nack.
   *  \retval false There is no in-record.
   *                This implies the entry is new or has been satisfied or Nacked.
   */
  bool
  hasInRecords() const noexcept
  {
    return !m_inRecords.empty();
  }

  InRecordCollection::iterator
  in_begin() noexcept
  {
    return m_inRecords.begin();
  }

  InRecordCollection::const_iterator
  in_begin() const noexcept
  {
    return m_inRecords.begin();
  }

  InRecordCollection::iterator
  in_end() noexcept
  {
    return m_inRecords.end();
  }

  InRecordCollection::const_iterator
  in_end() const noexcept
  {
    return m_inRecords.end();
  }

  /**
   * \brief Get the in-record for \p face.
   * \return an iterator to the in-record, or in_end() if it does not exist
   */
  InRecordCollection::iterator
  findInRecord(const Face& face) noexcept;

  /**
   * \brief Insert or update an in-record.
   * \return an iterator to the new or updated in-record
   */
  InRecordCollection::iterator
  insertOrUpdateInRecord(Face& face, const Interest& interest);

  /**
   * \brief Removes the in-record at position \p pos.
   * \param pos iterator to the in-record to remove; must be valid and dereferenceable.
   */
  void
  deleteInRecord(InRecordCollection::const_iterator pos)
  {
    m_inRecords.erase(pos);
  }

  /**
   * \brief Removes all in-records.
   */
  void
  clearInRecords() noexcept
  {
    m_inRecords.clear();
  }

public: // out-record
  /**
   * \brief Returns the collection of out-records.
   */
  const OutRecordCollection&
  getOutRecords() const noexcept
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
  hasOutRecords() const noexcept
  {
    return !m_outRecords.empty();
  }

  OutRecordCollection::iterator
  out_begin() noexcept
  {
    return m_outRecords.begin();
  }

  OutRecordCollection::const_iterator
  out_begin() const noexcept
  {
    return m_outRecords.begin();
  }

  OutRecordCollection::iterator
  out_end() noexcept
  {
    return m_outRecords.end();
  }

  OutRecordCollection::const_iterator
  out_end() const noexcept
  {
    return m_outRecords.end();
  }

  /**
   * \brief Get the out-record for \p face.
   * \return an iterator to the out-record, or out_end() if it does not exist
   */
  OutRecordCollection::iterator
  findOutRecord(const Face& face) noexcept;

  /**
   * \brief Insert or update an out-record.
   * \return an iterator to the new or updated out-record
   */
  OutRecordCollection::iterator
  insertOrUpdateOutRecord(Face& face, const Interest& interest);

  /**
   * \brief Delete the out-record for \p face if it exists.
   */
  void
  deleteOutRecord(const Face& face);

public:
  /** \brief Expiry timer.
   *
   *  This timer is used in forwarding pipelines to delete the entry
   */
  ndn::scheduler::EventId expiryTimer;

  /** \brief Indicates whether this PIT entry is satisfied.
   */
  bool isSatisfied = false;

  /** \brief Data freshness period.
   *  \note This field is meaningful only if #isSatisfied is true
   */
  time::milliseconds dataFreshnessPeriod = 0_ms;

private:
  shared_ptr<const Interest> m_interest;
  InRecordCollection m_inRecords;
  OutRecordCollection m_outRecords;

  name_tree::Entry* m_nameTreeEntry = nullptr;

  friend ::nfd::name_tree::Entry;
};

} // namespace pit
} // namespace nfd

#endif // NFD_DAEMON_TABLE_PIT_ENTRY_HPP
