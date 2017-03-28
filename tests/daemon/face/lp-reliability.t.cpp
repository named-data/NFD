/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#include "face/lp-reliability.hpp"
#include "face/face.hpp"
#include "face/lp-fragmenter.hpp"
#include "face/generic-link-service.hpp"

#include "tests/test-common.hpp"
#include "dummy-face.hpp"
#include "dummy-transport.hpp"

namespace nfd {
namespace face {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Face)

class DummyLpReliabilityLinkService : public GenericLinkService
{
public:
  LpReliability*
  getLpReliability()
  {
    return &m_reliability;
  }

  void
  sendLpPackets(std::vector<lp::Packet> frags)
  {
    if (frags.front().has<lp::FragmentField>()) {
      m_reliability.observeOutgoing(frags);
    }

    for (lp::Packet& frag : frags) {
      this->sendLpPacket(std::move(frag));
    }
  }

private:
  void
  doSendInterest(const Interest& interest) override
  {
    BOOST_ASSERT(false);
  }

  void
  doSendData(const Data& data) override
  {
    BOOST_ASSERT(false);
  }

  void
  doSendNack(const lp::Nack& nack) override
  {
    BOOST_ASSERT(false);
  }

  void
  doReceivePacket(Transport::Packet&& packet) override
  {
    BOOST_ASSERT(false);
  }
};

class LpReliabilityFixture : public UnitTestTimeFixture
{
public:
  LpReliabilityFixture()
    : linkService(make_unique<DummyLpReliabilityLinkService>())
    , transport(make_unique<DummyTransport>())
    , face(make_unique<DummyFace>())
  {
    linkService->setFaceAndTransport(*face, *transport);
    transport->setFaceAndLinkService(*face, *linkService);

    GenericLinkService::Options options;
    options.reliabilityOptions.isEnabled = true;
    linkService->setOptions(options);

    reliability = linkService->getLpReliability();
  }

public:
  unique_ptr<DummyLpReliabilityLinkService> linkService;
  unique_ptr<DummyTransport> transport;
  unique_ptr<DummyFace> face;
  LpReliability* reliability;
};

BOOST_FIXTURE_TEST_SUITE(TestLpReliability, LpReliabilityFixture)

BOOST_AUTO_TEST_CASE(SendNoFragmentField)
{
  lp::Packet pkt;
  pkt.add<lp::AckField>(0);

  linkService->sendLpPackets({pkt});
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 0);
  BOOST_CHECK_EQUAL(reliability->m_netPkts.size(), 0);
}

BOOST_AUTO_TEST_CASE(SendNotFragmented)
{
  shared_ptr<Interest> interest = makeInterest("/abc/def");

  lp::Packet pkt;
  pkt.add<lp::SequenceField>(123);
  pkt.add<lp::FragmentField>(make_pair(interest->wireEncode().begin(), interest->wireEncode().end()));

  linkService->sendLpPackets({pkt});

  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.size(), 1);
  lp::Packet cached;
  BOOST_REQUIRE_NO_THROW(cached.wireDecode(transport->sentPackets.front().packet));
  BOOST_REQUIRE(cached.has<lp::SequenceField>());
  BOOST_CHECK_EQUAL(cached.get<lp::SequenceField>(), 123);
  lp::Sequence seq = cached.get<lp::SequenceField>();
  ndn::Buffer::const_iterator begin, end;
  std::tie(begin, end) = cached.get<lp::FragmentField>();
  Block block(&*begin, std::distance(begin, end));
  Interest decodedInterest(block);
  BOOST_CHECK_EQUAL(decodedInterest, *interest);

  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(seq), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(seq).retxCount, 0);

  BOOST_REQUIRE_EQUAL(reliability->m_netPkts.size(), 1);
  BOOST_REQUIRE_EQUAL(reliability->m_netPkts.count(seq), 1);
  BOOST_REQUIRE_EQUAL(reliability->m_netPkts.at(seq).unackedFrags.size(), 1);
  BOOST_CHECK_EQUAL(reliability->m_netPkts.at(seq).unackedFrags.count(seq), 1);

  BOOST_CHECK_EQUAL(reliability->m_ackQueue.size(), 0);
}

BOOST_AUTO_TEST_CASE(SendFragmented)
{
  // Limit MTU
  transport->setMtu(100);

  Data data("/abc/def");

  // Create a Data block containing 60 octets of content, which should fragment into 2 packets
  uint8_t content[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                       0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                       0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                       0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                       0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                       0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};

  data.setContent(content, sizeof(content));
  signData(data);

  lp::Packet pkt;
  pkt.add<lp::FragmentField>(make_pair(data.wireEncode().begin(), data.wireEncode().end()));

  LpFragmenter fragmenter;
  bool wasFragmentSuccessful;
  std::vector<lp::Packet> frags;
  std::tie(wasFragmentSuccessful, frags) = fragmenter.fragmentPacket(pkt, 100);
  BOOST_REQUIRE(wasFragmentSuccessful);
  BOOST_REQUIRE_EQUAL(frags.size(), 2);

  frags.at(0).add<lp::SequenceField>(123);
  frags.at(1).add<lp::SequenceField>(124);
  linkService->sendLpPackets(std::move(frags));

  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.size(), 2);
  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(123), 1);
  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(124), 1);
  lp::Packet cached1;
  BOOST_REQUIRE_NO_THROW(cached1.wireDecode(transport->sentPackets.front().packet));
  BOOST_REQUIRE(cached1.has<lp::SequenceField>());
  BOOST_CHECK_EQUAL(cached1.get<lp::SequenceField>(), 123);
  lp::Packet cached2;
  BOOST_REQUIRE_NO_THROW(cached2.wireDecode(transport->sentPackets.back().packet));
  BOOST_REQUIRE(cached2.has<lp::SequenceField>());
  BOOST_CHECK_EQUAL(cached2.get<lp::SequenceField>(), 124);
  lp::Sequence firstSeq = cached1.get<lp::SequenceField>();

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(firstSeq).retxCount, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(firstSeq + 1).retxCount, 0);

  BOOST_REQUIRE_EQUAL(reliability->m_netPkts.size(), 1);
  BOOST_REQUIRE_EQUAL(reliability->m_netPkts.count(firstSeq), 1);
  BOOST_REQUIRE_EQUAL(reliability->m_netPkts.at(firstSeq).unackedFrags.size(), 2);
  BOOST_CHECK_EQUAL(reliability->m_netPkts.at(firstSeq).unackedFrags.count(firstSeq), 1);
  BOOST_CHECK_EQUAL(reliability->m_netPkts.at(firstSeq).unackedFrags.count(firstSeq + 1), 1);

  BOOST_CHECK_EQUAL(reliability->m_ackQueue.size(), 0);
}

BOOST_AUTO_TEST_CASE(ProcessIncomingPacket)
{
  BOOST_CHECK(!reliability->m_isIdleAckTimerRunning);

  shared_ptr<Interest> interest = makeInterest("/abc/def");

  lp::Packet pkt1;
  pkt1.add<lp::SequenceField>(999888);
  pkt1.add<lp::FragmentField>(make_pair(interest->wireEncode().begin(), interest->wireEncode().end()));

  BOOST_CHECK_EQUAL(reliability->m_ackQueue.size(), 0);

  reliability->processIncomingPacket(pkt1);

  BOOST_CHECK(reliability->m_isIdleAckTimerRunning);
  BOOST_REQUIRE_EQUAL(reliability->m_ackQueue.size(), 1);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.front(), 999888);

  lp::Packet pkt2;
  pkt2.add<lp::SequenceField>(111222);
  pkt2.add<lp::FragmentField>(make_pair(interest->wireEncode().begin(), interest->wireEncode().end()));

  reliability->processIncomingPacket(pkt2);

  BOOST_CHECK(reliability->m_isIdleAckTimerRunning);
  BOOST_REQUIRE_EQUAL(reliability->m_ackQueue.size(), 2);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.front(), 999888);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.back(), 111222);

  // T+5ms
  advanceClocks(time::milliseconds(1), 5);
  BOOST_CHECK(!reliability->m_isIdleAckTimerRunning);
}

BOOST_AUTO_TEST_CASE(ProcessReceivedAcks)
{
  shared_ptr<Interest> interest = makeInterest("/abc/def");

  lp::Packet pkt1;
  pkt1.add<lp::SequenceField>(1024);
  pkt1.add<lp::FragmentField>(make_pair(interest->wireEncode().begin(), interest->wireEncode().end()));

  lp::Packet pkt2;
  pkt2.add<lp::SequenceField>(1025);
  pkt2.add<lp::FragmentField>(make_pair(interest->wireEncode().begin(), interest->wireEncode().end()));

  linkService->sendLpPackets({pkt1, pkt2});

  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(1024), 1);
  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(1025), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(1024).retxCount, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(1025).retxCount, 0);
  BOOST_REQUIRE_EQUAL(reliability->m_netPkts.count(1024), 1);
  BOOST_CHECK_EQUAL(reliability->m_netPkts[1024].unackedFrags.size(), 2);
  BOOST_CHECK_EQUAL(reliability->m_netPkts[1024].unackedFrags.count(1024), 1);
  BOOST_CHECK_EQUAL(reliability->m_netPkts[1024].unackedFrags.count(1025), 1);

  advanceClocks(time::milliseconds(1), 500);

  lp::Packet ackPkt1;
  ackPkt1.add<lp::AckField>(101010); // Unknown sequence number - ignored
  ackPkt1.add<lp::AckField>(1025);

  reliability->processIncomingPacket(ackPkt1);

  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(1024), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(1025), 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(1024).retxCount, 0);
  BOOST_REQUIRE_EQUAL(reliability->m_netPkts.count(1024), 1);
  BOOST_CHECK_EQUAL(reliability->m_netPkts[1024].unackedFrags.size(), 1);
  BOOST_CHECK_EQUAL(reliability->m_netPkts[1024].unackedFrags.count(1024), 1);

  lp::Packet ackPkt2;
  ackPkt2.add<lp::AckField>(1024);

  reliability->processIncomingPacket(ackPkt2);

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 0);
  BOOST_CHECK_EQUAL(reliability->m_netPkts.size(), 0);
}

BOOST_AUTO_TEST_CASE(RetxUnackedSequence)
{
  shared_ptr<Interest> interest = makeInterest("/abc/def");

  lp::Packet pkt1;
  pkt1.add<lp::SequenceField>(1024);
  pkt1.add<lp::FragmentField>(make_pair(interest->wireEncode().begin(), interest->wireEncode().end()));

  lp::Packet pkt2;
  pkt2.add<lp::SequenceField>(1025);
  pkt2.add<lp::FragmentField>(make_pair(interest->wireEncode().begin(), interest->wireEncode().end()));

  linkService->sendLpPackets({pkt1});
  // T+500ms
  // 1024 rto: 1000ms, started T+0ms, retx 0
  advanceClocks(time::milliseconds(1), 500);
  linkService->sendLpPackets({pkt2});

  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(1024), 1);
  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(1025), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(1024).retxCount, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(1025).retxCount, 0);
  BOOST_REQUIRE_EQUAL(reliability->m_netPkts.count(1024), 1);
  BOOST_REQUIRE_EQUAL(reliability->m_netPkts.count(1025), 1);
  BOOST_CHECK_EQUAL(reliability->m_netPkts[1024].unackedFrags.size(), 1);
  BOOST_CHECK_EQUAL(reliability->m_netPkts[1024].unackedFrags.count(1024), 1);
  BOOST_CHECK_EQUAL(reliability->m_netPkts[1025].unackedFrags.size(), 1);
  BOOST_CHECK_EQUAL(reliability->m_netPkts[1025].unackedFrags.count(1025), 1);

  // T+1250ms
  // 1024 rto: 1000ms, started T+1000ms, retx 1
  // 1025 rto: 1000ms, started T+500ms, retx 0
  advanceClocks(time::milliseconds(1), 750);

  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(1024), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(1024).retxCount, 1);
  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(1025), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(1025).retxCount, 0);

  // T+2250ms
  // 1024 rto: 1000ms, started T+2000ms, retx 2
  // 1025 rto: 1000ms, started T+1500ms, retx 1
  advanceClocks(time::milliseconds(1), 1000);

  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(1024), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(1024).retxCount, 2);
  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(1025), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(1025).retxCount, 1);

  // T+3250ms
  // 1024 rto: 1000ms, started T+3000ms, retx 3
  // 1025 rto: 1000ms, started T+2500ms, retx 2
  advanceClocks(time::milliseconds(1), 1000);

  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(1024), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(1024).retxCount, 3);
  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(1025), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(1025).retxCount, 2);

  // T+4250ms
  // 1024 rto: expired, removed
  // 1025 rto: 1000ms, started T+3500ms, retx 3
  advanceClocks(time::milliseconds(1), 1000);

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(1024), 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(1025), 1);

  // T+4750ms
  // 1024 rto: expired, removed
  // 1025 rto: expired, removed
  advanceClocks(time::milliseconds(1), 1000);

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 0);
  BOOST_CHECK_EQUAL(reliability->m_netPkts.size(), 0);
}

BOOST_AUTO_TEST_CASE(LostPacketsWraparound)
{
  shared_ptr<Interest> interest = makeInterest("/abc/def");

  lp::Packet pkt1;
  pkt1.add<lp::SequenceField>(0xFFFFFFFFFFFFFFFF);
  pkt1.add<lp::FragmentField>(make_pair(interest->wireEncode().begin(), interest->wireEncode().end()));

  lp::Packet pkt2;
  pkt2.add<lp::SequenceField>(4);
  pkt2.add<lp::FragmentField>(make_pair(interest->wireEncode().begin(), interest->wireEncode().end()));

  lp::Packet pkt3;
  pkt3.add<lp::SequenceField>(5);
  pkt3.add<lp::FragmentField>(make_pair(interest->wireEncode().begin(), interest->wireEncode().end()));

  lp::Packet pkt4;
  pkt4.add<lp::SequenceField>(7);
  pkt4.add<lp::FragmentField>(make_pair(interest->wireEncode().begin(), interest->wireEncode().end()));

  lp::Packet pkt5;
  pkt5.add<lp::SequenceField>(8);
  pkt5.add<lp::FragmentField>(make_pair(interest->wireEncode().begin(), interest->wireEncode().end()));

  // Passed to sendLpPackets individually since they are from separate (encoded) network packets
  linkService->sendLpPackets({pkt1});
  linkService->sendLpPackets({pkt2});
  linkService->sendLpPackets({pkt3});
  linkService->sendLpPackets({pkt4});
  linkService->sendLpPackets({pkt5});

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 5);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(0xFFFFFFFFFFFFFFFF), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(4), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(5), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(7), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(8), 1);
  BOOST_CHECK_EQUAL(reliability->m_firstUnackedFrag->first, 0xFFFFFFFFFFFFFFFF);

  lp::Packet ackPkt1;
  ackPkt1.add<lp::AckField>(4);

  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 5);

  reliability->processIncomingPacket(ackPkt1);

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 4);
  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(0xFFFFFFFFFFFFFFFF), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(0xFFFFFFFFFFFFFFFF).retxCount, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(0xFFFFFFFFFFFFFFFF).nGreaterSeqAcks, 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(4), 0);
  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(5), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(5).retxCount, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(5).nGreaterSeqAcks, 0);
  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(7), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(7).retxCount, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(7).nGreaterSeqAcks, 0);
  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(8), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(8).retxCount, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(8).nGreaterSeqAcks, 0);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 5);
  BOOST_CHECK_EQUAL(reliability->m_firstUnackedFrag->first, 0xFFFFFFFFFFFFFFFF);

  lp::Packet ackPkt2;
  ackPkt2.add<lp::AckField>(7);

  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 5);

  reliability->processIncomingPacket(ackPkt2);

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 3);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(0xFFFFFFFFFFFFFFFF), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(0xFFFFFFFFFFFFFFFF).retxCount, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(0xFFFFFFFFFFFFFFFF).nGreaterSeqAcks, 2);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(4), 0);
  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(5), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(5).retxCount, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(5).nGreaterSeqAcks, 1);
  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(7), 0);
  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(8), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(8).retxCount, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(8).nGreaterSeqAcks, 0);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 5);
  BOOST_CHECK_EQUAL(reliability->m_firstUnackedFrag->first, 0xFFFFFFFFFFFFFFFF);

  lp::Packet ackPkt3;
  ackPkt3.add<lp::AckField>(5);

  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 5);

  reliability->processIncomingPacket(ackPkt3);

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 2);
  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(0xFFFFFFFFFFFFFFFF), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(0xFFFFFFFFFFFFFFFF).retxCount, 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(0xFFFFFFFFFFFFFFFF).nGreaterSeqAcks, 3);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(4), 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(5), 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(7), 0);
  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(8), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(8).retxCount, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(8).nGreaterSeqAcks, 0);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 6);
  BOOST_CHECK_EQUAL(reliability->m_firstUnackedFrag->first, 0xFFFFFFFFFFFFFFFF);
  lp::Packet sentRetxPkt(transport->sentPackets.back().packet);
  BOOST_REQUIRE(sentRetxPkt.has<lp::SequenceField>());
  BOOST_CHECK_EQUAL(sentRetxPkt.get<lp::SequenceField>(), 0xFFFFFFFFFFFFFFFF);

  lp::Packet ackPkt4;
  ackPkt4.add<lp::AckField>(0xFFFFFFFFFFFFFFFF);

  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 6);

  reliability->processIncomingPacket(ackPkt4);

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(0xFFFFFFFFFFFFFFFF), 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(4), 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(5), 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(7), 0);
  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(8), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(8).retxCount, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(8).nGreaterSeqAcks, 0);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 6);
  BOOST_CHECK_EQUAL(reliability->m_firstUnackedFrag->first, 8);
}

BOOST_AUTO_TEST_CASE(PiggybackAcks)
{
  reliability->m_ackQueue.push(256);
  reliability->m_ackQueue.push(257);
  reliability->m_ackQueue.push(10);

  lp::Packet pkt;
  pkt.add<lp::SequenceField>(123456);
  linkService->sendLpPackets({pkt});

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet sentPkt(transport->sentPackets.front().packet);

  BOOST_REQUIRE_EQUAL(sentPkt.count<lp::AckField>(), 3);
  BOOST_CHECK_EQUAL(sentPkt.get<lp::AckField>(0), 256);
  BOOST_CHECK_EQUAL(sentPkt.get<lp::AckField>(1), 257);
  BOOST_CHECK_EQUAL(sentPkt.get<lp::AckField>(2), 10);

  BOOST_CHECK_EQUAL(reliability->m_ackQueue.size(), 0);
}

BOOST_AUTO_TEST_CASE(PiggybackAcksMtu)
{
  // This test case tests for piggybacking Acks when there is an MTU on the link.

  reliability->m_ackQueue.push(1010);
  reliability->m_ackQueue.push(1011);
  reliability->m_ackQueue.push(1013);
  reliability->m_ackQueue.push(1014);

  Data data("/abc/def");

  // Create a Data block containing 60 octets of content, which should fragment into 2 packets
  uint8_t content[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                       0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                       0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                       0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                       0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                       0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};

  data.setContent(content, sizeof(content));
  signData(data);

  lp::Packet pkt1;
  pkt1.add<lp::SequenceField>(123);
  pkt1.add<lp::FragmentField>(make_pair(data.wireEncode().begin(), data.wireEncode().end()));

  // Allow 2 Acks per packet, plus a little bit of extra space
  // sizeof(lp::Sequence) + Ack Type (3 octets) + Ack Length (1 octet)
  transport->setMtu(pkt1.wireEncode().size() + 2 * (sizeof(lp::Sequence) + 3 + 1) + 3);

  linkService->sendLpPackets({pkt1});

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet sentPkt1(transport->sentPackets.front().packet);

  BOOST_REQUIRE_EQUAL(sentPkt1.count<lp::AckField>(), 2);
  BOOST_CHECK_EQUAL(sentPkt1.get<lp::AckField>(0), 1010);
  BOOST_CHECK_EQUAL(sentPkt1.get<lp::AckField>(1), 1011);

  BOOST_REQUIRE_EQUAL(reliability->m_ackQueue.size(), 2);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.front(), 1013);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.back(), 1014);

  lp::Packet pkt2;
  pkt2.add<lp::SequenceField>(105623);
  pkt2.add<lp::FragmentField>(make_pair(data.wireEncode().begin(), data.wireEncode().end()));

  // Allow 1 Acks per packet, plus a little bit of extra space (1 Ack - 1 octet)
  // sizeof(lp::Sequence) + Ack Type (3 octets) + Ack Length (1 octet)
  transport->setMtu(pkt2.wireEncode().size() + 2 * (sizeof(lp::Sequence) + 3 + 1) - 1);

  linkService->sendLpPackets({pkt2});

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 2);
  lp::Packet sentPkt2(transport->sentPackets.back().packet);

  BOOST_REQUIRE_EQUAL(sentPkt2.count<lp::AckField>(), 1);
  BOOST_CHECK_EQUAL(sentPkt2.get<lp::AckField>(), 1013);

  BOOST_REQUIRE_EQUAL(reliability->m_ackQueue.size(), 1);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.front(), 1014);

  lp::Packet pkt3;
  pkt3.add<lp::SequenceField>(969456);
  pkt3.add<lp::FragmentField>(make_pair(data.wireEncode().begin(), data.wireEncode().end()));

  // Allow 3 Acks per packet
  // sizeof(lp::Sequence) + Ack Type (3 octets) + Ack Length (1 octet)
  transport->setMtu(pkt3.wireEncode().size() + 3 * (sizeof(lp::Sequence) + 3 + 1));

  linkService->sendLpPackets({pkt3});

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 3);
  lp::Packet sentPkt3(transport->sentPackets.back().packet);

  BOOST_REQUIRE_EQUAL(sentPkt3.count<lp::AckField>(), 1);
  BOOST_CHECK_EQUAL(sentPkt3.get<lp::AckField>(), 1014);

  BOOST_CHECK_EQUAL(reliability->m_ackQueue.size(), 0);
}

BOOST_AUTO_TEST_CASE(IdleAckTimer)
{
  // T+1ms
  advanceClocks(time::milliseconds(1), 1);

  reliability->m_ackQueue.push(5000);
  reliability->m_ackQueue.push(5001);
  reliability->m_ackQueue.push(5002);
  BOOST_CHECK(!reliability->m_isIdleAckTimerRunning);
  reliability->startIdleAckTimer();
  BOOST_CHECK(reliability->m_isIdleAckTimerRunning);

  // T+5ms
  advanceClocks(time::milliseconds(1), 4);
  BOOST_CHECK(reliability->m_isIdleAckTimerRunning);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 0);

  // T+6ms
  advanceClocks(time::milliseconds(1), 1);

  BOOST_CHECK(!reliability->m_isIdleAckTimerRunning);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet sentPkt1(transport->sentPackets.back().packet);

  BOOST_REQUIRE_EQUAL(sentPkt1.count<lp::AckField>(), 3);
  BOOST_CHECK_EQUAL(sentPkt1.get<lp::AckField>(0), 5000);
  BOOST_CHECK_EQUAL(sentPkt1.get<lp::AckField>(1), 5001);
  BOOST_CHECK_EQUAL(sentPkt1.get<lp::AckField>(2), 5002);

  reliability->m_ackQueue.push(5003);
  reliability->m_ackQueue.push(5004);
  reliability->startIdleAckTimer();
  BOOST_CHECK(reliability->m_isIdleAckTimerRunning);

  // T+10ms
  advanceClocks(time::milliseconds(1), 4);
  BOOST_CHECK(reliability->m_isIdleAckTimerRunning);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 1);

  // T+11ms
  advanceClocks(time::milliseconds(1), 1);

  BOOST_CHECK(!reliability->m_isIdleAckTimerRunning);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 2);
  lp::Packet sentPkt2(transport->sentPackets.back().packet);

  BOOST_REQUIRE_EQUAL(sentPkt2.count<lp::AckField>(), 2);
  BOOST_CHECK_EQUAL(sentPkt2.get<lp::AckField>(0), 5003);
  BOOST_CHECK_EQUAL(sentPkt2.get<lp::AckField>(1), 5004);

  BOOST_CHECK_EQUAL(reliability->m_ackQueue.size(), 0);

  // T+16ms
  advanceClocks(time::milliseconds(1), 5);

  BOOST_CHECK(!reliability->m_isIdleAckTimerRunning);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 2);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.size(), 0);
}

BOOST_AUTO_TEST_CASE(IdleAckTimerMtu)
{
  // 1 (LpPacket Type) + 1 (LpPacket Length) + 2 Acks
  transport->setMtu(lp::Packet().wireEncode().size() + 2 * (sizeof(lp::Sequence) + 3 + 1));

  // T+1ms
  advanceClocks(time::milliseconds(1), 1);

  reliability->m_ackQueue.push(3000);
  reliability->m_ackQueue.push(3001);
  reliability->m_ackQueue.push(3002);
  reliability->m_ackQueue.push(3003);
  reliability->m_ackQueue.push(3004);
  BOOST_CHECK(!reliability->m_isIdleAckTimerRunning);
  reliability->startIdleAckTimer();
  BOOST_CHECK(reliability->m_isIdleAckTimerRunning);

  // T+5ms
  advanceClocks(time::milliseconds(1), 4);
  BOOST_CHECK(reliability->m_isIdleAckTimerRunning);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 0);

  // T+6ms
  advanceClocks(time::milliseconds(1), 1);

  BOOST_CHECK(!reliability->m_isIdleAckTimerRunning);

  reliability->m_ackQueue.push(3005);
  reliability->m_ackQueue.push(3006);
  reliability->startIdleAckTimer();
  BOOST_CHECK(reliability->m_isIdleAckTimerRunning);

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 3);
  lp::Packet sentPkt1(transport->sentPackets[0].packet);
  BOOST_REQUIRE_EQUAL(sentPkt1.count<lp::AckField>(), 2);
  BOOST_CHECK_EQUAL(sentPkt1.get<lp::AckField>(0), 3000);
  BOOST_CHECK_EQUAL(sentPkt1.get<lp::AckField>(1), 3001);
  lp::Packet sentPkt2(transport->sentPackets[1].packet);
  BOOST_REQUIRE_EQUAL(sentPkt2.count<lp::AckField>(), 2);
  BOOST_CHECK_EQUAL(sentPkt2.get<lp::AckField>(0), 3002);
  BOOST_CHECK_EQUAL(sentPkt2.get<lp::AckField>(1), 3003);
  lp::Packet sentPkt3(transport->sentPackets[2].packet);
  BOOST_REQUIRE_EQUAL(sentPkt3.count<lp::AckField>(), 1);
  BOOST_CHECK_EQUAL(sentPkt3.get<lp::AckField>(), 3004);

  BOOST_REQUIRE_EQUAL(reliability->m_ackQueue.size(), 2);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.front(), 3005);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.back(), 3006);

  // T+10ms
  advanceClocks(time::milliseconds(1), 4);
  BOOST_CHECK(reliability->m_isIdleAckTimerRunning);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 3);

  // T+11ms
  advanceClocks(time::milliseconds(1), 1);

  BOOST_CHECK(!reliability->m_isIdleAckTimerRunning);

  reliability->m_ackQueue.push(3007);
  reliability->startIdleAckTimer();

  BOOST_CHECK(reliability->m_isIdleAckTimerRunning);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 4);
  lp::Packet sentPkt4(transport->sentPackets[3].packet);
  BOOST_REQUIRE_EQUAL(sentPkt4.count<lp::AckField>(), 2);
  BOOST_CHECK_EQUAL(sentPkt4.get<lp::AckField>(0), 3005);
  BOOST_CHECK_EQUAL(sentPkt4.get<lp::AckField>(1), 3006);

  BOOST_REQUIRE_EQUAL(reliability->m_ackQueue.size(), 1);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.front(), 3007);
  BOOST_CHECK(reliability->m_isIdleAckTimerRunning);

  // T+15ms
  advanceClocks(time::milliseconds(1), 4);
  BOOST_CHECK(reliability->m_isIdleAckTimerRunning);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 4);

  // T+16ms
  advanceClocks(time::milliseconds(1), 1);

  BOOST_CHECK(!reliability->m_isIdleAckTimerRunning);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 5);
  lp::Packet sentPkt5(transport->sentPackets[4].packet);
  BOOST_REQUIRE_EQUAL(sentPkt5.count<lp::AckField>(), 1);
  BOOST_CHECK_EQUAL(sentPkt5.get<lp::AckField>(), 3007);

  BOOST_CHECK_EQUAL(reliability->m_ackQueue.size(), 0);

  // T+21ms
  advanceClocks(time::milliseconds(1), 5);

  BOOST_CHECK(!reliability->m_isIdleAckTimerRunning);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 5);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // TestLpReliability

BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
