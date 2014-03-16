/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "pit-entry.hpp"
#include <algorithm>

namespace nfd {
namespace pit {

const Name Entry::LOCALHOST_NAME("ndn:/localhost");
const Name Entry::LOCALHOP_NAME("ndn:/localhop");

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
predicate_FaceRecord_Face(const FaceRecord& faceRecord, const Face* face)
{
  return faceRecord.getFace().get() == face;
}

static inline bool
predicate_FaceRecord_ne_Face_and_unexpired(const FaceRecord& faceRecord,
  const Face* face, time::Point now)
{
  return faceRecord.getFace().get() != face && faceRecord.getExpiry() >= now;
}

bool
Entry::canForwardTo(const Face& face) const
{
  OutRecordCollection::const_iterator outIt = std::find_if(
    m_outRecords.begin(), m_outRecords.end(),
    bind(&predicate_FaceRecord_Face, _1, &face));
  bool hasUnexpiredOutRecord = outIt != m_outRecords.end() &&
                               outIt->getExpiry() >= time::now();
  if (hasUnexpiredOutRecord) {
    return false;
  }

  InRecordCollection::const_iterator inIt = std::find_if(
    m_inRecords.begin(), m_inRecords.end(),
    bind(&predicate_FaceRecord_ne_Face_and_unexpired, _1, &face, time::now()));
  bool hasUnexpiredOtherInRecord = inIt != m_inRecords.end();
  if (!hasUnexpiredOtherInRecord) {
    return false;
  }

  return !this->violatesScope(face);
}

bool
Entry::violatesScope(const Face& face) const
{
  // /localhost scope
  bool isViolatingLocalhost = !face.isLocal() &&
                              LOCALHOST_NAME.isPrefixOf(this->getName());
  if (isViolatingLocalhost) {
    return true;
  }

  // /localhop scope
  bool isViolatingLocalhop = !face.isLocal() &&
                             LOCALHOP_NAME.isPrefixOf(this->getName()) &&
                             !this->hasLocalInRecord();
  if (isViolatingLocalhop) {
    return true;
  }

  return false;
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
    m_inRecords.end(), bind(&predicate_FaceRecord_Face, _1, face.get()));
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
    m_outRecords.end(), bind(&predicate_FaceRecord_Face, _1, face.get()));
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
    m_outRecords.end(), bind(&predicate_FaceRecord_Face, _1, face.get()));
  if (it != m_outRecords.end()) {
    m_outRecords.erase(it);
  }
}

static inline bool
predicate_FaceRecord_unexpired(const FaceRecord& faceRecord, time::Point now)
{
  return faceRecord.getExpiry() >= now;
}

bool
Entry::hasUnexpiredOutRecords() const
{
  OutRecordCollection::const_iterator it = std::find_if(m_outRecords.begin(),
    m_outRecords.end(), bind(&predicate_FaceRecord_unexpired, _1, time::now()));
  return it != m_outRecords.end();
}

} // namespace pit
} // namespace nfd
