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

#include "face/ndnlp-sequence-generator.hpp"
#include "face/ndnlp-slicer.hpp"
#include "face/ndnlp-partial-message-store.hpp"

#include "tests/test-common.hpp"

#include <boost/scoped_array.hpp>

namespace nfd {
namespace ndnlp {
namespace tests {

using namespace nfd::tests;

BOOST_FIXTURE_TEST_SUITE(FaceNdnlp, BaseFixture)

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

// slice a Block to one fragment
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

// slice a Block to four fragments
BOOST_AUTO_TEST_CASE(Slice4)
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
    uint64_t fragIndex = ndn::readNonNegativeInteger(fragIndexElement);
    BOOST_CHECK_EQUAL(fragIndex, i);

    const Block& fragCountElement = elements[2];
    BOOST_CHECK_EQUAL(fragCountElement.type(), static_cast<uint32_t>(tlv::NdnlpFragCount));
    uint64_t fragCount = ndn::readNonNegativeInteger(fragCountElement);
    BOOST_CHECK_EQUAL(fragCount, 4);

    const Block& payloadElement = elements[3];
    BOOST_CHECK_EQUAL(payloadElement.type(), static_cast<uint32_t>(tlv::NdnlpPayload));
    size_t payloadSize = payloadElement.value_size();
    totalPayloadSize += payloadSize;
  }

  BOOST_CHECK_EQUAL(totalPayloadSize, block.size());
}

class ReassembleFixture : protected UnitTestTimeFixture
{
protected:
  ReassembleFixture()
    : slicer(1500)
    , pms(time::milliseconds(100))
  {
    pms.onReceive.connect([this] (const Block& block) {
      received.push_back(block);
    });
  }

  Block
  makeBlock(size_t valueLength)
  {
    boost::scoped_array<uint8_t> blockValue(new uint8_t[valueLength]);
    memset(blockValue.get(), 0xcc, valueLength);
    return ndn::dataBlock(0x01, blockValue.get(), valueLength);
  }

  void
  receiveNdnlpData(const Block& block)
  {
    bool isOk = false;
    ndnlp::NdnlpData pkt;
    std::tie(isOk, pkt) = ndnlp::NdnlpData::fromBlock(block);
    BOOST_REQUIRE(isOk);
    pms.receive(pkt);
  }

protected:
  ndnlp::Slicer slicer;
  ndnlp::PartialMessageStore pms;

  static const time::nanoseconds IDLE_DURATION;

  // received network layer packets
  std::vector<Block> received;
};

// reassemble one fragment into one Block
BOOST_FIXTURE_TEST_CASE(Reassemble1, ReassembleFixture)
{
  Block block = makeBlock(60);
  ndnlp::PacketArray pa = slicer.slice(block);
  BOOST_REQUIRE_EQUAL(pa->size(), 1);

  BOOST_CHECK_EQUAL(received.size(), 0);
  this->receiveNdnlpData(pa->at(0));

  BOOST_REQUIRE_EQUAL(received.size(), 1);
  BOOST_CHECK_EQUAL_COLLECTIONS(received.at(0).begin(), received.at(0).end(),
                                block.begin(),          block.end());
}

// reassemble four and two fragments into two Blocks
BOOST_FIXTURE_TEST_CASE(Reassemble4and2, ReassembleFixture)
{
  Block block = makeBlock(5050);
  ndnlp::PacketArray pa = slicer.slice(block);
  BOOST_REQUIRE_EQUAL(pa->size(), 4);

  Block block2 = makeBlock(2000);
  ndnlp::PacketArray pa2 = slicer.slice(block2);
  BOOST_REQUIRE_EQUAL(pa2->size(), 2);

  BOOST_CHECK_EQUAL(received.size(), 0);
  this->receiveNdnlpData(pa->at(0));
  BOOST_CHECK_EQUAL(received.size(), 0);
  this->advanceClocks(time::milliseconds(40));

  this->receiveNdnlpData(pa->at(1));
  BOOST_CHECK_EQUAL(received.size(), 0);
  this->advanceClocks(time::milliseconds(40));

  this->receiveNdnlpData(pa2->at(1));
  BOOST_CHECK_EQUAL(received.size(), 0);
  this->advanceClocks(time::milliseconds(40));

  this->receiveNdnlpData(pa->at(1));
  BOOST_CHECK_EQUAL(received.size(), 0);
  this->advanceClocks(time::milliseconds(40));

  this->receiveNdnlpData(pa2->at(0));
  BOOST_CHECK_EQUAL(received.size(), 1);
  this->advanceClocks(time::milliseconds(40));

  this->receiveNdnlpData(pa->at(3));
  BOOST_CHECK_EQUAL(received.size(), 1);
  this->advanceClocks(time::milliseconds(40));

  this->receiveNdnlpData(pa->at(2));

  BOOST_REQUIRE_EQUAL(received.size(), 2);
  BOOST_CHECK_EQUAL_COLLECTIONS(received.at(1).begin(), received.at(1).end(),
                                block.begin(),          block.end());
  BOOST_CHECK_EQUAL_COLLECTIONS(received.at(0).begin(), received.at(0).end(),
                                block2.begin(),         block2.end());
}

// reassemble four fragments into one Block, but another two fragments are expired
BOOST_FIXTURE_TEST_CASE(ReassembleTimeout, ReassembleFixture)
{
  Block block = makeBlock(5050);
  ndnlp::PacketArray pa = slicer.slice(block);
  BOOST_REQUIRE_EQUAL(pa->size(), 4);

  Block block2 = makeBlock(2000);
  ndnlp::PacketArray pa2 = slicer.slice(block2);
  BOOST_REQUIRE_EQUAL(pa2->size(), 2);

  BOOST_CHECK_EQUAL(received.size(), 0);
  this->receiveNdnlpData(pa->at(0));
  BOOST_CHECK_EQUAL(received.size(), 0);
  this->advanceClocks(time::milliseconds(40));

  this->receiveNdnlpData(pa->at(1));
  BOOST_CHECK_EQUAL(received.size(), 0);
  this->advanceClocks(time::milliseconds(40));

  this->receiveNdnlpData(pa2->at(1));
  BOOST_CHECK_EQUAL(received.size(), 0);
  this->advanceClocks(time::milliseconds(40));

  this->receiveNdnlpData(pa->at(1));
  BOOST_CHECK_EQUAL(received.size(), 0);
  this->advanceClocks(time::milliseconds(40));

  this->receiveNdnlpData(pa->at(3));
  BOOST_CHECK_EQUAL(received.size(), 0);
  this->advanceClocks(time::milliseconds(40));

  this->receiveNdnlpData(pa->at(2));
  BOOST_CHECK_EQUAL(received.size(), 1);
  this->advanceClocks(time::milliseconds(40));

  this->receiveNdnlpData(pa2->at(0)); // last fragment was received 160ms ago, expired
  BOOST_CHECK_EQUAL(received.size(), 1);
  this->advanceClocks(time::milliseconds(40));

  BOOST_REQUIRE_EQUAL(received.size(), 1);
  BOOST_CHECK_EQUAL_COLLECTIONS(received.at(0).begin(), received.at(0).end(),
                                block.begin(),          block.end());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndnlp
} // namespace nfd
