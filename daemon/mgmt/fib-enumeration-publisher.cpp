/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
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
 **/

#include "fib-enumeration-publisher.hpp"
#include "core/logger.hpp"
#include "table/fib.hpp"

#include <ndn-cxx/management/nfd-fib-entry.hpp>

namespace nfd {

NFD_LOG_INIT("FibEnumerationPublisher");

FibEnumerationPublisher::FibEnumerationPublisher(const Fib& fib,
                                                 AppFace& face,
                                                 const Name& prefix,
                                                 ndn::KeyChain& keyChain)
  : SegmentPublisher(face, prefix, keyChain)
  , m_fib(fib)
{
}

FibEnumerationPublisher::~FibEnumerationPublisher()
{
}

size_t
FibEnumerationPublisher::generate(ndn::EncodingBuffer& outBuffer)
{
  size_t totalLength = 0;

  /// \todo Enable use of Fib::const_reverse_iterator (when it is available)
  for (Fib::const_iterator i = m_fib.begin(); i != m_fib.end(); ++i)
    {
      const fib::Entry& entry = *i;
      const Name& prefix = entry.getPrefix();
      size_t fibEntryLength = 0;

      ndn::nfd::FibEntry tlvEntry;
      const fib::NextHopList& nextHops = entry.getNextHops();

      for (fib::NextHopList::const_iterator j = nextHops.begin();
           j != nextHops.end();
           ++j)
        {
          const fib::NextHop& next = *j;
          ndn::nfd::NextHopRecord nextHopRecord;
          nextHopRecord.setFaceId(next.getFace()->getId());
          nextHopRecord.setCost(next.getCost());

          tlvEntry.addNextHopRecord(nextHopRecord);
        }

      tlvEntry.setPrefix(prefix);
      fibEntryLength += tlvEntry.wireEncode(outBuffer);

      NFD_LOG_DEBUG("generate: fib entry length = " << fibEntryLength);

      totalLength += fibEntryLength;
    }

  NFD_LOG_DEBUG("generate: Total length = " << totalLength);
  return totalLength;
}

} // namespace nfd
