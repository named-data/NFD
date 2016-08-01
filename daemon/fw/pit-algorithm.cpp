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

#include "pit-algorithm.hpp"

namespace nfd {
namespace scope_prefix {
const Name LOCALHOST("ndn:/localhost");
const Name LOCALHOP("ndn:/localhop");
} // namespace scope_prefix

namespace fw {

bool
violatesScope(const pit::Entry& pitEntry, const Face& outFace)
{
  if (outFace.getScope() == ndn::nfd::FACE_SCOPE_LOCAL) {
    return false;
  }
  BOOST_ASSERT(outFace.getScope() == ndn::nfd::FACE_SCOPE_NON_LOCAL);

  if (scope_prefix::LOCALHOST.isPrefixOf(pitEntry.getName())) {
    // face is non-local, violates localhost scope
    return true;
  }

  if (scope_prefix::LOCALHOP.isPrefixOf(pitEntry.getName())) {
    // face is non-local, violates localhop scope unless PIT entry has local in-record
    return std::none_of(pitEntry.in_begin(), pitEntry.in_end(),
      [] (const pit::InRecord& inRecord) { return inRecord.getFace().getScope() == ndn::nfd::FACE_SCOPE_LOCAL; });
  }

  // Name is not subject to scope control
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

  return !violatesScope(pitEntry, face);
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

} // namespace fw
} // namespace nfd
