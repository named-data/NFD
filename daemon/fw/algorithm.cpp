/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#include "algorithm.hpp"

namespace nfd {
namespace scope_prefix {
const Name LOCALHOST("ndn:/localhost");
const Name LOCALHOP("ndn:/localhop");
} // namespace scope_prefix

namespace fw {

bool
wouldViolateScope(const Face& inFace, const Interest& interest, const Face& outFace)
{
  if (outFace.getScope() == ndn::nfd::FACE_SCOPE_LOCAL) {
    // forwarding to a local face is always allowed
    return false;
  }

  if (scope_prefix::LOCALHOST.isPrefixOf(interest.getName())) {
    // localhost Interests cannot be forwarded to a non-local face
    return true;
  }

  if (scope_prefix::LOCALHOP.isPrefixOf(interest.getName())) {
    // localhop Interests can be forwarded to a non-local face only if it comes from a local face
    return inFace.getScope() != ndn::nfd::FACE_SCOPE_LOCAL;
  }

  // Interest name is not subject to scope control
  return false;
}

bool
canForwardToLegacy(const pit::Entry& pitEntry, const Face& face)
{
  time::steady_clock::TimePoint now = time::steady_clock::now();

  bool hasUnexpiredOutRecord = std::any_of(pitEntry.out_begin(), pitEntry.out_end(),
    [&face, &now] (const pit::OutRecord& outRecord) {
      return &outRecord.getFace() == &face && outRecord.getExpiry() >= now;
    });
  if (hasUnexpiredOutRecord) {
    return false;
  }

  bool hasUnexpiredOtherInRecord = std::any_of(pitEntry.in_begin(), pitEntry.in_end(),
    [&face, &now] (const pit::InRecord& inRecord) {
      return &inRecord.getFace() != &face && inRecord.getExpiry() >= now;
    });
  if (!hasUnexpiredOtherInRecord) {
    return false;
  }

  return true;
}

int
findDuplicateNonce(const pit::Entry& pitEntry, uint32_t nonce, const Face& face)
{
  int dnw = DUPLICATE_NONCE_NONE;

  for (const pit::InRecord& inRecord : pitEntry.getInRecords()) {
    if (inRecord.getLastNonce() == nonce) {
      if (&inRecord.getFace() == &face) {
        dnw |= DUPLICATE_NONCE_IN_SAME;
      }
      else {
        dnw |= DUPLICATE_NONCE_IN_OTHER;
      }
    }
  }

  for (const pit::OutRecord& outRecord : pitEntry.getOutRecords()) {
    if (outRecord.getLastNonce() == nonce) {
      if (&outRecord.getFace() == &face) {
        dnw |= DUPLICATE_NONCE_OUT_SAME;
      }
      else {
        dnw |= DUPLICATE_NONCE_OUT_OTHER;
      }
    }
  }

  return dnw;
}

bool
hasPendingOutRecords(const pit::Entry& pitEntry)
{
  time::steady_clock::TimePoint now = time::steady_clock::now();
  return std::any_of(pitEntry.out_begin(), pitEntry.out_end(),
                      [&now] (const pit::OutRecord& outRecord) {
                        return outRecord.getExpiry() >= now &&
                               outRecord.getIncomingNack() == nullptr;
                      });
}

time::steady_clock::TimePoint
getLastOutgoing(const pit::Entry& pitEntry)
{
  pit::OutRecordCollection::const_iterator lastOutgoing = std::max_element(
    pitEntry.out_begin(), pitEntry.out_end(),
    [] (const pit::OutRecord& a, const pit::OutRecord& b) {
      return a.getLastRenewed() < b.getLastRenewed();
    });
  BOOST_ASSERT(lastOutgoing != pitEntry.out_end());

  return lastOutgoing->getLastRenewed();
}

} // namespace fw
} // namespace nfd
