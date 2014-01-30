/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TABLE_PIT_ENTRY_HPP
#define NFD_TABLE_PIT_ENTRY_HPP

#include "pit-in-record.hpp"
#include "pit-out-record.hpp"
#include "core/scheduler.hpp"

namespace nfd {
namespace pit {

/** \class InRecordCollection
 *  \brief represents an unordered collection of InRecords
 */
typedef std::list< InRecord>  InRecordCollection;

/** \class OutRecordCollection
 *  \brief represents an unordered collection of OutRecords
 */
typedef std::list<OutRecord> OutRecordCollection;

/** \class Entry
 *  \brief represents a PIT entry
 */
class Entry : noncopyable
{
public:
  explicit
  Entry(const Interest& interest);
  
  const Interest&
  getInterest() const;
  
  /** \return{ Interest Name }
   */
  const Name&
  getName() const;
  
  const InRecordCollection&
  getInRecords() const;
  
  const OutRecordCollection&
  getOutRecords() const;
  
  /** \brief records a nonce
   *
   *  \return{ true if nonce is new; false if nonce is seen before }
   */
  bool
  addNonce(uint32_t nonce);
  
  /** \brief inserts a InRecord for face, and updates it with interest
   *
   *  If InRecord for face exists, the existing one is updated.
   *  This method does not add the Nonce as a seen Nonce.
   *  \return{ an iterator to the InRecord }
   */
  InRecordCollection::iterator
  insertOrUpdateInRecord(shared_ptr<Face> face, const Interest& interest);
  
  /// deletes all InRecords
  void
  deleteInRecords();
  
  /** \brief inserts a OutRecord for face, and updates it with interest
   *
   *  If OutRecord for face exists, the existing one is updated.
   *  \return{ an iterator to the OutRecord }
   */
  OutRecordCollection::iterator
  insertOrUpdateOutRecord(shared_ptr<Face> face, const Interest& interest);
  
  /// deletes one OutRecord for face if exists
  void
  deleteOutRecord(shared_ptr<Face> face);
  
public:
  EventId m_unsatisfyTimer;
  EventId m_stragglerTimer;

private:
  std::set<uint32_t> m_nonces;
  const Interest m_interest;
  InRecordCollection m_inRecords;
  OutRecordCollection m_outRecords;
};

inline const Interest&
Entry::getInterest() const
{
  return m_interest;
}

} // namespace pit
} // namespace nfd

#endif // NFD_TABLE_PIT_ENTRY_HPP
