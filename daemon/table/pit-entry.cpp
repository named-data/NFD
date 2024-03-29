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

#include "pit-entry.hpp"

#include <algorithm>

namespace nfd::pit {

void
FaceRecord::update(const Interest& interest)
{
  m_lastNonce = interest.getNonce();
  m_lastRenewed = time::steady_clock::now();

  auto lifetime = interest.getInterestLifetime();
  // impose an upper bound on the lifetime to prevent integer overflow when calculating m_expiry
  lifetime = std::min<time::milliseconds>(lifetime, 10_days);
  m_expiry = m_lastRenewed + lifetime;
}

void
InRecord::update(const Interest& interest)
{
  FaceRecord::update(interest);
  m_interest = interest.shared_from_this();
}

bool
OutRecord::setIncomingNack(const lp::Nack& nack)
{
  if (nack.getInterest().getNonce() != this->getLastNonce()) {
    return false;
  }

  m_incomingNack = make_unique<lp::NackHeader>(nack.getHeader());
  return true;
}

Entry::Entry(const Interest& interest)
  : m_interest(interest.shared_from_this())
{
}

bool
Entry::canMatch(const Interest& interest, size_t nEqualNameComps) const
{
  BOOST_ASSERT(m_interest->getName().compare(0, nEqualNameComps,
                                             interest.getName(), 0, nEqualNameComps) == 0);

  return m_interest->getName().compare(nEqualNameComps, Name::npos,
                                       interest.getName(), nEqualNameComps) == 0 &&
         m_interest->getCanBePrefix() == interest.getCanBePrefix() &&
         m_interest->getMustBeFresh() == interest.getMustBeFresh();
  /// \todo #3162 match ForwardingHint field
}

InRecordCollection::iterator
Entry::findInRecord(const Face& face) noexcept
{
  return std::find_if(m_inRecords.begin(), m_inRecords.end(),
                      [&face] (const InRecord& inRecord) { return &inRecord.getFace() == &face; });
}

InRecordCollection::iterator
Entry::insertOrUpdateInRecord(Face& face, const Interest& interest)
{
  BOOST_ASSERT(this->canMatch(interest));

  auto it = findInRecord(face);
  if (it == m_inRecords.end()) {
    m_inRecords.emplace_front(face);
    it = m_inRecords.begin();
  }

  it->update(interest);
  return it;
}

OutRecordCollection::iterator
Entry::findOutRecord(const Face& face) noexcept
{
  return std::find_if(m_outRecords.begin(), m_outRecords.end(),
                      [&face] (const OutRecord& outRecord) { return &outRecord.getFace() == &face; });
}

OutRecordCollection::iterator
Entry::insertOrUpdateOutRecord(Face& face, const Interest& interest)
{
  BOOST_ASSERT(this->canMatch(interest));

  auto it = findOutRecord(face);
  if (it == m_outRecords.end()) {
    m_outRecords.emplace_front(face);
    it = m_outRecords.begin();
  }

  it->update(interest);
  return it;
}

void
Entry::deleteOutRecord(const Face& face)
{
  auto it = std::find_if(m_outRecords.begin(), m_outRecords.end(),
                         [&face] (const OutRecord& outRecord) { return &outRecord.getFace() == &face; });
  if (it != m_outRecords.end()) {
    m_outRecords.erase(it);
  }
}

} // namespace nfd::pit
