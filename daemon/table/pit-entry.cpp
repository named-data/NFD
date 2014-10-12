/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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

#include "pit-entry.hpp"
#include <algorithm>

namespace nfd {
namespace pit {

const Name Entry::LOCALHOST_NAME("ndn:/localhost");
const Name Entry::LOCALHOP_NAME("ndn:/localhop");

Entry::Entry(const Interest& interest)
  : m_interest(interest.shared_from_this())
{
}

const Name&
Entry::getName() const
{
  return m_interest->getName();
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
  const Face* face, const time::steady_clock::TimePoint& now)
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
                               outIt->getExpiry() >= time::steady_clock::now();
  if (hasUnexpiredOutRecord) {
    return false;
  }

  InRecordCollection::const_iterator inIt = std::find_if(
    m_inRecords.begin(), m_inRecords.end(),
    bind(&predicate_FaceRecord_ne_Face_and_unexpired, _1, &face, time::steady_clock::now()));
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

int
Entry::findNonce(uint32_t nonce, const Face& face) const
{
  // TODO should we ignore expired in/out records?

  int dnw = DUPLICATE_NONCE_NONE;

  for (InRecordCollection::const_iterator it = m_inRecords.begin();
       it != m_inRecords.end(); ++it) {
    if (it->getLastNonce() == nonce) {
      if (it->getFace().get() == &face) {
        dnw |= DUPLICATE_NONCE_IN_SAME;
      }
      else {
        dnw |= DUPLICATE_NONCE_IN_OTHER;
      }
    }
  }

  for (OutRecordCollection::const_iterator it = m_outRecords.begin();
       it != m_outRecords.end(); ++it) {
    if (it->getLastNonce() == nonce) {
      if (it->getFace().get() == &face) {
        dnw |= DUPLICATE_NONCE_OUT_SAME;
      }
      else {
        dnw |= DUPLICATE_NONCE_OUT_OTHER;
      }
    }
  }

  return dnw;
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

InRecordCollection::const_iterator
Entry::getInRecord(shared_ptr<Face> face) const
{
  return std::find_if(m_inRecords.begin(), m_inRecords.end(),
                      bind(&predicate_FaceRecord_Face, _1, face.get()));
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
  return it;
}

OutRecordCollection::const_iterator
Entry::getOutRecord(shared_ptr<Face> face) const
{
  return std::find_if(m_outRecords.begin(), m_outRecords.end(),
                      bind(&predicate_FaceRecord_Face, _1, face.get()));
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
predicate_FaceRecord_unexpired(const FaceRecord& faceRecord, const time::steady_clock::TimePoint& now)
{
  return faceRecord.getExpiry() >= now;
}

bool
Entry::hasUnexpiredOutRecords() const
{
  OutRecordCollection::const_iterator it = std::find_if(m_outRecords.begin(),
    m_outRecords.end(), bind(&predicate_FaceRecord_unexpired, _1, time::steady_clock::now()));
  return it != m_outRecords.end();
}

} // namespace pit
} // namespace nfd
