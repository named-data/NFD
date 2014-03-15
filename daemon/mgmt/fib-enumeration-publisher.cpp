/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "fib-enumeration-publisher.hpp"

#include "common.hpp"

#include <ndn-cpp-dev/management/nfd-fib-entry.hpp>

namespace nfd {

NFD_LOG_INIT("FibEnumerationPublisher");

FibEnumerationPublisher::FibEnumerationPublisher(const Fib& fib,
                                                 shared_ptr<AppFace> face,
                                                 const Name& prefix)
  : SegmentPublisher(face, prefix)
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
