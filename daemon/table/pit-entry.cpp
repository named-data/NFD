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

bool
Entry::hasLocalInRecord() const
{
  return std::any_of(m_inRecords.begin(), m_inRecords.end(),
                     [] (const InRecord& inRecord) { return inRecord.getFace()->isLocal(); });
}

bool
Entry::canForwardTo(const Face& face) const
{
  time::steady_clock::TimePoint now = time::steady_clock::now();

  bool hasUnexpiredOutRecord = std::any_of(m_outRecords.begin(), m_outRecords.end(),
    [&face, &now] (const OutRecord& outRecord) {
      return outRecord.getFace().get() == &face && outRecord.getExpiry() >= now;
    });
  if (hasUnexpiredOutRecord) {
    return false;
  }

  bool hasUnexpiredOtherInRecord = std::any_of(m_inRecords.begin(), m_inRecords.end(),
    [&face, &now] (const InRecord& inRecord) {
      return inRecord.getFace().get() != &face && inRecord.getExpiry() >= now;
    });
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

  for (const InRecord& inRecord : m_inRecords) {
    if (inRecord.getLastNonce() == nonce) {
      if (inRecord.getFace().get() == &face) {
        dnw |= DUPLICATE_NONCE_IN_SAME;
      }
      else {
        dnw |= DUPLICATE_NONCE_IN_OTHER;
      }
    }
  }

  for (const OutRecord& outRecord : m_outRecords) {
    if (outRecord.getLastNonce() == nonce) {
      if (outRecord.getFace().get() == &face) {
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
  auto it = std::find_if(m_inRecords.begin(), m_inRecords.end(),
    [&face] (const InRecord& inRecord) { return inRecord.getFace() == face; });
  if (it == m_inRecords.end()) {
    m_inRecords.emplace_front(face);
    it = m_inRecords.begin();
  }

  it->update(interest);
  return it;
}

InRecordCollection::const_iterator
Entry::getInRecord(const Face& face) const
{
  return std::find_if(m_inRecords.begin(), m_inRecords.end(),
    [&face] (const InRecord& inRecord) { return inRecord.getFace().get() == &face; });
}

void
Entry::deleteInRecords()
{
  m_inRecords.clear();
}

OutRecordCollection::iterator
Entry::insertOrUpdateOutRecord(shared_ptr<Face> face, const Interest& interest)
{
  auto it = std::find_if(m_outRecords.begin(), m_outRecords.end(),
    [&face] (const OutRecord& outRecord) { return outRecord.getFace() == face; });
  if (it == m_outRecords.end()) {
    m_outRecords.emplace_front(face);
    it = m_outRecords.begin();
  }

  it->update(interest);
  return it;
}

OutRecordCollection::const_iterator
Entry::getOutRecord(const Face& face) const
{
  return std::find_if(m_outRecords.begin(), m_outRecords.end(),
    [&face] (const OutRecord& outRecord) { return outRecord.getFace().get() == &face; });
}

void
Entry::deleteOutRecord(const Face& face)
{
  auto it = std::find_if(m_outRecords.begin(), m_outRecords.end(),
    [&face] (const OutRecord& outRecord) { return outRecord.getFace().get() == &face; });
  if (it != m_outRecords.end()) {
    m_outRecords.erase(it);
  }
}

bool
Entry::hasUnexpiredOutRecords() const
{
  time::steady_clock::TimePoint now = time::steady_clock::now();

  return std::any_of(m_outRecords.begin(), m_outRecords.end(),
    [&now] (const OutRecord& outRecord) { return outRecord.getExpiry() >= now; });
}

} // namespace pit
} // namespace nfd
