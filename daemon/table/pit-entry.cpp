/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "pit-entry.hpp"
#include <algorithm>

namespace nfd {
namespace pit {

Entry::Entry(const Interest& interest)
  : m_interest(interest)
{
}

const Name&
Entry::getName() const
{
  return m_interest.getName();
}

const InRecordCollection&
Entry::getInRecords() const
{
  return m_inRecords;
}

const OutRecordCollection&
Entry::getOutRecords() const
{
  return m_outRecords;
}

static inline bool
predicate_InRecord_isLocal(const InRecord& inRecord)
{
  return inRecord.getFace()->isLocal();
}

bool
Entry::hasLocalInRecord() const
{
  InRecordCollection::const_iterator it = std::find_if(
    m_inRecords.begin(), m_inRecords.end(), &predicate_InRecord_isLocal);
  return it != m_inRecords.end();
}

static inline bool
predicate_FaceRecord_Face(const FaceRecord& faceRecord, shared_ptr<Face> face)
{
  return faceRecord.getFace() == face;
}

static inline bool
predicate_FaceRecord_ne_Face_and_unexpired(const FaceRecord& faceRecord,
  shared_ptr<Face> face, time::Point now)
{
  return faceRecord.getFace() != face && faceRecord.getExpiry() >= now;
}

bool
Entry::canForwardTo(shared_ptr<Face> face) const
{
  OutRecordCollection::const_iterator outIt = std::find_if(
    m_outRecords.begin(), m_outRecords.end(),
    bind(&predicate_FaceRecord_Face, _1, face));
  bool hasUnexpiredOutRecord = outIt != m_outRecords.end() &&
                               outIt->getExpiry() >= time::now();
  if (hasUnexpiredOutRecord) {
    return false;
  }

  InRecordCollection::const_iterator inIt = std::find_if(
    m_inRecords.begin(), m_inRecords.end(),
    bind(&predicate_FaceRecord_ne_Face_and_unexpired, _1, face, time::now()));
  bool hasUnexpiredOtherInRecord = inIt != m_inRecords.end();
  return hasUnexpiredOtherInRecord;
}

bool
Entry::addNonce(uint32_t nonce)
{
  std::pair<std::set<uint32_t>::iterator, bool> insertResult =
    m_nonces.insert(nonce);

  return insertResult.second;
}

InRecordCollection::iterator
Entry::insertOrUpdateInRecord(shared_ptr<Face> face, const Interest& interest)
{
  InRecordCollection::iterator it = std::find_if(m_inRecords.begin(),
    m_inRecords.end(), bind(&predicate_FaceRecord_Face, _1, face));
  if (it == m_inRecords.end()) {
    m_inRecords.push_front(InRecord(face));
    it = m_inRecords.begin();
  }

  it->update(interest);
  return it;
}

void
Entry::deleteInRecords()
{
  m_inRecords.clear();
}

OutRecordCollection::iterator
Entry::insertOrUpdateOutRecord(shared_ptr<Face> face, const Interest& interest)
{
  OutRecordCollection::iterator it = std::find_if(m_outRecords.begin(),
    m_outRecords.end(), bind(&predicate_FaceRecord_Face, _1, face));
  if (it == m_outRecords.end()) {
    m_outRecords.push_front(OutRecord(face));
    it = m_outRecords.begin();
  }

  it->update(interest);
  m_nonces.insert(interest.getNonce());
  return it;
}

void
Entry::deleteOutRecord(shared_ptr<Face> face)
{
  OutRecordCollection::iterator it = std::find_if(m_outRecords.begin(),
    m_outRecords.end(), bind(&predicate_FaceRecord_Face, _1, face));
  if (it != m_outRecords.end()) {
    m_outRecords.erase(it);
  }
}


} // namespace pit
} // namespace nfd
