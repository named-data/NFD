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

#include "retx-suppression.hpp"

namespace nfd {
namespace fw {

time::steady_clock::TimePoint
RetxSuppression::getLastOutgoing(const pit::Entry& pitEntry) const
{
  pit::OutRecordCollection::const_iterator lastOutgoing = std::max_element(
    pitEntry.out_begin(), pitEntry.out_end(),
    [] (const pit::OutRecord& a, const pit::OutRecord& b) {
      return a.getLastRenewed() < b.getLastRenewed();
    });
  BOOST_ASSERT(lastOutgoing != pitEntry.out_end()); // otherwise it's new PIT entry

  return lastOutgoing->getLastRenewed();
}

} // namespace fw
} // namespace nfd
