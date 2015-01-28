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

#ifndef NFD_CORE_SEGMENT_PUBLISHER_HPP
#define NFD_CORE_SEGMENT_PUBLISHER_HPP

#include "common.hpp"

#include <ndn-cxx/encoding/encoding-buffer.hpp>
#include <ndn-cxx/security/key-chain.hpp>

namespace nfd {

/** \brief provides a publisher of Status Dataset or other segmented octet stream
 *  \sa http://redmine.named-data.net/projects/nfd/wiki/StatusDataset
 */
template <class FaceBase>
class SegmentPublisher : noncopyable
{
public:
  SegmentPublisher(FaceBase& face,
                   const Name& prefix,
                   ndn::KeyChain& keyChain,
                   const time::milliseconds& freshnessPeriod = getDefaultFreshness())
    : m_face(face)
    , m_prefix(prefix)
    , m_keyChain(keyChain)
    , m_freshnessPeriod(freshnessPeriod)
  {
  }

  virtual
  ~SegmentPublisher()
  {
  }

  static size_t
  getMaxSegmentSize()
  {
    static const size_t MAX_SEGMENT_SIZE = ndn::MAX_NDN_PACKET_SIZE >> 1;
    return MAX_SEGMENT_SIZE;
  }

  static constexpr time::milliseconds
  getDefaultFreshness()
  {
    return time::milliseconds(1000);
  }

  void
  publish()
  {
    ndn::EncodingBuffer buffer;
    generate(buffer);

    const uint8_t* rawBuffer = buffer.buf();
    const uint8_t* segmentBegin = rawBuffer;
    const uint8_t* end = rawBuffer + buffer.size();

    Name segmentPrefix(m_prefix);
    segmentPrefix.appendVersion();

    uint64_t segmentNo = 0;
    do {
      const uint8_t* segmentEnd = segmentBegin + getMaxSegmentSize();
      if (segmentEnd > end) {
        segmentEnd = end;
      }

      Name segmentName(segmentPrefix);
      segmentName.appendSegment(segmentNo);

      shared_ptr<Data> data = make_shared<Data>(segmentName);
      data->setContent(segmentBegin, segmentEnd - segmentBegin);
      data->setFreshnessPeriod(m_freshnessPeriod);

      segmentBegin = segmentEnd;
      if (segmentBegin >= end) {
        data->setFinalBlockId(segmentName[-1]);
      }

      publishSegment(data);
      ++segmentNo;
    } while (segmentBegin < end);
  }

protected:
  /** \brief In a derived class, write the octets into outBuffer.
   */
  virtual size_t
  generate(ndn::EncodingBuffer& outBuffer) = 0;

private:
  void
  publishSegment(shared_ptr<Data>& data)
  {
    m_keyChain.sign(*data);
    m_face.put(*data);
  }

private:
  FaceBase& m_face;
  const Name m_prefix;
  ndn::KeyChain& m_keyChain;
  const time::milliseconds m_freshnessPeriod;
};

} // namespace nfd

#endif // NFD_CORE_SEGMENT_PUBLISHER_HPP
