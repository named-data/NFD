/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "face-status-publisher.hpp"
#include "face-flags.hpp"
#include "core/logger.hpp"
#include <ndn-cpp-dev/management/nfd-face-status.hpp>

namespace nfd {

NFD_LOG_INIT("FaceStatusPublisher");


FaceStatusPublisher::FaceStatusPublisher(const FaceTable& faceTable,
                                         shared_ptr<AppFace> face,
                                         const Name& prefix)
  : SegmentPublisher(face, prefix)
  , m_faceTable(faceTable)
{

}


FaceStatusPublisher::~FaceStatusPublisher()
{

}

size_t
FaceStatusPublisher::generate(ndn::EncodingBuffer& outBuffer)
{
  size_t totalLength = 0;

  for (FaceTable::const_reverse_iterator i = m_faceTable.rbegin();
       i != m_faceTable.rend(); ++i) {
    const shared_ptr<Face>& face = *i;
    const FaceCounters& counters = face->getCounters();

    ndn::nfd::FaceStatus status;
    status.setFaceId(face->getId())
          .setRemoteUri(face->getRemoteUri().toString())
          .setLocalUri(face->getLocalUri().toString())
          .setFlags(getFaceFlags(*face))
          .setNInInterests(counters.getNInInterests())
          .setNInDatas(counters.getNInDatas())
          .setNOutInterests(counters.getNOutInterests())
          .setNOutDatas(counters.getNOutDatas());

    totalLength += status.wireEncode(outBuffer);
  }
  return totalLength;
}

} // namespace nfd
