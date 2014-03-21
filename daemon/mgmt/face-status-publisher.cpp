/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */
#include "face-status-publisher.hpp"

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
       i != m_faceTable.rend();
       ++i)
    {
      const shared_ptr<Face>& face = *i;
      const FaceCounters& counters = face->getCounters();
      const std::string uri = face->getUri().toString();

      size_t statusLength = 0;

      statusLength += prependNonNegativeIntegerBlock(outBuffer,
                                                     ndn::tlv::nfd::TotalOutgoingDataCounter,
                                                     counters.getOutData());

      statusLength += prependNonNegativeIntegerBlock(outBuffer,
                                                     ndn::tlv::nfd::TotalOutgoingInterestCounter,
                                                     counters.getOutInterest());

      statusLength += prependNonNegativeIntegerBlock(outBuffer,
                                                     ndn::tlv::nfd::TotalIncomingDataCounter,
                                                     counters.getInData());

      statusLength += prependNonNegativeIntegerBlock(outBuffer,
                                                     ndn::tlv::nfd::TotalIncomingInterestCounter,
                                                     counters.getInInterest());

      statusLength += prependByteArrayBlock(outBuffer,
                                            ndn::tlv::nfd::Uri,
                                            reinterpret_cast<const uint8_t*>(uri.c_str()),
                                            uri.size());

      statusLength += prependNonNegativeIntegerBlock(outBuffer,
                                                     ndn::tlv::nfd::FaceId,
                                                     face->getId());

      statusLength += outBuffer.prependVarNumber(statusLength);
      statusLength += outBuffer.prependVarNumber(ndn::tlv::nfd::FaceStatus);

      totalLength += statusLength;
    }
  return totalLength;
}

} // namespace nfd
