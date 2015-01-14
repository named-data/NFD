/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
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

#include "retransmission-suppression.hpp"

namespace nfd {
namespace fw {

/// \todo don't use fixed interval; make it adaptive or use exponential back-off #1913
const time::milliseconds RetransmissionSuppression::MIN_RETRANSMISSION_INTERVAL(100);

RetransmissionSuppression::Result
RetransmissionSuppression::decide(const Face& inFace, const Interest& interest,
                                  const pit::Entry& pitEntry)
{
  bool isNewPitEntry = !pitEntry.hasUnexpiredOutRecords();
  if (isNewPitEntry) {
    return NEW;
  }

  // when was the last outgoing Interest?
  const pit::OutRecordCollection& outRecords = pitEntry.getOutRecords();
  pit::OutRecordCollection::const_iterator lastOutgoing = std::max_element(
      outRecords.begin(), outRecords.end(),
      [] (const pit::OutRecord& a, const pit::OutRecord& b) {
        return a.getLastRenewed() < b.getLastRenewed();
      });
  BOOST_ASSERT(lastOutgoing != outRecords.end()); // otherwise it's new PIT entry

  time::steady_clock::TimePoint now = time::steady_clock::now();
  time::steady_clock::Duration sinceLastOutgoing = now - lastOutgoing->getLastRenewed();
  bool shouldSuppress = sinceLastOutgoing < MIN_RETRANSMISSION_INTERVAL;
  return shouldSuppress ? SUPPRESS : FORWARD;
}

} // namespace fw
} // namespace nfd
