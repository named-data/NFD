/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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
#include "face/generic-link-service.hpp"

#include "tests/test-common.hpp"
#include "dummy-face.hpp"
#include "dummy-transport.hpp"

#include <cstring>
#include <unordered_set>

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
      Interest interest("/test/prefix");
      lp::Packet pkt;
      pkt.add<lp::FragmentField>(make_pair(interest.wireEncode().begin(), interest.wireEncode().end()));
      m_reliability.handleOutgoing(frags, std::move(pkt), true);
    }

    for (lp::Packet frag : frags) {
      this->sendLpPacket(std::move(frag));
    }
  }

private:
  void
  doSendInterest(const Interest&) final
  {
    BOOST_ASSERT(false);
  }

  void
  doSendData(const Data&) final
  {
    BOOST_ASSERT(false);
  }

  void
  doSendNack(const lp::Nack&) final
  {
    BOOST_ASSERT(false);
  }

  void
  doReceivePacket(Transport::Packet&&) final
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
    reliability->m_lastTxSeqNo = 1;
  }

  static bool
  netPktHasUnackedFrag(const shared_ptr<LpReliability::NetPkt>& netPkt, lp::Sequence txSeq)
  {
    return std::any_of(netPkt->unackedFrags.begin(), netPkt->unackedFrags.end(),
                       [txSeq] (const LpReliability::UnackedFrags::iterator& frag) {
                         return frag->first == txSeq;
                       });
  }

  /** \brief make an LpPacket with fragment of specified size
   *  \param pktNo packet identifier, which can be extracted with \p getPktNo
   *  \param payloadSize total payload size; if this is less than 4, 4 will be used
   */
  static lp::Packet
  makeFrag(uint32_t pktNo, size_t payloadSize = 4)
  {
    payloadSize = std::max(payloadSize, static_cast<size_t>(4));
    BOOST_ASSERT(payloadSize <= 255);

    lp::Packet pkt;
    ndn::Buffer buf(payloadSize);
    std::memcpy(buf.data(), &pktNo, sizeof(pktNo));
    pkt.set<lp::FragmentField>(make_pair(buf.cbegin(), buf.cend()));
    return pkt;
  }

  /** \brief extract packet identifier from LpPacket made with \p makeFrag
   *  \retval 0 packet identifier cannot be extracted
   */
  static uint32_t
  getPktNo(const lp::Packet& pkt)
  {
    BOOST_ASSERT(pkt.has<lp::FragmentField>());

    ndn::Buffer::const_iterator begin, end;
    std::tie(begin, end) = pkt.get<lp::FragmentField>();
    if (std::distance(begin, end) < 4) {
      return 0;
    }

    uint32_t value = 0;
    std::memcpy(&value, &*begin, sizeof(value));
    return value;
  }

protected:
  unique_ptr<DummyLpReliabilityLinkService> linkService;
  unique_ptr<DummyTransport> transport;
  unique_ptr<DummyFace> face;
  LpReliability* reliability;
};

BOOST_FIXTURE_TEST_SUITE(TestLpReliability, LpReliabilityFixture)

BOOST_AUTO_TEST_CASE(SendNoFragmentField)
{
  lp::Packet pkt;

  linkService->sendLpPackets({pkt});
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 0);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.size(), 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nAcknowledged, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetransmitted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetxExhausted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nDroppedInterests, 0);
}

BOOST_AUTO_TEST_CASE(SendUnfragmentedRetx)
{
  lp::Packet pkt1 = makeFrag(1024, 50);
  lp::Packet pkt2 = makeFrag(3000, 30);

  linkService->sendLpPackets({pkt1});
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet cached1(transport->sentPackets.front().packet);
  BOOST_REQUIRE(cached1.has<lp::TxSequenceField>());
  BOOST_CHECK(!cached1.has<lp::SequenceField>());
  lp::Sequence firstTxSeq = cached1.get<lp::TxSequenceField>();
  BOOST_CHECK_EQUAL(firstTxSeq, 2);
  BOOST_CHECK_EQUAL(getPktNo(cached1), 1024);
  BOOST_CHECK_EQUAL(linkService->getCounters().nAcknowledged, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetransmitted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetxExhausted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nDroppedInterests, 0);

  // T+500ms
  // 1024 rto: 1000ms, txSeq: 2, started T+0ms, retx 0
  advanceClocks(time::milliseconds(1), 500);
  linkService->sendLpPackets({pkt2});
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 2);

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 2);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(firstTxSeq), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(firstTxSeq + 1), 1);
  BOOST_CHECK(reliability->m_unackedFrags.at(firstTxSeq).netPkt);
  BOOST_CHECK(reliability->m_unackedFrags.at(firstTxSeq + 1).netPkt);
  BOOST_CHECK_NE(reliability->m_unackedFrags.at(firstTxSeq).netPkt,
                 reliability->m_unackedFrags.at(firstTxSeq + 1).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(firstTxSeq).retxCount, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(firstTxSeq + 1).retxCount, 0);
  BOOST_CHECK_EQUAL(reliability->m_firstUnackedFrag->first, firstTxSeq);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.size(), 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nAcknowledged, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetransmitted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetxExhausted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nDroppedInterests, 0);

  // T+1250ms
  // 1024 rto: 1000ms, txSeq: 4, started T+1000ms, retx 1
  // 3000 rto: 1000ms, txSeq: 3, started T+500ms, retx 0
  advanceClocks(time::milliseconds(1), 750);

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 2);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(firstTxSeq), 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(firstTxSeq + 2), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(firstTxSeq + 2).retxCount, 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(firstTxSeq + 1), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(firstTxSeq + 1).retxCount, 0);
  BOOST_CHECK_EQUAL(reliability->m_firstUnackedFrag->first, firstTxSeq + 1);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 3);
  BOOST_CHECK_EQUAL(linkService->getCounters().nAcknowledged, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetransmitted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetxExhausted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nDroppedInterests, 0);

  // T+2250ms
  // 1024 rto: 1000ms, txSeq: 6, started T+2000ms, retx 2
  // 3000 rto: 1000ms, txSeq: 5, started T+1500ms, retx 1
  advanceClocks(time::milliseconds(1), 1000);

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 2);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(firstTxSeq + 1), 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(firstTxSeq + 2), 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(firstTxSeq + 4), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(firstTxSeq + 4).retxCount, 2);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(firstTxSeq + 3), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(firstTxSeq + 3).retxCount, 1);
  BOOST_CHECK_EQUAL(reliability->m_firstUnackedFrag->first, firstTxSeq + 3);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 5);
  BOOST_CHECK_EQUAL(linkService->getCounters().nAcknowledged, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetransmitted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetxExhausted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nDroppedInterests, 0);

  // T+3250ms
  // 1024 rto: 1000ms, txSeq: 8, started T+3000ms, retx 3
  // 3000 rto: 1000ms, txSeq: 7, started T+2500ms, retx 2
  advanceClocks(time::milliseconds(1), 1000);

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 2);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(firstTxSeq + 3), 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(firstTxSeq + 4), 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(firstTxSeq + 6), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(firstTxSeq + 6).retxCount, 3);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(firstTxSeq + 5), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(firstTxSeq + 5).retxCount, 2);
  BOOST_CHECK_EQUAL(reliability->m_firstUnackedFrag->first, firstTxSeq + 5);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 7);
  BOOST_CHECK_EQUAL(linkService->getCounters().nAcknowledged, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetransmitted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetxExhausted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nDroppedInterests, 0);

  // T+4250ms
  // 1024 rto: expired, removed
  // 3000 rto: 1000ms, txSeq: 9, started T+3500ms, retx 3
  advanceClocks(time::milliseconds(1), 1000);

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(firstTxSeq + 5), 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(firstTxSeq + 6), 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(firstTxSeq + 7), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(firstTxSeq + 7).retxCount, 3);
  BOOST_CHECK_EQUAL(reliability->m_firstUnackedFrag->first, firstTxSeq + 7);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 8);

  BOOST_CHECK_EQUAL(linkService->getCounters().nAcknowledged, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetransmitted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetxExhausted, 1);
  BOOST_CHECK_EQUAL(linkService->getCounters().nDroppedInterests, 1);

  // T+4750ms
  // 1024 rto: expired, removed
  // 3000 rto: expired, removed
  advanceClocks(time::milliseconds(1), 1000);

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 0);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.size(), 0);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 8);
  BOOST_CHECK_EQUAL(linkService->getCounters().nAcknowledged, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetransmitted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetxExhausted, 2);
  BOOST_CHECK_EQUAL(linkService->getCounters().nDroppedInterests, 2);
}

BOOST_AUTO_TEST_CASE(SendFragmentedRetx)
{
  lp::Packet pkt1 = makeFrag(2048, 30);
  lp::Packet pkt2 = makeFrag(2049, 30);
  lp::Packet pkt3 = makeFrag(2050, 10);

  linkService->sendLpPackets({pkt1, pkt2, pkt3});
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 3);

  lp::Packet cached1(transport->sentPackets.at(0).packet);
  BOOST_REQUIRE(cached1.has<lp::TxSequenceField>());
  BOOST_CHECK_EQUAL(cached1.get<lp::TxSequenceField>(), 2);
  BOOST_CHECK_EQUAL(getPktNo(cached1), 2048);
  lp::Packet cached2(transport->sentPackets.at(1).packet);
  BOOST_REQUIRE(cached2.has<lp::TxSequenceField>());
  BOOST_CHECK_EQUAL(cached2.get<lp::TxSequenceField>(), 3);
  BOOST_CHECK_EQUAL(getPktNo(cached2), 2049);
  lp::Packet cached3(transport->sentPackets.at(2).packet);
  BOOST_REQUIRE(cached3.has<lp::TxSequenceField>());
  BOOST_CHECK_EQUAL(cached3.get<lp::TxSequenceField>(), 4);
  BOOST_CHECK_EQUAL(getPktNo(cached3), 2050);
  BOOST_CHECK_EQUAL(linkService->getCounters().nAcknowledged, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetransmitted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetxExhausted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nDroppedInterests, 0);

  // T+0ms
  // 2048 rto: 1000ms, txSeq: 2, started T+0ms, retx 0
  // 2049 rto: 1000ms, txSeq: 3, started T+0ms, retx 0
  // 2050 rto: 1000ms, txSeq: 4, started T+0ms, retx 0

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(2), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(3), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(4), 1);
  BOOST_CHECK_EQUAL(getPktNo(reliability->m_unackedFrags.at(2).pkt), 2048);
  BOOST_CHECK_EQUAL(getPktNo(reliability->m_unackedFrags.at(3).pkt), 2049);
  BOOST_CHECK_EQUAL(getPktNo(reliability->m_unackedFrags.at(4).pkt), 2050);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(2).retxCount, 0);
  BOOST_REQUIRE(reliability->m_unackedFrags.at(2).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(3).retxCount, 0);
  BOOST_REQUIRE(reliability->m_unackedFrags.at(3).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(4).retxCount, 0);
  BOOST_REQUIRE(reliability->m_unackedFrags.at(4).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(2).netPkt, reliability->m_unackedFrags.at(3).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(2).netPkt, reliability->m_unackedFrags.at(4).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(2).netPkt->unackedFrags.size(), 3);
  BOOST_CHECK(netPktHasUnackedFrag(reliability->m_unackedFrags.at(2).netPkt, 2));
  BOOST_CHECK(netPktHasUnackedFrag(reliability->m_unackedFrags.at(2).netPkt, 3));
  BOOST_CHECK(netPktHasUnackedFrag(reliability->m_unackedFrags.at(2).netPkt, 4));
  BOOST_CHECK_EQUAL(reliability->m_firstUnackedFrag->first, 2);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.size(), 0);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 3);
  BOOST_CHECK_EQUAL(linkService->getCounters().nAcknowledged, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetransmitted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetxExhausted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nDroppedInterests, 0);

  // T+250ms
  // 2048 rto: 1000ms, txSeq: 2, started T+0ms, retx 0
  // 2049 rto: 1000ms, txSeq: 5, started T+250ms, retx 1
  // 2050 rto: 1000ms, txSeq: 4, started T+0ms, retx 0
  advanceClocks(time::milliseconds(1), 250);
  reliability->onLpPacketLost(3);

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(2), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(3), 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(5), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(4), 1);
  BOOST_CHECK_EQUAL(getPktNo(reliability->m_unackedFrags.at(2).pkt), 2048);
  BOOST_CHECK_EQUAL(getPktNo(reliability->m_unackedFrags.at(5).pkt), 2049);
  BOOST_CHECK_EQUAL(getPktNo(reliability->m_unackedFrags.at(4).pkt), 2050);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(2).retxCount, 0);
  BOOST_REQUIRE(reliability->m_unackedFrags.at(2).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(5).retxCount, 1);
  BOOST_REQUIRE(reliability->m_unackedFrags.at(5).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(4).retxCount, 0);
  BOOST_REQUIRE(reliability->m_unackedFrags.at(4).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(2).netPkt, reliability->m_unackedFrags.at(5).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(2).netPkt, reliability->m_unackedFrags.at(4).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(2).netPkt->unackedFrags.size(), 3);
  BOOST_CHECK(netPktHasUnackedFrag(reliability->m_unackedFrags.at(2).netPkt, 2));
  BOOST_CHECK(!netPktHasUnackedFrag(reliability->m_unackedFrags.at(2).netPkt, 3));
  BOOST_CHECK(netPktHasUnackedFrag(reliability->m_unackedFrags.at(2).netPkt, 5));
  BOOST_CHECK(netPktHasUnackedFrag(reliability->m_unackedFrags.at(2).netPkt, 4));
  BOOST_CHECK_EQUAL(reliability->m_firstUnackedFrag->first, 2);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 4);
  BOOST_CHECK_EQUAL(linkService->getCounters().nAcknowledged, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetransmitted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetxExhausted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nDroppedInterests, 0);

  // T+500ms
  // 2048 rto: 1000ms, txSeq: 2, started T+0ms, retx 0
  // 2049 rto: 1000ms, txSeq: 6, started T+500ms, retx 2
  // 2050 rto: 1000ms, txSeq: 4, started T+0ms, retx 0
  advanceClocks(time::milliseconds(1), 250);
  reliability->onLpPacketLost(5);

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(2), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(5), 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(6), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(4), 1);
  BOOST_CHECK_EQUAL(getPktNo(reliability->m_unackedFrags.at(2).pkt), 2048);
  BOOST_CHECK_EQUAL(getPktNo(reliability->m_unackedFrags.at(6).pkt), 2049);
  BOOST_CHECK_EQUAL(getPktNo(reliability->m_unackedFrags.at(4).pkt), 2050);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(2).retxCount, 0);
  BOOST_REQUIRE(reliability->m_unackedFrags.at(2).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(6).retxCount, 2);
  BOOST_REQUIRE(reliability->m_unackedFrags.at(6).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(4).retxCount, 0);
  BOOST_REQUIRE(reliability->m_unackedFrags.at(4).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(2).netPkt, reliability->m_unackedFrags.at(6).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(2).netPkt, reliability->m_unackedFrags.at(4).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(2).netPkt->unackedFrags.size(), 3);
  BOOST_CHECK(netPktHasUnackedFrag(reliability->m_unackedFrags.at(2).netPkt, 2));
  BOOST_CHECK(!netPktHasUnackedFrag(reliability->m_unackedFrags.at(2).netPkt, 5));
  BOOST_CHECK(netPktHasUnackedFrag(reliability->m_unackedFrags.at(2).netPkt, 6));
  BOOST_CHECK(netPktHasUnackedFrag(reliability->m_unackedFrags.at(2).netPkt, 4));
  BOOST_CHECK_EQUAL(reliability->m_firstUnackedFrag->first, 2);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 5);
  BOOST_CHECK_EQUAL(linkService->getCounters().nAcknowledged, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetransmitted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetxExhausted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nDroppedInterests, 0);

  // T+750ms
  // 2048 rto: 1000ms, txSeq: 2, started T+0ms, retx 0
  // 2049 rto: 1000ms, txSeq: 7, started T+750ms, retx 3
  // 2050 rto: 1000ms, txSeq: 4, started T+0ms, retx 0
  advanceClocks(time::milliseconds(1), 250);
  reliability->onLpPacketLost(6);

  BOOST_REQUIRE_EQUAL(reliability->m_unackedFrags.count(2), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(6), 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(7), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(4), 1);
  BOOST_CHECK_EQUAL(getPktNo(reliability->m_unackedFrags.at(2).pkt), 2048);
  BOOST_CHECK_EQUAL(getPktNo(reliability->m_unackedFrags.at(7).pkt), 2049);
  BOOST_CHECK_EQUAL(getPktNo(reliability->m_unackedFrags.at(4).pkt), 2050);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(2).retxCount, 0);
  BOOST_REQUIRE(reliability->m_unackedFrags.at(2).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(7).retxCount, 3);
  BOOST_REQUIRE(reliability->m_unackedFrags.at(7).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(4).retxCount, 0);
  BOOST_REQUIRE(reliability->m_unackedFrags.at(4).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(2).netPkt, reliability->m_unackedFrags.at(7).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(2).netPkt, reliability->m_unackedFrags.at(4).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(2).netPkt->unackedFrags.size(), 3);
  BOOST_CHECK(netPktHasUnackedFrag(reliability->m_unackedFrags.at(2).netPkt, 2));
  BOOST_CHECK(!netPktHasUnackedFrag(reliability->m_unackedFrags.at(2).netPkt, 6));
  BOOST_CHECK(netPktHasUnackedFrag(reliability->m_unackedFrags.at(2).netPkt, 7));
  BOOST_CHECK(netPktHasUnackedFrag(reliability->m_unackedFrags.at(2).netPkt, 4));
  BOOST_CHECK_EQUAL(reliability->m_firstUnackedFrag->first, 2);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 6);
  BOOST_CHECK_EQUAL(linkService->getCounters().nAcknowledged, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetransmitted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetxExhausted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nDroppedInterests, 0);

  // T+850ms
  // 2048 rto: expired, removed
  // 2049 rto: expired, removed
  // 2050 rto: expired, removed
  advanceClocks(time::milliseconds(1), 100);
  reliability->onLpPacketLost(7);

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 0);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.size(), 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nAcknowledged, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetransmitted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetxExhausted, 1);
  BOOST_CHECK_EQUAL(linkService->getCounters().nDroppedInterests, 1);
}

BOOST_AUTO_TEST_CASE(AckUnknownTxSeq)
{
  linkService->sendLpPackets({makeFrag(1, 50)});

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(2), 1);
  BOOST_CHECK(reliability->m_unackedFrags.at(2).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_firstUnackedFrag->first, 2);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 1);
  BOOST_CHECK_EQUAL(linkService->getCounters().nAcknowledged, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetransmitted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetxExhausted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nDroppedInterests, 0);

  lp::Packet ackPkt;
  ackPkt.add<lp::AckField>(10101010);
  reliability->processIncomingPacket(ackPkt);

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(2), 1);
  BOOST_CHECK(reliability->m_unackedFrags.at(2).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_firstUnackedFrag->first, 2);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 1);
  BOOST_CHECK_EQUAL(linkService->getCounters().nAcknowledged, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetransmitted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetxExhausted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nDroppedInterests, 0);
}

BOOST_AUTO_TEST_CASE(LossByGreaterAcks)
{
  // Detect loss by 3x greater Acks, also tests wraparound

  reliability->m_lastTxSeqNo = 0xFFFFFFFFFFFFFFFE;

  // Passed to sendLpPackets individually since they are
  // from separate, non-fragmented network packets
  linkService->sendLpPackets({makeFrag(1, 50)});
  linkService->sendLpPackets({makeFrag(2, 50)});
  linkService->sendLpPackets({makeFrag(3, 50)});
  linkService->sendLpPackets({makeFrag(4, 50)});
  linkService->sendLpPackets({makeFrag(5, 50)});

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 5);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(0xFFFFFFFFFFFFFFFF), 1); // pkt1
  BOOST_CHECK(reliability->m_unackedFrags.at(0xFFFFFFFFFFFFFFFF).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(0), 1); // pkt2
  BOOST_CHECK(reliability->m_unackedFrags.at(0).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(1), 1); // pkt3
  BOOST_CHECK(reliability->m_unackedFrags.at(1).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(2), 1); // pkt4
  BOOST_CHECK(reliability->m_unackedFrags.at(2).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(3), 1); // pkt5
  BOOST_CHECK(reliability->m_unackedFrags.at(3).netPkt);
  BOOST_CHECK_EQUAL(reliability->m_firstUnackedFrag->first, 0xFFFFFFFFFFFFFFFF);
  BOOST_CHECK_EQUAL(linkService->getCounters().nAcknowledged, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetransmitted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetxExhausted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nDroppedInterests, 0);

  lp::Packet ackPkt1;
  ackPkt1.add<lp::AckField>(0);

  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 5);

  reliability->processIncomingPacket(ackPkt1);

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 4);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(0xFFFFFFFFFFFFFFFF), 1); // pkt1
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(0xFFFFFFFFFFFFFFFF).retxCount, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(0xFFFFFFFFFFFFFFFF).nGreaterSeqAcks, 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(0), 0); // pkt2
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(1), 1); // pkt3
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(1).retxCount, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(1).nGreaterSeqAcks, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(2), 1); // pkt4
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(2).retxCount, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(2).nGreaterSeqAcks, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(3), 1); // pkt5
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(3).retxCount, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(3).nGreaterSeqAcks, 0);
  BOOST_CHECK_EQUAL(reliability->m_firstUnackedFrag->first, 0xFFFFFFFFFFFFFFFF);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 5);
  BOOST_CHECK_EQUAL(linkService->getCounters().nAcknowledged, 1);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetransmitted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetxExhausted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nDroppedInterests, 0);

  lp::Packet ackPkt2;
  ackPkt2.add<lp::AckField>(2);
  ackPkt1.add<lp::AckField>(101010); // Unknown TxSequence number - ignored

  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 5);

  reliability->processIncomingPacket(ackPkt2);

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 3);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(0xFFFFFFFFFFFFFFFF), 1); // pkt1
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(0xFFFFFFFFFFFFFFFF).retxCount, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(0xFFFFFFFFFFFFFFFF).nGreaterSeqAcks, 2);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(0), 0); // pkt2
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(1), 1); // pkt3
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(1).retxCount, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(1).nGreaterSeqAcks, 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(2), 0); // pkt4
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(3), 1); // pkt5
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(3).retxCount, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(3).nGreaterSeqAcks, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(101010), 0);
  BOOST_CHECK_EQUAL(reliability->m_firstUnackedFrag->first, 0xFFFFFFFFFFFFFFFF);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 5);
  BOOST_CHECK_EQUAL(linkService->getCounters().nAcknowledged, 2);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetransmitted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetxExhausted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nDroppedInterests, 0);

  lp::Packet ackPkt3;
  ackPkt3.add<lp::AckField>(1);

  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 5);

  reliability->processIncomingPacket(ackPkt3);

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 2);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(0xFFFFFFFFFFFFFFFF), 0); // pkt1 old TxSeq
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(0), 0); // pkt2
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(1), 0); // pkt3
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(2), 0); // pkt4
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(3), 1); // pkt5
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(3).retxCount, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(3).nGreaterSeqAcks, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(4), 1); // pkt1 new TxSeq
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(4).retxCount, 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(4).nGreaterSeqAcks, 0);
  BOOST_CHECK_EQUAL(reliability->m_firstUnackedFrag->first, 3);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 6);
  lp::Packet sentRetxPkt(transport->sentPackets.back().packet);
  BOOST_REQUIRE(sentRetxPkt.has<lp::TxSequenceField>());
  BOOST_CHECK_EQUAL(sentRetxPkt.get<lp::TxSequenceField>(), 4);
  BOOST_REQUIRE(sentRetxPkt.has<lp::FragmentField>());
  BOOST_CHECK_EQUAL(getPktNo(sentRetxPkt), 1);
  BOOST_CHECK_EQUAL(linkService->getCounters().nAcknowledged, 3);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetransmitted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetxExhausted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nDroppedInterests, 0);

  lp::Packet ackPkt4;
  ackPkt4.add<lp::AckField>(4);

  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 6);

  reliability->processIncomingPacket(ackPkt4);

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(0xFFFFFFFFFFFFFFFF), 0); // pkt1 old TxSeq
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(0), 0); // pkt2
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(1), 0); // pkt3
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(2), 0); // pkt4
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(3), 1); // pkt5
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(3).retxCount, 0);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(3).nGreaterSeqAcks, 1);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.count(4), 0); // pkt1 new TxSeq
  BOOST_CHECK_EQUAL(reliability->m_firstUnackedFrag->first, 3);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 6);
  BOOST_CHECK_EQUAL(linkService->getCounters().nAcknowledged, 3);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetransmitted, 1);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetxExhausted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nDroppedInterests, 0);
}

BOOST_AUTO_TEST_CASE(SkipFragmentsRemovedInRtt)
{
  auto opts = linkService->getOptions();
  opts.reliabilityOptions.maxRetx = 0; // just to make the test case shorter
  opts.reliabilityOptions.seqNumLossThreshold = 3;
  linkService->setOptions(opts);

  lp::Packet frag1 = makeFrag(5001);
  lp::Packet frag2 = makeFrag(5002);
  linkService->sendLpPackets({frag1, frag2}); // First packet has 2 fragments
  linkService->sendLpPackets({makeFrag(5003)});
  linkService->sendLpPackets({makeFrag(5004)});
  linkService->sendLpPackets({makeFrag(5005)});

  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 5);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 5);

  lp::Sequence firstTxSeq = reliability->m_firstUnackedFrag->first;

  // Ack the last 2 packets
  lp::Packet ackPkt1;
  ackPkt1.add<lp::AckField>(firstTxSeq + 4);
  ackPkt1.add<lp::AckField>(firstTxSeq + 3);
  reliability->processIncomingPacket(ackPkt1);

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 3);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(firstTxSeq).nGreaterSeqAcks, 2);
  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.at(firstTxSeq + 1).nGreaterSeqAcks, 2);

  // Ack the third packet (5003)
  // This triggers a "loss by greater Acks" for packets 5001 and 5002
  lp::Packet ackPkt2;
  ackPkt2.add<lp::AckField>(firstTxSeq + 2);
  reliability->processIncomingPacket(ackPkt2); // tests crash/assert reported in bug #4479

  BOOST_CHECK_EQUAL(reliability->m_unackedFrags.size(), 0);
}

BOOST_AUTO_TEST_CASE(CancelLossNotificationOnAck)
{
  reliability->onDroppedInterest.connect([] (const Interest&) {
    BOOST_FAIL("Packet loss timeout should be cancelled when packet acknowledged");
  });

  reliability->m_lastTxSeqNo = 0;

  linkService->sendLpPackets({makeFrag(1, 50)});

  advanceClocks(time::milliseconds(1), 500);

  lp::Packet ackPkt;
  ackPkt.add<lp::AckField>(1);
  reliability->processIncomingPacket(ackPkt);

  advanceClocks(time::milliseconds(1), 1000);

  BOOST_CHECK_EQUAL(linkService->getCounters().nAcknowledged, 1);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetransmitted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nRetxExhausted, 0);
  BOOST_CHECK_EQUAL(linkService->getCounters().nDroppedInterests, 0);
}

BOOST_AUTO_TEST_CASE(ProcessIncomingPacket)
{
  BOOST_CHECK(!reliability->m_isIdleAckTimerRunning);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.size(), 0);

  lp::Packet pkt1 = makeFrag(100, 40);
  pkt1.add<lp::TxSequenceField>(765432);

  reliability->processIncomingPacket(pkt1);

  BOOST_CHECK(reliability->m_isIdleAckTimerRunning);
  BOOST_REQUIRE_EQUAL(reliability->m_ackQueue.size(), 1);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.front(), 765432);

  lp::Packet pkt2 = makeFrag(276, 40);
  pkt2.add<lp::TxSequenceField>(234567);

  reliability->processIncomingPacket(pkt2);

  BOOST_CHECK(reliability->m_isIdleAckTimerRunning);
  BOOST_REQUIRE_EQUAL(reliability->m_ackQueue.size(), 2);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.front(), 765432);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.back(), 234567);

  // T+5ms
  advanceClocks(time::milliseconds(1), 5);
  BOOST_CHECK(!reliability->m_isIdleAckTimerRunning);
}

BOOST_AUTO_TEST_CASE(PiggybackAcks)
{
  reliability->m_ackQueue.push(256);
  reliability->m_ackQueue.push(257);
  reliability->m_ackQueue.push(10);

  lp::Packet pkt;
  linkService->sendLpPackets({pkt});

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet sentPkt(transport->sentPackets.front().packet);

  BOOST_REQUIRE_EQUAL(sentPkt.count<lp::AckField>(), 3);
  BOOST_CHECK_EQUAL(sentPkt.get<lp::AckField>(0), 256);
  BOOST_CHECK_EQUAL(sentPkt.get<lp::AckField>(1), 257);
  BOOST_CHECK_EQUAL(sentPkt.get<lp::AckField>(2), 10);
  BOOST_CHECK(!sentPkt.has<lp::TxSequenceField>());

  BOOST_CHECK_EQUAL(reliability->m_ackQueue.size(), 0);
}

BOOST_AUTO_TEST_CASE(PiggybackAcksMtu)
{
  // MTU is 1500, payload has 60 octets plus 6 octets for LpPacket and Fragment TL and 10 octets for
  // TxSequence, leaving 1426 octets for piggybacking. Each Ack header is 12 octets, so each
  // LpPacket can carry 118 Acks, and it takes 9 LpPackets for 1000 Acks.

  transport->setMtu(1500);

  std::unordered_set<lp::Sequence> expectedAcks;
  for (lp::Sequence i = 1000; i < 2000; i++) {
    reliability->m_ackQueue.push(i);
    expectedAcks.insert(i);
  }

  for (uint32_t i = 1; i <= 9; i++) {
    lp::Packet pkt = makeFrag(i, 60);
    linkService->sendLpPackets({pkt});

    BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), i);
    lp::Packet sentPkt(transport->sentPackets.back().packet);
    BOOST_CHECK_EQUAL(getPktNo(sentPkt), i);
    BOOST_CHECK(sentPkt.has<lp::AckField>());

    for (lp::Sequence ack : sentPkt.list<lp::AckField>()) {
      BOOST_CHECK_EQUAL(expectedAcks.erase(ack), 1);
    }
  }

  BOOST_CHECK(reliability->m_ackQueue.empty());
  BOOST_CHECK(expectedAcks.empty());
}

BOOST_AUTO_TEST_CASE(PiggybackAcksMtuNoSpace)
{
  // MTU is 250, payload has 230 octets plus 4 octets for LpPacket and Fragment TL and 10 octets for
  // TxSequence, leaving 6 octets for piggybacking. Each Ack header is 12 octets, so there's no room
  // to piggyback any Ack in LpPacket.

  transport->setMtu(250);

  for (lp::Sequence i = 1000; i < 1100; i++) {
    reliability->m_ackQueue.push(i);
  }

  lp::Packet pkt = makeFrag(1, 230);
  linkService->sendLpPackets({pkt});

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet sentPkt(transport->sentPackets.back().packet);
  BOOST_CHECK_EQUAL(getPktNo(sentPkt), 1);
  BOOST_CHECK(!sentPkt.has<lp::AckField>());

  BOOST_CHECK_EQUAL(reliability->m_ackQueue.size(), 100);
}

BOOST_AUTO_TEST_CASE(StartIdleAckTimer)
{
  BOOST_CHECK(!reliability->m_isIdleAckTimerRunning);

  lp::Packet pkt1 = makeFrag(1, 100);
  pkt1.add<lp::TxSequenceField>(12);
  reliability->processIncomingPacket({pkt1});
  BOOST_CHECK(reliability->m_isIdleAckTimerRunning);

  // T+1ms
  advanceClocks(time::milliseconds(1), 1);
  BOOST_CHECK(reliability->m_isIdleAckTimerRunning);

  lp::Packet pkt2 = makeFrag(2, 100);
  pkt2.add<lp::TxSequenceField>(13);
  reliability->processIncomingPacket({pkt2});
  BOOST_CHECK(reliability->m_isIdleAckTimerRunning);

  // T+5ms
  advanceClocks(time::milliseconds(1), 4);
  BOOST_CHECK(!reliability->m_isIdleAckTimerRunning);

  lp::Packet pkt3 = makeFrag(3, 100);
  pkt3.add<lp::TxSequenceField>(15);
  reliability->processIncomingPacket({pkt3});
  BOOST_CHECK(reliability->m_isIdleAckTimerRunning);

  // T+9ms
  advanceClocks(time::milliseconds(1), 4);
  BOOST_CHECK(reliability->m_isIdleAckTimerRunning);

  // T+10ms
  advanceClocks(time::milliseconds(1), 1);
  BOOST_CHECK(!reliability->m_isIdleAckTimerRunning);
}

BOOST_AUTO_TEST_CASE(IdleAckTimer)
{
  // T+0ms: populate ack queue and start idle ack timer
  std::unordered_set<lp::Sequence> expectedAcks;
  for (lp::Sequence i = 1000; i < 1500; i++) {
    reliability->m_ackQueue.push(i);
    expectedAcks.insert(i);
  }
  BOOST_CHECK(!reliability->m_isIdleAckTimerRunning);
  reliability->startIdleAckTimer();
  BOOST_CHECK(reliability->m_isIdleAckTimerRunning);

  // T+4ms: idle ack timer has not yet expired, no IDLE packet generated
  advanceClocks(time::milliseconds(1), 4);
  BOOST_CHECK(reliability->m_isIdleAckTimerRunning);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.size(), 500);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.front(), 1000);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.back(), 1499);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 0);

  // T+5ms: idle ack timer expires, IDLE packet generated
  advanceClocks(time::milliseconds(1), 1);
  BOOST_CHECK(!reliability->m_isIdleAckTimerRunning);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.size(), 0);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);

  lp::Packet sentPkt(transport->sentPackets.back().packet);
  BOOST_CHECK(!sentPkt.has<lp::TxSequenceField>());
  for (lp::Sequence ack : sentPkt.list<lp::AckField>()) {
    BOOST_CHECK_EQUAL(expectedAcks.erase(ack), 1);
  }
  BOOST_CHECK(expectedAcks.empty());
}

BOOST_AUTO_TEST_CASE(IdleAckTimerMtu)
{
  transport->setMtu(1500);

  // T+0ms: populate ack queue and start idle ack timer
  std::unordered_set<lp::Sequence> expectedAcks;
  for (lp::Sequence i = 1000; i < 1500; i++) {
    reliability->m_ackQueue.push(i);
    expectedAcks.insert(i);
  }
  BOOST_CHECK(!reliability->m_isIdleAckTimerRunning);
  reliability->startIdleAckTimer();
  BOOST_CHECK(reliability->m_isIdleAckTimerRunning);

  // T+4ms: idle ack timer has not yet expired, no IDLE packet generated
  advanceClocks(time::milliseconds(1), 4);
  BOOST_CHECK(reliability->m_isIdleAckTimerRunning);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.size(), 500);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.front(), 1000);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.back(), 1499);
  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 0);

  // T+5ms: idle ack timer expires, IDLE packets generated
  advanceClocks(time::milliseconds(1), 1);
  BOOST_CHECK(!reliability->m_isIdleAckTimerRunning);
  BOOST_CHECK_EQUAL(reliability->m_ackQueue.size(), 0);

  // MTU is 1500. LpPacket TL occupies 4 octets. Each Ack header is 12 octets. There are room for
  // 124 Acks per LpPacket, and it takes 5 LpPackets to carry 500 Acks.
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 5);
  for (size_t i = 0; i < 5; i++) {
    lp::Packet sentPkt(transport->sentPackets[i].packet);
    BOOST_CHECK(!sentPkt.has<lp::TxSequenceField>());
    BOOST_CHECK_EQUAL(sentPkt.count<lp::AckField>(), i == 4 ? 4 : 124);
    for (lp::Sequence ack : sentPkt.list<lp::AckField>()) {
      BOOST_CHECK_EQUAL(expectedAcks.erase(ack), 1);
    }
  }

  BOOST_CHECK(expectedAcks.empty());
}

BOOST_AUTO_TEST_SUITE_END() // TestLpReliability
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
