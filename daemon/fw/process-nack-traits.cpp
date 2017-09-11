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

#include "process-nack-traits.hpp"
#include "core/logger.hpp"

namespace nfd {
namespace fw {

NFD_LOG_INIT("ProcessNackTraits");

void
ProcessNackTraitsBase::processNack(const Face& inFace, const lp::Nack& nack,
                                   const shared_ptr<pit::Entry>& pitEntry)
{
  int nOutRecordsNotNacked = 0;
  Face* lastFaceNotNacked = nullptr;
  lp::NackReason leastSevereReason = lp::NackReason::NONE;
  for (const pit::OutRecord& outR : pitEntry->getOutRecords()) {
    const lp::NackHeader* inNack = outR.getIncomingNack();
    if (inNack == nullptr) {
      ++nOutRecordsNotNacked;
      lastFaceNotNacked = &outR.getFace();
      continue;
    }

    if (isLessSevere(inNack->getReason(), leastSevereReason)) {
      leastSevereReason = inNack->getReason();
    }
  }

  lp::NackHeader outNack;
  outNack.setReason(leastSevereReason);

  if (nOutRecordsNotNacked == 1) {
    BOOST_ASSERT(lastFaceNotNacked != nullptr);
    pit::InRecordCollection::iterator inR = pitEntry->getInRecord(*lastFaceNotNacked);
    if (inR != pitEntry->in_end()) {
      // one out-record not Nacked, which is also a downstream
      NFD_LOG_DEBUG(nack.getInterest() << " nack-from=" << inFace.getId() <<
                    " nack=" << nack.getReason() <<
                    " nack-to(bidirectional)=" << lastFaceNotNacked->getId() <<
                    " out-nack=" << outNack.getReason());
      this->sendNackForProcessNackTraits(pitEntry, *lastFaceNotNacked, outNack);
      return;
    }
  }

  if (nOutRecordsNotNacked > 0) {
    NFD_LOG_DEBUG(nack.getInterest() << " nack-from=" << inFace.getId() <<
                  " nack=" << nack.getReason() <<
                  " waiting=" << nOutRecordsNotNacked);
    // continue waiting
    return;
  }

  NFD_LOG_DEBUG(nack.getInterest() << " nack-from=" << inFace.getId() <<
                " nack=" << nack.getReason() <<
                " nack-to=all out-nack=" << outNack.getReason());
  this->sendNacksForProcessNackTraits(pitEntry, outNack);
}

} // namespace fw
} // namespace nfd
