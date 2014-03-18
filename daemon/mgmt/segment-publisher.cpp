/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "segment-publisher.hpp"

#include "common.hpp"
#include "face/face.hpp"

#include <ndn-cpp-dev/util/time.hpp>

namespace nfd {

NFD_LOG_INIT("SegmentPublisher");

SegmentPublisher::SegmentPublisher(shared_ptr<AppFace> face,
                                   const Name& prefix)
  : m_face(face)
  , m_prefix(prefix)
{

}


SegmentPublisher::~SegmentPublisher()
{

}

void
SegmentPublisher::publish()
{
  Name segmentPrefix(m_prefix);
  segmentPrefix.appendSegment(time::toUnixTimestamp(time::system_clock::now()).count());

  static const size_t  MAX_SEGMENT_SIZE = MAX_NDN_PACKET_SIZE >> 1;

  ndn::EncodingBuffer buffer;

  generate(buffer);

  const uint8_t* rawBuffer = buffer.buf();
  const uint8_t* segmentBegin = rawBuffer;
  const uint8_t* end = rawBuffer + buffer.size();

  uint64_t segmentNo = 0;
  while (segmentBegin < end)
    {
      const uint8_t* segmentEnd = segmentBegin + MAX_SEGMENT_SIZE;
      if (segmentEnd > end)
        {
          segmentEnd = end;
        }

      Name segmentName(segmentPrefix);
      segmentName.appendSegment(segmentNo);

      shared_ptr<Data> data(make_shared<Data>(segmentName));
      data->setContent(segmentBegin, segmentEnd - segmentBegin);

      segmentBegin = segmentEnd;
      if (segmentBegin >= end)
        {
          NFD_LOG_DEBUG("final block is " << segmentNo);
          data->setFinalBlockId(segmentName[-1]);
        }

      NFD_LOG_DEBUG("publishing segment #" << segmentNo);
      publishSegment(data);
      segmentNo++;
    }
}

void
SegmentPublisher::publishSegment(shared_ptr<Data>& data)
{
  m_face->sign(*data);
  m_face->put(*data);
}

} // namespace nfd
