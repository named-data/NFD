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

#include "segment-publisher.hpp"

#include "core/logger.hpp"
#include "face/face.hpp"

#include <ndn-cxx/util/time.hpp>

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
  segmentPrefix.appendVersion();

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
