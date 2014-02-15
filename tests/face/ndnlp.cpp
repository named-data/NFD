/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "face/ndnlp-sequence-generator.hpp"
#include "face/ndnlp-slicer.hpp"

#include <boost/test/unit_test.hpp>

namespace nfd {

BOOST_AUTO_TEST_SUITE(FaceNdnlp)

BOOST_AUTO_TEST_CASE(SequenceBlock)
{
  ndnlp::SequenceBlock sb(0x8000, 2);
  BOOST_CHECK_EQUAL(sb.count(), 2);
  BOOST_CHECK_EQUAL(sb[0], 0x8000);
  BOOST_CHECK_EQUAL(sb[1], 0x8001);
  BOOST_CHECK_THROW(sb[2], std::out_of_range);
}

// sequence number can safely wrap around
BOOST_AUTO_TEST_CASE(SequenceBlockWrap)
{
  ndnlp::SequenceBlock sb(std::numeric_limits<uint64_t>::max(), 2);
  BOOST_CHECK_EQUAL(sb[0], std::numeric_limits<uint64_t>::max());
  BOOST_CHECK_EQUAL(sb[1], std::numeric_limits<uint64_t>::min());
  BOOST_CHECK_EQUAL(sb[1] - sb[0], 1);
}

BOOST_AUTO_TEST_CASE(SequenceGenerator)
{
  ndnlp::SequenceGenerator seqgen;
  
  ndnlp::SequenceBlock sb1 = seqgen.nextBlock(2);
  BOOST_CHECK_EQUAL(sb1.count(), 2);

  ndnlp::SequenceBlock sb2 = seqgen.nextBlock(1);
  BOOST_CHECK_NE(sb1[0], sb2[0]);
  BOOST_CHECK_NE(sb1[1], sb2[0]);
}

// slice a Block to one NDNLP packet
BOOST_AUTO_TEST_CASE(Slice1)
{
  uint8_t blockValue[60];
  memset(blockValue, 0xcc, sizeof(blockValue));
  Block block = ndn::dataBlock(0x01, blockValue, sizeof(blockValue));
  
  ndnlp::Slicer slicer(9000);
  ndnlp::PacketArray pa = slicer.slice(block);
  
  BOOST_REQUIRE_EQUAL(pa->size(), 1);
  
  const Block& pkt = pa->at(0);
  BOOST_CHECK_EQUAL(pkt.type(), static_cast<uint32_t>(tlv::NdnlpData));
  pkt.parse();
  
  const Block::element_container& elements = pkt.elements();
  BOOST_REQUIRE_EQUAL(elements.size(), 2);
  
  const Block& sequenceElement = elements[0];
  BOOST_CHECK_EQUAL(sequenceElement.type(), static_cast<uint32_t>(tlv::NdnlpSequence));
  BOOST_REQUIRE_EQUAL(sequenceElement.value_size(), sizeof(uint64_t));
  
  const Block& payloadElement = elements[1];
  BOOST_CHECK_EQUAL(payloadElement.type(), static_cast<uint32_t>(tlv::NdnlpPayload));
  size_t payloadSize = payloadElement.value_size();
  BOOST_CHECK_EQUAL(payloadSize, block.size());
  
  BOOST_CHECK_EQUAL_COLLECTIONS(payloadElement.value_begin(), payloadElement.value_end(),
                                block.begin(),                block.end());
}

// slice a Block to four NDNLP packets
BOOST_AUTO_TEST_CASE(SliceData4)
{
  uint8_t blockValue[5050];
  memset(blockValue, 0xcc, sizeof(blockValue));
  Block block = ndn::dataBlock(0x01, blockValue, sizeof(blockValue));
  
  ndnlp::Slicer slicer(1500);
  ndnlp::PacketArray pa = slicer.slice(block);
  
  BOOST_REQUIRE_EQUAL(pa->size(), 4);
  
  uint64_t seq0 = 0xdddd;
  
  size_t totalPayloadSize = 0;
  
  for (size_t i = 0; i < 4; ++i) {
    const Block& pkt = pa->at(i);
    BOOST_CHECK_EQUAL(pkt.type(), static_cast<uint32_t>(tlv::NdnlpData));
    pkt.parse();
    
    const Block::element_container& elements = pkt.elements();
    BOOST_REQUIRE_EQUAL(elements.size(), 4);
    
    const Block& sequenceElement = elements[0];
    BOOST_CHECK_EQUAL(sequenceElement.type(), static_cast<uint32_t>(tlv::NdnlpSequence));
    BOOST_REQUIRE_EQUAL(sequenceElement.value_size(), sizeof(uint64_t));
    uint64_t seq = be64toh(*reinterpret_cast<const uint64_t*>(
                             &*sequenceElement.value_begin()));
    if (i == 0) {
      seq0 = seq;
    }
    BOOST_CHECK_EQUAL(seq, seq0 + i);
    
    const Block& fragIndexElement = elements[1];
    BOOST_CHECK_EQUAL(fragIndexElement.type(), static_cast<uint32_t>(tlv::NdnlpFragIndex));
    BOOST_REQUIRE_EQUAL(fragIndexElement.value_size(), sizeof(uint16_t));
    uint16_t fragIndex = be16toh(*reinterpret_cast<const uint16_t*>(
                                   &*fragIndexElement.value_begin()));
    BOOST_CHECK_EQUAL(fragIndex, i);
    
    const Block& fragCountElement = elements[2];
    BOOST_CHECK_EQUAL(fragCountElement.type(), static_cast<uint32_t>(tlv::NdnlpFragCount));
    BOOST_REQUIRE_EQUAL(fragCountElement.value_size(), sizeof(uint16_t));
    uint16_t fragCount = be16toh(*reinterpret_cast<const uint16_t*>(
                                   &*fragCountElement.value_begin()));
    BOOST_CHECK_EQUAL(fragCount, 4);
    
    const Block& payloadElement = elements[3];
    BOOST_CHECK_EQUAL(payloadElement.type(), static_cast<uint32_t>(tlv::NdnlpPayload));
    size_t payloadSize = payloadElement.value_size();
    totalPayloadSize += payloadSize;
  }
  
  BOOST_CHECK_EQUAL(totalPayloadSize, block.size());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace nfd
