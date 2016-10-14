/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#include "face/lp-reassembler.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace face {
namespace tests {

using namespace nfd::tests;

class LpReassemblerFixture : public UnitTestTimeFixture
{
protected:
  LpReassemblerFixture()
  {
    reassembler.beforeTimeout.connect(
      [this] (Transport::EndpointId remoteEp, size_t nDroppedFragments) {
        timeoutHistory.push_back(std::make_tuple(remoteEp, nDroppedFragments));
      });
  }

protected:
  LpReassembler reassembler;
  std::vector<std::tuple<Transport::EndpointId, size_t>> timeoutHistory;

  static const uint8_t data[10];
};

const uint8_t LpReassemblerFixture::data[10] = {
  0x06, 0x08, // Data
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
};

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestLpReassembler, LpReassemblerFixture)

BOOST_AUTO_TEST_SUITE(SingleFragment)

BOOST_AUTO_TEST_CASE(Normal)
{
  ndn::Buffer dataBuffer(data, sizeof(data));

  lp::Packet received;
  received.add<lp::FragmentField>(std::make_pair(dataBuffer.begin(), dataBuffer.end()));
  received.add<lp::FragIndexField>(0);
  received.add<lp::FragCountField>(1);
  received.add<lp::SequenceField>(1000);
  received.add<lp::NextHopFaceIdField>(200);

  bool isComplete = false;
  Block netPacket;
  lp::Packet packet;
  std::tie(isComplete, netPacket, packet) = reassembler.receiveFragment(0, received);

  BOOST_REQUIRE(isComplete);
  BOOST_CHECK(packet.has<lp::NextHopFaceIdField>());
  BOOST_CHECK_EQUAL_COLLECTIONS(data, data + sizeof(data), netPacket.begin(), netPacket.end());
  BOOST_CHECK_EQUAL(reassembler.size(), 0);
}

BOOST_AUTO_TEST_CASE(OmitFragIndex)
{
  ndn::Buffer dataBuffer(data, sizeof(data));

  lp::Packet received;
  received.add<lp::FragmentField>(std::make_pair(dataBuffer.begin(), dataBuffer.end()));
  received.add<lp::FragCountField>(1);
  received.add<lp::SequenceField>(1000);
  received.add<lp::NextHopFaceIdField>(200);

  bool isComplete = false;
  Block netPacket;
  lp::Packet packet;
  std::tie(isComplete, netPacket, packet) = reassembler.receiveFragment(0, received);

  BOOST_REQUIRE(isComplete);
  BOOST_CHECK(packet.has<lp::NextHopFaceIdField>());
  BOOST_CHECK_EQUAL_COLLECTIONS(data, data + sizeof(data), netPacket.begin(), netPacket.end());
}

BOOST_AUTO_TEST_CASE(OmitFragCount)
{
  ndn::Buffer dataBuffer(data, sizeof(data));

  lp::Packet received;
  received.add<lp::FragmentField>(std::make_pair(dataBuffer.begin(), dataBuffer.end()));
  received.add<lp::FragIndexField>(0);
  received.add<lp::SequenceField>(1000);
  received.add<lp::NextHopFaceIdField>(200);

  bool isComplete = false;
  Block netPacket;
  lp::Packet packet;
  std::tie(isComplete, netPacket, packet) = reassembler.receiveFragment(0, received);

  BOOST_REQUIRE(isComplete);
  BOOST_CHECK(packet.has<lp::NextHopFaceIdField>());
  BOOST_CHECK_EQUAL_COLLECTIONS(data, data + sizeof(data), netPacket.begin(), netPacket.end());
}

BOOST_AUTO_TEST_CASE(OmitFragIndexAndFragCount)
{
  ndn::Buffer dataBuffer(data, sizeof(data));

  lp::Packet received;
  received.add<lp::FragmentField>(std::make_pair(dataBuffer.begin(), dataBuffer.end()));
  received.add<lp::SequenceField>(1000);
  received.add<lp::NextHopFaceIdField>(200);

  bool isComplete = false;
  Block netPacket;
  lp::Packet packet;
  std::tie(isComplete, netPacket, packet) = reassembler.receiveFragment(0, received);

  BOOST_REQUIRE(isComplete);
  BOOST_CHECK(packet.has<lp::NextHopFaceIdField>());
  BOOST_CHECK_EQUAL_COLLECTIONS(data, data + sizeof(data), netPacket.begin(), netPacket.end());
}

BOOST_AUTO_TEST_SUITE_END() // SingleFragment

BOOST_AUTO_TEST_SUITE(MultiFragment)

BOOST_AUTO_TEST_CASE(Normal)
{
  ndn::Buffer data1Buffer(data, 4);
  ndn::Buffer data2Buffer(data + 4, 4);
  ndn::Buffer data3Buffer(data + 8, 2);

  lp::Packet received1;
  received1.add<lp::FragmentField>(std::make_pair(data1Buffer.begin(), data1Buffer.end()));
  received1.add<lp::FragIndexField>(0);
  received1.add<lp::FragCountField>(3);
  received1.add<lp::SequenceField>(1000);
  received1.add<lp::NextHopFaceIdField>(200);

  lp::Packet received2;
  received2.add<lp::FragmentField>(std::make_pair(data2Buffer.begin(), data2Buffer.end()));
  received2.add<lp::FragIndexField>(1);
  received2.add<lp::FragCountField>(3);
  received2.add<lp::SequenceField>(1001);

  lp::Packet received3;
  received3.add<lp::FragmentField>(std::make_pair(data3Buffer.begin(), data3Buffer.end()));
  received3.add<lp::FragIndexField>(2);
  received3.add<lp::FragCountField>(3);
  received3.add<lp::SequenceField>(1002);

  bool isComplete = false;
  Block netPacket;
  lp::Packet packet;

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(0, received1);
  BOOST_REQUIRE(!isComplete);
  BOOST_CHECK_EQUAL(reassembler.size(), 1);

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(0, received2);
  BOOST_REQUIRE(!isComplete);
  BOOST_CHECK_EQUAL(reassembler.size(), 1);

  std::tie(isComplete, netPacket, packet) = reassembler.receiveFragment(0, received3);
  BOOST_REQUIRE(isComplete);
  BOOST_CHECK(packet.has<lp::NextHopFaceIdField>());
  BOOST_CHECK_EQUAL_COLLECTIONS(data, data + sizeof(data), netPacket.begin(), netPacket.end());
  BOOST_CHECK_EQUAL(reassembler.size(), 0);
}

BOOST_AUTO_TEST_CASE(OmitFragIndex0)
{
  ndn::Buffer data1Buffer(data, 4);
  ndn::Buffer data2Buffer(data + 4, 4);
  ndn::Buffer data3Buffer(data + 8, 2);

  lp::Packet received1;
  received1.add<lp::FragmentField>(std::make_pair(data1Buffer.begin(), data1Buffer.end()));
  received1.add<lp::FragCountField>(3);
  received1.add<lp::SequenceField>(1000);
  received1.add<lp::NextHopFaceIdField>(200);

  lp::Packet received2;
  received2.add<lp::FragmentField>(std::make_pair(data2Buffer.begin(), data2Buffer.end()));
  received2.add<lp::FragIndexField>(1);
  received2.add<lp::FragCountField>(3);
  received2.add<lp::SequenceField>(1001);

  lp::Packet received3;
  received3.add<lp::FragmentField>(std::make_pair(data3Buffer.begin(), data3Buffer.end()));
  received3.add<lp::FragIndexField>(2);
  received3.add<lp::FragCountField>(3);
  received3.add<lp::SequenceField>(1002);

  bool isComplete = false;
  Block netPacket;
  lp::Packet packet;

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(0, received1);
  BOOST_REQUIRE(!isComplete);

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(0, received2);
  BOOST_REQUIRE(!isComplete);

  std::tie(isComplete, netPacket, packet) = reassembler.receiveFragment(0, received3);
  BOOST_REQUIRE(isComplete);
  BOOST_CHECK(packet.has<lp::NextHopFaceIdField>());
  BOOST_CHECK_EQUAL_COLLECTIONS(data, data + sizeof(data), netPacket.begin(), netPacket.end());
}

BOOST_AUTO_TEST_CASE(OutOfOrder)
{
  ndn::Buffer data0Buffer(data, 4);
  ndn::Buffer data1Buffer(data + 4, 4);
  ndn::Buffer data2Buffer(data + 8, 2);

  lp::Packet frag0;
  frag0.add<lp::FragmentField>(std::make_pair(data0Buffer.begin(), data0Buffer.end()));
  frag0.add<lp::FragIndexField>(0);
  frag0.add<lp::FragCountField>(3);
  frag0.add<lp::SequenceField>(1000);
  frag0.add<lp::NextHopFaceIdField>(200);

  lp::Packet frag1;
  frag1.add<lp::FragmentField>(std::make_pair(data1Buffer.begin(), data1Buffer.end()));
  frag1.add<lp::FragIndexField>(1);
  frag1.add<lp::FragCountField>(3);
  frag1.add<lp::SequenceField>(1001);

  lp::Packet frag2;
  frag2.add<lp::FragmentField>(std::make_pair(data2Buffer.begin(), data2Buffer.end()));
  frag2.add<lp::FragIndexField>(2);
  frag2.add<lp::FragCountField>(3);
  frag2.add<lp::SequenceField>(1002);

  bool isComplete = false;

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(0, frag2);
  BOOST_REQUIRE(!isComplete);

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(0, frag0);
  BOOST_REQUIRE(!isComplete);

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(0, frag1);
  BOOST_REQUIRE(isComplete);
}

BOOST_AUTO_TEST_CASE(Duplicate)
{
  ndn::Buffer data0Buffer(data, 5);

  lp::Packet frag0;
  frag0.add<lp::FragmentField>(std::make_pair(data0Buffer.begin(), data0Buffer.end()));
  frag0.add<lp::FragIndexField>(0);
  frag0.add<lp::FragCountField>(2);
  frag0.add<lp::SequenceField>(1000);

  bool isComplete = false;

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(0, frag0);
  BOOST_REQUIRE(!isComplete);

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(1, frag0);
  BOOST_REQUIRE(!isComplete);
}

BOOST_AUTO_TEST_CASE(Timeout)
{
  ndn::Buffer data1Buffer(data, 5);
  ndn::Buffer data2Buffer(data + 5, 5);

  lp::Packet received1;
  received1.add<lp::FragmentField>(std::make_pair(data1Buffer.begin(), data1Buffer.end()));
  received1.add<lp::FragIndexField>(0);
  received1.add<lp::FragCountField>(2);
  received1.add<lp::SequenceField>(1000);
  received1.add<lp::NextHopFaceIdField>(200);

  lp::Packet received2;
  received2.add<lp::FragmentField>(std::make_pair(data2Buffer.begin(), data2Buffer.end()));
  received2.add<lp::FragIndexField>(1);
  received2.add<lp::FragCountField>(2);
  received2.add<lp::SequenceField>(1001);

  const Transport::EndpointId REMOTE_EP = 11028;
  bool isComplete = false;
  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(REMOTE_EP, received1);
  BOOST_REQUIRE(!isComplete);
  BOOST_CHECK_EQUAL(reassembler.size(), 1);
  BOOST_CHECK(timeoutHistory.empty());

  advanceClocks(time::milliseconds(1), 600);
  BOOST_CHECK_EQUAL(reassembler.size(), 0);
  BOOST_REQUIRE_EQUAL(timeoutHistory.size(), 1);
  BOOST_CHECK_EQUAL(std::get<0>(timeoutHistory.back()), REMOTE_EP);
  BOOST_CHECK_EQUAL(std::get<1>(timeoutHistory.back()), 1);

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(REMOTE_EP, received2);
  BOOST_REQUIRE(!isComplete);
}

BOOST_AUTO_TEST_CASE(MissingSequence)
{
  ndn::Buffer data1Buffer(data, 4);
  ndn::Buffer data2Buffer(data + 4, 4);
  ndn::Buffer data3Buffer(data + 8, 2);

  lp::Packet received1;
  received1.add<lp::FragmentField>(std::make_pair(data1Buffer.begin(), data1Buffer.end()));
  received1.add<lp::FragIndexField>(0);
  received1.add<lp::FragCountField>(3);
  received1.add<lp::SequenceField>(1000);
  received1.add<lp::NextHopFaceIdField>(200);

  lp::Packet received2;
  received2.add<lp::FragmentField>(std::make_pair(data2Buffer.begin(), data2Buffer.end()));
  received2.add<lp::FragIndexField>(1);
  received2.add<lp::FragCountField>(3);

  lp::Packet received3;
  received3.add<lp::FragmentField>(std::make_pair(data3Buffer.begin(), data3Buffer.end()));
  received3.add<lp::FragIndexField>(2);
  received3.add<lp::FragCountField>(3);
  received3.add<lp::SequenceField>(1002);

  bool isComplete = false;

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(0, received1);
  BOOST_REQUIRE(!isComplete);

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(0, received2);
  BOOST_REQUIRE(!isComplete);

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(0, received3);
  BOOST_REQUIRE(!isComplete);

  advanceClocks(time::milliseconds(1), 600);

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(0, received2);
  BOOST_REQUIRE(!isComplete);
}

BOOST_AUTO_TEST_CASE(FragCountOverLimit)
{
  ndn::Buffer data1Buffer(data, sizeof(data));

  lp::Packet received1;
  received1.add<lp::FragmentField>(std::make_pair(data1Buffer.begin(), data1Buffer.end()));
  received1.add<lp::FragIndexField>(0);
  received1.add<lp::FragCountField>(256);
  received1.add<lp::SequenceField>(1000);
  received1.add<lp::NextHopFaceIdField>(200);

  bool isComplete = false;

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(0, received1);
  BOOST_REQUIRE(!isComplete);
}

BOOST_AUTO_TEST_CASE(MissingFragCount)
{
  ndn::Buffer data1Buffer(data, 4);
  ndn::Buffer data2Buffer(data + 4, 4);
  ndn::Buffer data3Buffer(data + 8, 2);

  lp::Packet received1;
  received1.add<lp::FragmentField>(std::make_pair(data1Buffer.begin(), data1Buffer.end()));
  received1.add<lp::FragIndexField>(0);
  received1.add<lp::FragCountField>(3);
  received1.add<lp::SequenceField>(1000);
  received1.add<lp::NextHopFaceIdField>(200);

  lp::Packet received2;
  received2.add<lp::FragmentField>(std::make_pair(data2Buffer.begin(), data2Buffer.end()));
  received2.add<lp::FragIndexField>(1);
  received2.add<lp::FragCountField>(50);
  received2.add<lp::SequenceField>(1001);

  lp::Packet received3;
  received3.add<lp::FragmentField>(std::make_pair(data3Buffer.begin(), data3Buffer.end()));
  received3.add<lp::FragIndexField>(2);
  received3.add<lp::FragCountField>(3);
  received3.add<lp::SequenceField>(1002);

  bool isComplete = false;

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(0, received1);
  BOOST_REQUIRE(!isComplete);

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(0, received2);
  BOOST_REQUIRE(!isComplete);

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(0, received3);
  BOOST_REQUIRE(!isComplete);
}

BOOST_AUTO_TEST_CASE(OverFragCount)
{
  LpReassembler::Options options;
  options.nMaxFragments = 2;
  reassembler.setOptions(options);

  ndn::Buffer data1Buffer(data, 4);
  ndn::Buffer data2Buffer(data + 4, 4);
  ndn::Buffer data3Buffer(data + 8, 2);

  lp::Packet received1;
  received1.add<lp::FragmentField>(std::make_pair(data1Buffer.begin(), data1Buffer.end()));
  received1.add<lp::FragIndexField>(0);
  received1.add<lp::FragCountField>(3);
  received1.add<lp::SequenceField>(1000);
  received1.add<lp::NextHopFaceIdField>(200);

  lp::Packet received2;
  received2.add<lp::FragmentField>(std::make_pair(data2Buffer.begin(), data2Buffer.end()));
  received2.add<lp::FragIndexField>(1);
  received2.add<lp::FragCountField>(3);
  received2.add<lp::SequenceField>(1001);

  lp::Packet received3;
  received3.add<lp::FragmentField>(std::make_pair(data3Buffer.begin(), data3Buffer.end()));
  received3.add<lp::FragIndexField>(2);
  received3.add<lp::FragCountField>(3);
  received3.add<lp::SequenceField>(1002);

  bool isComplete = false;

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(0, received1);
  BOOST_REQUIRE(!isComplete);

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(0, received2);
  BOOST_REQUIRE(!isComplete);

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(0, received3);
  BOOST_REQUIRE(!isComplete);
}

BOOST_AUTO_TEST_SUITE_END() // MultiFragment

BOOST_AUTO_TEST_SUITE(MultipleRemoteEndpoints)

BOOST_AUTO_TEST_CASE(Normal)
{
  ndn::Buffer data1Buffer(data, 5);
  ndn::Buffer data2Buffer(data + 5, 5);

  lp::Packet frag1_1;
  frag1_1.add<lp::FragmentField>(std::make_pair(data1Buffer.begin(), data1Buffer.end()));
  frag1_1.add<lp::FragIndexField>(0);
  frag1_1.add<lp::FragCountField>(2);
  frag1_1.add<lp::SequenceField>(2000);
  frag1_1.add<lp::NextHopFaceIdField>(200);

  lp::Packet frag1_2;
  frag1_2.add<lp::FragmentField>(std::make_pair(data2Buffer.begin(), data2Buffer.end()));
  frag1_2.add<lp::FragIndexField>(1);
  frag1_2.add<lp::FragCountField>(2);
  frag1_2.add<lp::SequenceField>(2001);

  lp::Packet frag2_1;
  frag2_1.add<lp::FragmentField>(std::make_pair(data1Buffer.begin(), data1Buffer.end()));
  frag2_1.add<lp::FragIndexField>(0);
  frag2_1.add<lp::FragCountField>(2);
  frag2_1.add<lp::SequenceField>(2000);
  frag2_1.add<lp::NextHopFaceIdField>(200);

  lp::Packet frag2_2;
  frag2_2.add<lp::FragmentField>(std::make_pair(data2Buffer.begin(), data2Buffer.end()));
  frag2_2.add<lp::FragIndexField>(1);
  frag2_2.add<lp::FragCountField>(2);
  frag2_2.add<lp::SequenceField>(2001);

  bool isComplete = false;

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(1, frag1_1);
  BOOST_REQUIRE(!isComplete);
  BOOST_CHECK_EQUAL(reassembler.size(), 1);

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(2, frag2_2);
  BOOST_REQUIRE(!isComplete);
  BOOST_CHECK_EQUAL(reassembler.size(), 2);

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(1, frag1_2);
  BOOST_REQUIRE(isComplete);
  BOOST_CHECK_EQUAL(reassembler.size(), 1);

  std::tie(isComplete, std::ignore, std::ignore) = reassembler.receiveFragment(2, frag2_1);
  BOOST_REQUIRE(isComplete);
  BOOST_CHECK_EQUAL(reassembler.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // MultipleRemoteEndpoints

BOOST_AUTO_TEST_SUITE_END() // TestLpReassembler
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
