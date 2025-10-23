/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2025,  Regents of the University of California,
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

#include "face/generic-link-service.hpp"
#include "face/face.hpp"

#include "tests/test-common.hpp"
#include "tests/key-chain-fixture.hpp"
#include "tests/daemon/global-io-fixture.hpp"
#include "dummy-transport.hpp"

#include <ndn-cxx/lp/fields.hpp>
#include <ndn-cxx/lp/tags.hpp>

#include <cmath>

namespace nfd::tests {

using namespace nfd::face;

BOOST_AUTO_TEST_SUITE(Face)

using nfd::Face;

class GenericLinkServiceFixture : public GlobalIoTimeFixture, public KeyChainFixture
{
protected:
  GenericLinkServiceFixture()
  {
    // By default, GenericLinkService is created with default options.
    // Test cases may invoke initialize() with alternate options.
    initialize({});
  }

  void
  initialize(const GenericLinkService::Options& options,
             ssize_t mtu = MTU_UNLIMITED,
             ssize_t sendQueueCapacity = QUEUE_UNSUPPORTED)
  {
    face = make_unique<Face>(make_unique<GenericLinkService>(options),
                             make_unique<DummyTransport>("dummy://", "dummy://",
                                                         ndn::nfd::FACE_SCOPE_NON_LOCAL,
                                                         ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                                                         ndn::nfd::LINK_TYPE_POINT_TO_POINT,
                                                         mtu, sendQueueCapacity));
    service = static_cast<GenericLinkService*>(face->getLinkService());
    transport = static_cast<DummyTransport*>(face->getTransport());

    face->afterReceiveInterest.connect(
      [this] (const Interest& interest, const EndpointId&) { receivedInterests.push_back(interest); });
    face->afterReceiveData.connect(
      [this] (const Data& data, const EndpointId&) { receivedData.push_back(data); });
    face->afterReceiveNack.connect(
      [this] (const lp::Nack& nack, const EndpointId&) { receivedNacks.push_back(nack); });
  }

  lp::PrefixAnnouncementHeader
  makePrefixAnnHeader(const Name& announcedName)
  {
    return lp::PrefixAnnouncementHeader{signPrefixAnn(makePrefixAnn(announcedName, 1_h),
                                                      m_keyChain, ndn::signingWithSha256())};
  }

protected:
  unique_ptr<Face> face;
  GenericLinkService* service = nullptr;
  DummyTransport* transport = nullptr;
  std::vector<Interest> receivedInterests;
  std::vector<Data> receivedData;
  std::vector<lp::Nack> receivedNacks;
};

BOOST_FIXTURE_TEST_SUITE(TestGenericLinkService, GenericLinkServiceFixture)

BOOST_AUTO_TEST_SUITE(SimpleSendReceive) // send and receive without other fields

BOOST_AUTO_TEST_CASE(SendInterest)
{
  // Initialize with Options that disables all services
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  auto interest1 = makeInterest("/localhost/test");
  face->sendInterest(*interest1);

  BOOST_CHECK_EQUAL(service->getCounters().nOutInterests, 1);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet interest1pkt(transport->sentPackets.back());
  BOOST_CHECK(interest1pkt.has<lp::FragmentField>());
  BOOST_CHECK(!interest1pkt.has<lp::SequenceField>());
}

BOOST_AUTO_TEST_CASE(SendData)
{
  // Initialize with Options that disables all services
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  auto data1 = makeData("/localhost/test");
  face->sendData(*data1);

  BOOST_CHECK_EQUAL(service->getCounters().nOutData, 1);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet data1pkt(transport->sentPackets.back());
  BOOST_CHECK(data1pkt.has<lp::FragmentField>());
  BOOST_CHECK(!data1pkt.has<lp::SequenceField>());
}

BOOST_AUTO_TEST_CASE(SendDataOverrideMtu)
{
  // Initialize with Options that disables all services and does not override MTU
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  BOOST_CHECK_EQUAL(transport->getMtu(), MTU_UNLIMITED);
  BOOST_CHECK_EQUAL(service->getEffectiveMtu(), MTU_UNLIMITED);
  BOOST_CHECK_EQUAL(face->getMtu(), MTU_UNLIMITED);
  BOOST_CHECK_EQUAL(service->canOverrideMtuTo(MTU_UNLIMITED), false);
  BOOST_CHECK_EQUAL(service->canOverrideMtuTo(MTU_INVALID), false);
  // Attempts to override MTU will fail when transport MTU is unlimited
  BOOST_CHECK_EQUAL(service->canOverrideMtuTo(4000), false);

  // Initialize with Options that disables all services and overrides MTU (Transport MTU 8800)
  options.overrideMtu = MIN_MTU;
  initialize(options, ndn::MAX_NDN_PACKET_SIZE);

  // Ensure effective MTU is override value
  BOOST_CHECK_EQUAL(transport->getMtu(), ndn::MAX_NDN_PACKET_SIZE);
  BOOST_CHECK_EQUAL(service->getEffectiveMtu(), MIN_MTU);
  BOOST_CHECK_EQUAL(face->getMtu(), MIN_MTU);

  // Check MTU overrides with Transport MTU finite
  BOOST_CHECK_EQUAL(service->canOverrideMtuTo(MTU_UNLIMITED), false);
  BOOST_CHECK_EQUAL(service->canOverrideMtuTo(MTU_INVALID), false);
  BOOST_CHECK_EQUAL(service->canOverrideMtuTo(MIN_MTU - 1), false);
  BOOST_CHECK_EQUAL(service->canOverrideMtuTo(MIN_MTU), true);
  BOOST_CHECK_EQUAL(service->canOverrideMtuTo(4000), true);
  BOOST_CHECK_EQUAL(service->canOverrideMtuTo(20000), true);

  // Send Data with less than MIN_MTU octets
  auto data1 = makeData("/localhost");
  BOOST_CHECK_LE(data1->wireEncode().size(), MIN_MTU);
  face->sendData(*data1);

  BOOST_CHECK_EQUAL(service->getCounters().nOutData, 1);
  BOOST_CHECK_EQUAL(service->getCounters().nOutOverMtu, 0);

  // Send Data with more than MIN_MTU octets
  auto data2 = makeData("/localhost/test/1234567890/1234567890/1234567890/1234567890");
  BOOST_CHECK_GT(data2->wireEncode().size(), MIN_MTU);
  face->sendData(*data2);

  BOOST_CHECK_EQUAL(service->getCounters().nOutData, 2);
  BOOST_CHECK_EQUAL(service->getCounters().nOutOverMtu, 1);

  // Override MTU greater than the Transport's MTU will not be utilized
  options.overrideMtu = 5000;
  initialize(options, 4000);
  BOOST_CHECK_EQUAL(service->getEffectiveMtu(), 4000);
  BOOST_CHECK_EQUAL(face->getMtu(), 4000);
}

BOOST_AUTO_TEST_CASE(SendNack)
{
  // Initialize with Options that disables all services
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  auto nack1 = makeNack(*makeInterest("/localhost/test", false, std::nullopt, 323),
                        lp::NackReason::NO_ROUTE);
  face->sendNack(nack1);

  BOOST_CHECK_EQUAL(service->getCounters().nOutNacks, 1);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet nack1pkt(transport->sentPackets.back());
  BOOST_CHECK(nack1pkt.has<lp::NackField>());
  BOOST_CHECK(nack1pkt.has<lp::FragmentField>());
  BOOST_CHECK(!nack1pkt.has<lp::SequenceField>());
}

BOOST_AUTO_TEST_CASE(ReceiveBareInterest)
{
  // Initialize with Options that disables all services
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  auto interest1 = makeInterest("/23Rd9hEiR");
  transport->receivePacket(interest1->wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInInterests, 1);
  BOOST_REQUIRE_EQUAL(receivedInterests.size(), 1);
  BOOST_CHECK_EQUAL(receivedInterests.back().wireEncode(), interest1->wireEncode());
}

BOOST_AUTO_TEST_CASE(ReceiveInterest)
{
  // Initialize with Options that disables all services
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  auto interest1 = makeInterest("/23Rd9hEiR");
  lp::Packet lpPacket;
  lpPacket.set<lp::FragmentField>({interest1->wireEncode().begin(), interest1->wireEncode().end()});
  lpPacket.set<lp::SequenceField>(0); // force LpPacket encoding

  transport->receivePacket(lpPacket.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInInterests, 1);
  BOOST_REQUIRE_EQUAL(receivedInterests.size(), 1);
  BOOST_CHECK_EQUAL(receivedInterests.back().wireEncode(), interest1->wireEncode());
}

BOOST_AUTO_TEST_CASE(ReceiveBareData)
{
  // Initialize with Options that disables all services
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  auto data1 = makeData("/12345678");
  transport->receivePacket(data1->wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInData, 1);
  BOOST_REQUIRE_EQUAL(receivedData.size(), 1);
  BOOST_CHECK_EQUAL(receivedData.back().wireEncode(), data1->wireEncode());
}

BOOST_AUTO_TEST_CASE(ReceiveData)
{
  // Initialize with Options that disables all services
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  auto data1 = makeData("/12345689");
  lp::Packet lpPacket;
  lpPacket.set<lp::FragmentField>({data1->wireEncode().begin(), data1->wireEncode().end()});
  lpPacket.set<lp::SequenceField>(0); // force LpPacket encoding

  transport->receivePacket(lpPacket.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInData, 1);
  BOOST_REQUIRE_EQUAL(receivedData.size(), 1);
  BOOST_CHECK_EQUAL(receivedData.back().wireEncode(), data1->wireEncode());
}

BOOST_AUTO_TEST_CASE(ReceiveNack)
{
  // Initialize with Options that disables all services
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  auto nack1 = makeNack(*makeInterest("/localhost/test", false, std::nullopt, 323),
                        lp::NackReason::NO_ROUTE);
  lp::Packet lpPacket;
  lpPacket.set<lp::FragmentField>({nack1.getInterest().wireEncode().begin(),
                                   nack1.getInterest().wireEncode().end()});
  lpPacket.set<lp::NackField>(nack1.getHeader());

  transport->receivePacket(lpPacket.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInNacks, 1);
  BOOST_REQUIRE_EQUAL(receivedNacks.size(), 1);
  BOOST_CHECK_EQUAL(receivedNacks.back().getReason(), nack1.getReason());
  BOOST_CHECK_EQUAL(receivedNacks.back().getInterest().wireEncode(), nack1.getInterest().wireEncode());
}

BOOST_AUTO_TEST_CASE(ReceiveIdlePacket)
{
  // Initialize with Options that disables all services
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  lp::Packet lpPacket;
  lpPacket.set<lp::SequenceField>(0);

  transport->receivePacket(lpPacket.wireEncode());

  // IDLE packet should be ignored, but is not an error
  BOOST_CHECK_EQUAL(service->getCounters().nInLpInvalid, 0);
  BOOST_CHECK_EQUAL(receivedInterests.size(), 0);
  BOOST_CHECK_EQUAL(receivedData.size(), 0);
  BOOST_CHECK_EQUAL(receivedNacks.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // SimpleSendReceive

BOOST_AUTO_TEST_SUITE(Fragmentation)

BOOST_AUTO_TEST_CASE(FragmentationDisabledExceedMtuDrop)
{
  // Initialize with Options that disable fragmentation
  GenericLinkService::Options options;
  options.allowFragmentation = false;
  initialize(options);

  transport->setMtu(55);

  auto data = makeData("/test/data/123456789/987654321/123456789");
  face->sendData(*data);

  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 0);
  BOOST_CHECK_EQUAL(service->getCounters().nOutOverMtu, 1);
}

// It is possible for some interfaces (e.g., virtual Ethernet) to have their MTU set to zero
// This test case ensures that packets are dropped if the MTU is zero
BOOST_AUTO_TEST_CASE(FragmentationDisabledZeroMtuDrop)
{
  // Initialize with Options that disable fragmentation
  GenericLinkService::Options options;
  options.allowFragmentation = false;
  initialize(options);

  transport->setMtu(0);

  auto data = makeData("/test/data/123456789/987654321/123456789");
  face->sendData(*data);

  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 0);
  BOOST_CHECK_EQUAL(service->getCounters().nOutOverMtu, 1);
}

BOOST_AUTO_TEST_CASE(FragmentationUnlimitedMtu)
{
  // Initialize with Options that enable fragmentation
  GenericLinkService::Options options;
  options.allowFragmentation = true;
  initialize(options);

  transport->setMtu(MTU_UNLIMITED);

  auto data = makeData("/test/data/123456789/987654321/123456789");
  face->sendData(*data);

  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 1);
}

BOOST_AUTO_TEST_CASE(FragmentationUnderMtu)
{
  // Initialize with Options that enable fragmentation
  GenericLinkService::Options options;
  options.allowFragmentation = true;
  initialize(options);

  transport->setMtu(105);

  auto data = makeData("/test/data/123456789/987654321/123456789");
  face->sendData(*data);

  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 1);
}

BOOST_AUTO_TEST_CASE(FragmentationOverMtu)
{
  // Initialize with Options that enable fragmentation
  GenericLinkService::Options options;
  options.allowFragmentation = true;
  initialize(options);

  transport->setMtu(60);

  auto data = makeData("/test/data/123456789/987654321/123456789");
  face->sendData(*data);

  BOOST_CHECK_GT(transport->sentPackets.size(), 1);
}

// It is possible for some interfaces (e.g., virtual Ethernet) to have their MTU set to zero
// This test case ensures that packets are dropped if the MTU is zero
BOOST_AUTO_TEST_CASE(FragmentationZeroMtuDrop)
{
  // Initialize with Options that enable fragmentation
  GenericLinkService::Options options;
  options.allowFragmentation = true;
  initialize(options);

  transport->setMtu(0);

  auto data = makeData("/test/data/123456789/987654321/123456789");
  face->sendData(*data);

  BOOST_CHECK_EQUAL(transport->sentPackets.size(), 0);
  BOOST_CHECK_EQUAL(service->getCounters().nFragmentationErrors, 1);
}

BOOST_AUTO_TEST_CASE(ReassembleFragments)
{
  // Initialize with Options that enables reassembly
  GenericLinkService::Options options;
  options.allowReassembly = true;
  initialize(options);

  auto interest = makeInterest(
    "/mt7P130BHXmtLm5dwaY5dpUM6SWYNN2B05g7y3UhsQuLvDdnTWdNnTeEiLuW3FAbJRSG3tzQ0UfaSEgG9rvYHmsKtgPMag1Hj4Tr");
  lp::Packet packet(interest->wireEncode());

  // fragment the packet
  LpFragmenter fragmenter({});
  auto [isOk, frags] = fragmenter.fragmentPacket(packet, 100);
  BOOST_REQUIRE(isOk);
  BOOST_TEST(frags.size() > 1);

  // receive the fragments
  for (ssize_t fragIndex = frags.size() - 1; fragIndex >= 0; --fragIndex) {
    size_t sequence = 1000 + fragIndex;
    frags[fragIndex].add<lp::SequenceField>(sequence);

    transport->receivePacket(frags[fragIndex].wireEncode());

    if (fragIndex > 0) {
      BOOST_CHECK(receivedInterests.empty());
      BOOST_CHECK_EQUAL(service->getCounters().nReassembling, 1);
    }
    else {
      BOOST_CHECK_EQUAL(receivedInterests.size(), 1);
      BOOST_CHECK_EQUAL(receivedInterests.back().wireEncode(), interest->wireEncode());
      BOOST_CHECK_EQUAL(service->getCounters().nReassembling, 0);
    }
  }
}

BOOST_AUTO_TEST_CASE(ReassemblyDisabledDropFragIndex)
{
  // Initialize with Options that disables reassembly
  GenericLinkService::Options options;
  options.allowReassembly = false;
  initialize(options);

  auto interest = makeInterest("/IgFe6NvH");
  lp::Packet packet(interest->wireEncode());
  packet.set<lp::FragIndexField>(140);

  transport->receivePacket(packet.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInLpInvalid, 0); // not an error
  BOOST_CHECK(receivedInterests.empty());
}

BOOST_AUTO_TEST_CASE(ReassemblyDisabledDropFragCount)
{
  // Initialize with Options that disables reassembly
  GenericLinkService::Options options;
  options.allowReassembly = false;
  initialize(options);

  auto interest = makeInterest("/SeGmEjvIVX");
  lp::Packet packet(interest->wireEncode());
  packet.set<lp::FragCountField>(276);

  transport->receivePacket(packet.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInLpInvalid, 0); // not an error
  BOOST_CHECK(receivedInterests.empty());
}

BOOST_AUTO_TEST_SUITE_END() // Fragmentation

BOOST_AUTO_TEST_SUITE(Reliability)

BOOST_AUTO_TEST_CASE(SendInterest)
{
  // Initialize with Options that enables reliability
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  options.reliabilityOptions.isEnabled = true;
  initialize(options);

  auto interest1 = makeInterest("/localhost/test");
  face->sendInterest(*interest1);

  BOOST_CHECK_EQUAL(service->getCounters().nOutInterests, 1);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet interest1pkt(transport->sentPackets.back());
  BOOST_CHECK(interest1pkt.has<lp::FragmentField>());
  BOOST_CHECK(interest1pkt.has<lp::TxSequenceField>());
}

BOOST_AUTO_TEST_CASE(SendData)
{
  // Initialize with Options that enables reliability
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  options.reliabilityOptions.isEnabled = true;
  initialize(options);

  auto data1 = makeData("/localhost/test");
  face->sendData(*data1);

  BOOST_CHECK_EQUAL(service->getCounters().nOutData, 1);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet data1pkt(transport->sentPackets.back());
  BOOST_CHECK(data1pkt.has<lp::FragmentField>());
  BOOST_CHECK(data1pkt.has<lp::TxSequenceField>());
}

BOOST_AUTO_TEST_CASE(SendNack)
{
  // Initialize with Options that enables reliability
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  options.reliabilityOptions.isEnabled = true;
  initialize(options);

  auto nack1 = makeNack(*makeInterest("/localhost/test", false, std::nullopt, 323),
                        lp::NackReason::NO_ROUTE);
  face->sendNack(nack1);

  BOOST_CHECK_EQUAL(service->getCounters().nOutNacks, 1);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet nack1pkt(transport->sentPackets.back());
  BOOST_CHECK(nack1pkt.has<lp::NackField>());
  BOOST_CHECK(nack1pkt.has<lp::FragmentField>());
  BOOST_CHECK(nack1pkt.has<lp::TxSequenceField>());
}

BOOST_AUTO_TEST_CASE(DropDuplicatePacket)
{
  // Initialize with Options that enables reliability
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  options.reliabilityOptions.isEnabled = true;
  initialize(options);

  Interest interest("/test/prefix");
  lp::Packet pkt1;
  pkt1.add<lp::FragmentField>({interest.wireEncode().begin(), interest.wireEncode().end()});
  pkt1.add<lp::SequenceField>(7);
  pkt1.add<lp::TxSequenceField>(12);
  transport->receivePacket(pkt1.wireEncode());
  BOOST_CHECK_EQUAL(service->getCounters().nInInterests, 1);
  BOOST_CHECK_EQUAL(service->getCounters().nDuplicateSequence, 0);

  lp::Packet pkt2;
  pkt2.add<lp::FragmentField>({interest.wireEncode().begin(), interest.wireEncode().end()});
  pkt2.add<lp::SequenceField>(7);
  pkt2.add<lp::TxSequenceField>(13);
  transport->receivePacket(pkt2.wireEncode());
  BOOST_CHECK_EQUAL(service->getCounters().nInInterests, 1);
  BOOST_CHECK_EQUAL(service->getCounters().nDuplicateSequence, 1);
}

BOOST_AUTO_TEST_SUITE_END() // Reliability

// congestion detection and marking
BOOST_AUTO_TEST_SUITE(CongestionMark)

BOOST_AUTO_TEST_CASE(NoCongestion)
{
  GenericLinkService::Options options;
  options.allowCongestionMarking = true;
  options.baseCongestionMarkingInterval = 100_ms;
  initialize(options, MTU_UNLIMITED, 65536);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, time::steady_clock::time_point::max());
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 0);

  auto interest = makeInterest("/12345678");

  // congestion threshold will be 32768 bytes, since min(65536, 65536 / 2) = 32768 bytes

  // no congestion
  transport->setSendQueueLength(0);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet pkt1(transport->sentPackets.back());
  BOOST_CHECK_EQUAL(pkt1.count<lp::CongestionMarkField>(), 0);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, time::steady_clock::time_point::max());
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 0);

  // no congestion
  transport->setSendQueueLength(32768);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 2);
  lp::Packet pkt2(transport->sentPackets.back());
  BOOST_CHECK_EQUAL(pkt2.count<lp::CongestionMarkField>(), 0);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, time::steady_clock::time_point::max());
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 0);
}

BOOST_AUTO_TEST_CASE(CongestionCoDel)
{
  GenericLinkService::Options options;
  options.allowCongestionMarking = true;
  options.baseCongestionMarkingInterval = 100_ms;
  initialize(options, MTU_UNLIMITED, 65536);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, time::steady_clock::time_point::max());
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 0);

  auto interest = makeInterest("/12345678");

  // first congested packet, will not be marked
  transport->setSendQueueLength(65537);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet pkt0(transport->sentPackets.back());
  BOOST_REQUIRE_EQUAL(pkt0.count<lp::CongestionMarkField>(), 0);
  auto nextMarkTime = time::steady_clock::now() + 100_ms;
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);

  time::nanoseconds markingInterval(
        static_cast<time::nanoseconds::rep>(options.baseCongestionMarkingInterval.count() /
                                            std::sqrt(service->m_nMarkedSinceInMarkingState + 1)));

  advanceClocks(markingInterval + 1_ms);
  face->sendInterest(*interest);
  lp::Packet pkt1(transport->sentPackets.back());

  // First congestion mark appears after one interval (100 ms)
  BOOST_REQUIRE_EQUAL(pkt1.count<lp::CongestionMarkField>(), 1);
  BOOST_CHECK_EQUAL(pkt1.get<lp::CongestionMarkField>(), 1);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 1);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 1);

  // advance clock to half of marking interval cycle
  advanceClocks(markingInterval / 2); // 50ms

  // second congested packet, but within marking interval, will not be marked
  transport->setSendQueueLength(66000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 3);
  lp::Packet pkt2(transport->sentPackets.back());
  BOOST_CHECK_EQUAL(pkt2.count<lp::CongestionMarkField>(), 0);

  markingInterval = time::nanoseconds(
      static_cast<time::nanoseconds::rep>(options.baseCongestionMarkingInterval.count() /
                                          std::sqrt(service->m_nMarkedSinceInMarkingState + 1)));
  nextMarkTime += markingInterval;

  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 1);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 1);

  // advance clocks past end of initial interval cycle
  this->advanceClocks((markingInterval / 2) + 1_ms);

  // first congested packet after waiting marking interval, will be marked
  transport->setSendQueueLength(66000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 4);
  lp::Packet pkt3(transport->sentPackets.back());
  BOOST_REQUIRE_EQUAL(pkt3.count<lp::CongestionMarkField>(), 1);
  BOOST_CHECK_EQUAL(pkt3.get<lp::CongestionMarkField>(), 1);
  markingInterval = time::nanoseconds(
      static_cast<time::nanoseconds::rep>(options.baseCongestionMarkingInterval.count() /
                                          std::sqrt(service->m_nMarkedSinceInMarkingState + 1)));
  nextMarkTime += markingInterval;
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 2);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 2);

  // advance clock partway through current marking interval
  this->advanceClocks(markingInterval - 20_ms);

  // still congested, but within marking interval cycle
  transport->setSendQueueLength(66000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 5);
  lp::Packet pkt4(transport->sentPackets.back());
  BOOST_CHECK_EQUAL(pkt4.count<lp::CongestionMarkField>(), 0);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 2);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 2);

  // advance clocks past end of current marking interval cycle
  this->advanceClocks(21_ms);

  // still congested, after marking interval cycle
  transport->setSendQueueLength(66000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 6);
  lp::Packet pkt5(transport->sentPackets.back());
  BOOST_REQUIRE_EQUAL(pkt5.count<lp::CongestionMarkField>(), 1);
  BOOST_CHECK_EQUAL(pkt5.get<lp::CongestionMarkField>(), 1);
  markingInterval = time::nanoseconds(
    static_cast<time::nanoseconds::rep>(options.baseCongestionMarkingInterval.count() /
                                        std::sqrt(service->m_nMarkedSinceInMarkingState + 1)));
  nextMarkTime += markingInterval;
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 3);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 3);

  this->advanceClocks(1_ms);

  // still congested, but within marking interval cycle
  transport->setSendQueueLength(66000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 7);
  lp::Packet pkt6(transport->sentPackets.back());
  BOOST_CHECK_EQUAL(pkt6.count<lp::CongestionMarkField>(), 0);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 3);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 3);

  this->advanceClocks(markingInterval);

  // still congested, after marking interval cycle
  transport->setSendQueueLength(66000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 8);
  lp::Packet pkt7(transport->sentPackets.back());
  BOOST_REQUIRE_EQUAL(pkt7.count<lp::CongestionMarkField>(), 1);
  BOOST_CHECK_EQUAL(pkt7.get<lp::CongestionMarkField>(), 1);
  markingInterval = time::nanoseconds(
    static_cast<time::nanoseconds::rep>(options.baseCongestionMarkingInterval.count() /
                                        std::sqrt(service->m_nMarkedSinceInMarkingState + 1)));
  nextMarkTime += markingInterval;
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 4);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 4);

  // no more congestion
  transport->setSendQueueLength(30000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 9);
  lp::Packet pkt8(transport->sentPackets.back());
  BOOST_CHECK_EQUAL(pkt8.count<lp::CongestionMarkField>(), 0);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, time::steady_clock::time_point::max());
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 4);

  this->advanceClocks(50_ms);

  // send queue congested again, but can't mark packet because within one full interval of last mark
  transport->setSendQueueLength(66000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 10);
  lp::Packet pkt9(transport->sentPackets.back());
  BOOST_CHECK_EQUAL(pkt9.count<lp::CongestionMarkField>(), 0);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  markingInterval = time::nanoseconds(
      static_cast<time::nanoseconds::rep>(options.baseCongestionMarkingInterval.count() /
                                          std::sqrt(service->m_nMarkedSinceInMarkingState + 1)));
  nextMarkTime = time::steady_clock::now() + markingInterval;
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 4);

  // advance clock past full 100ms interval since last mark
  this->advanceClocks(markingInterval + 2_ms);
  BOOST_CHECK_GT(time::steady_clock::now(), nextMarkTime);

  transport->setSendQueueLength(66000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 11);
  lp::Packet pkt10(transport->sentPackets.back());
  BOOST_REQUIRE_EQUAL(pkt10.count<lp::CongestionMarkField>(), 1);
  BOOST_CHECK_EQUAL(pkt10.get<lp::CongestionMarkField>(), 1);
  markingInterval = time::nanoseconds(
        static_cast<time::nanoseconds::rep>(options.baseCongestionMarkingInterval.count() /
                                            std::sqrt(service->m_nMarkedSinceInMarkingState + 1)));
  nextMarkTime += markingInterval;
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 1);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 5);

  // advance clock partway through 100ms marking interval
  this->advanceClocks(50_ms);

  // not marked since within 100ms window before can mark again
  transport->setSendQueueLength(66000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 12);
  lp::Packet pkt11(transport->sentPackets.back());
  BOOST_CHECK_EQUAL(pkt11.count<lp::CongestionMarkField>(), 0);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 1);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 5);

  // advance clocks past m_nextMarkTime
  this->advanceClocks(51_ms);

  // markable packet, queue length still above threshold
  transport->setSendQueueLength(66000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 13);
  lp::Packet pkt12(transport->sentPackets.back());
  BOOST_REQUIRE_EQUAL(pkt12.count<lp::CongestionMarkField>(), 1);
  BOOST_CHECK_EQUAL(pkt12.get<lp::CongestionMarkField>(), 1);
  markingInterval = time::nanoseconds(
    static_cast<time::nanoseconds::rep>(options.baseCongestionMarkingInterval.count() /
                                        std::sqrt(service->m_nMarkedSinceInMarkingState + 1)));
  nextMarkTime += markingInterval;
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 2);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 6);

  // no more congestion
  transport->setSendQueueLength(10000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 14);
  lp::Packet pkt13(transport->sentPackets.back());
  BOOST_CHECK_EQUAL(pkt13.count<lp::CongestionMarkField>(), 0);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, time::steady_clock::time_point::max());
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 6);

  // advance clocks past one full interval since last mark
  this->advanceClocks(102_ms);

  // start congestion again
  transport->setSendQueueLength(66000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 15);
  lp::Packet pkt14(transport->sentPackets.back());
  BOOST_REQUIRE_EQUAL(pkt14.count<lp::CongestionMarkField>(), 0);
  nextMarkTime = time::steady_clock::now() + 100_ms;
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 6);

  // no more congestion, cancel marking interval
  transport->setSendQueueLength(5000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 16);
  lp::Packet pkt15(transport->sentPackets.back());
  BOOST_CHECK_EQUAL(pkt15.count<lp::CongestionMarkField>(), 0);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, time::steady_clock::time_point::max());
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 6);
}

BOOST_AUTO_TEST_CASE(DefaultThreshold)
{
  GenericLinkService::Options options;
  options.allowCongestionMarking = true;
  options.baseCongestionMarkingInterval = 100_ms;
  initialize(options, MTU_UNLIMITED, QUEUE_UNSUPPORTED);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, time::steady_clock::time_point::max());
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 0);

  auto interest = makeInterest("/12345678");

  // congestion threshold will be 65536 bytes, since the transport reports that it cannot measure
  // the queue capacity

  // no congestion
  transport->setSendQueueLength(0);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet pkt1(transport->sentPackets.back());
  BOOST_CHECK_EQUAL(pkt1.count<lp::CongestionMarkField>(), 0);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, time::steady_clock::time_point::max());
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 0);

  // no congestion
  transport->setSendQueueLength(65536);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 2);
  lp::Packet pkt2(transport->sentPackets.back());
  BOOST_CHECK_EQUAL(pkt2.count<lp::CongestionMarkField>(), 0);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, time::steady_clock::time_point::max());
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 0);

  // first congested (not marked yet) packet
  transport->setSendQueueLength(65537);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 3);
  lp::Packet pkt3(transport->sentPackets.back());
  BOOST_REQUIRE_EQUAL(pkt3.count<lp::CongestionMarkField>(), 0);
  auto nextMarkTime = time::steady_clock::now() + 100_ms;
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 0);
}

BOOST_AUTO_TEST_SUITE_END() // CongestionMark

BOOST_AUTO_TEST_SUITE(LpFields)

BOOST_AUTO_TEST_CASE(ReceiveNextHopFaceId)
{
  // Initialize with Options that enables local fields
  GenericLinkService::Options options;
  options.allowLocalFields = true;
  initialize(options);

  auto interest = makeInterest("/12345678");
  lp::Packet packet(interest->wireEncode());
  packet.set<lp::NextHopFaceIdField>(1000);

  transport->receivePacket(packet.wireEncode());

  BOOST_REQUIRE_EQUAL(receivedInterests.size(), 1);
  auto tag = receivedInterests.back().getTag<lp::NextHopFaceIdTag>();
  BOOST_REQUIRE(tag != nullptr);
  BOOST_CHECK_EQUAL(*tag, 1000);
}

BOOST_AUTO_TEST_CASE(ReceiveNextHopFaceIdDisabled)
{
  // Initialize with Options that disables local fields
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  auto interest = makeInterest("/12345678");
  lp::Packet packet(interest->wireEncode());
  packet.set<lp::NextHopFaceIdField>(1000);

  transport->receivePacket(packet.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInNetInvalid, 0); // not an error
  BOOST_CHECK(receivedInterests.empty());
}

BOOST_AUTO_TEST_CASE(ReceiveNextHopFaceIdDropData)
{
  // Initialize with Options that enables local fields
  GenericLinkService::Options options;
  options.allowLocalFields = true;
  initialize(options);

  auto data = makeData("/12345678");
  lp::Packet packet(data->wireEncode());
  packet.set<lp::NextHopFaceIdField>(1000);

  transport->receivePacket(packet.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInNetInvalid, 1);
  BOOST_CHECK(receivedData.empty());
}

BOOST_AUTO_TEST_CASE(ReceiveNextHopFaceIdDropNack)
{
  // Initialize with Options that enables local fields
  GenericLinkService::Options options;
  options.allowLocalFields = true;
  initialize(options);

  auto nack = makeNack(*makeInterest("/localhost/test", false, std::nullopt, 123),
                       lp::NackReason::NO_ROUTE);
  lp::Packet packet;
  packet.set<lp::FragmentField>({nack.getInterest().wireEncode().begin(),
                                 nack.getInterest().wireEncode().end()});
  packet.set<lp::NackField>(nack.getHeader());
  packet.set<lp::NextHopFaceIdField>(1000);

  transport->receivePacket(packet.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInNetInvalid, 1);
  BOOST_CHECK(receivedNacks.empty());
}

BOOST_AUTO_TEST_CASE(ReceiveCachePolicy)
{
  // Initialize with Options that disables local fields
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);
  // CachePolicy is unprivileged and does not require allowLocalFields option.

  auto data = makeData("/12345678");
  lp::Packet packet(data->wireEncode());
  packet.set<lp::CachePolicyField>(lp::CachePolicy().setPolicy(lp::CachePolicyType::NO_CACHE));

  transport->receivePacket(packet.wireEncode());

  BOOST_REQUIRE_EQUAL(receivedData.size(), 1);
  auto tag = receivedData.back().getTag<lp::CachePolicyTag>();
  BOOST_REQUIRE(tag != nullptr);
  BOOST_CHECK_EQUAL(tag->get().getPolicy(), lp::CachePolicyType::NO_CACHE);
}

BOOST_AUTO_TEST_CASE(ReceiveCachePolicyDropInterest)
{
  // Initialize with Options that enables local fields
  GenericLinkService::Options options;
  options.allowLocalFields = true;
  initialize(options);

  auto interest = makeInterest("/12345678");
  lp::Packet packet(interest->wireEncode());
  lp::CachePolicy policy;
  policy.setPolicy(lp::CachePolicyType::NO_CACHE);
  packet.set<lp::CachePolicyField>(policy);

  transport->receivePacket(packet.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInNetInvalid, 1);
  BOOST_CHECK(receivedInterests.empty());
}

BOOST_AUTO_TEST_CASE(ReceiveCachePolicyDropNack)
{
  // Initialize with Options that enables local fields
  GenericLinkService::Options options;
  options.allowLocalFields = true;
  initialize(options);

  lp::Nack nack = makeNack(*makeInterest("/localhost/test", false, std::nullopt, 123),
                           lp::NackReason::NO_ROUTE);
  lp::Packet packet(nack.getInterest().wireEncode());
  packet.set<lp::NackField>(nack.getHeader());
  lp::CachePolicy policy;
  policy.setPolicy(lp::CachePolicyType::NO_CACHE);
  packet.set<lp::CachePolicyField>(policy);

  transport->receivePacket(packet.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInNetInvalid, 1);
  BOOST_CHECK(receivedNacks.empty());
}

BOOST_AUTO_TEST_CASE(SendIncomingFaceId)
{
  // Initialize with Options that enables local fields
  GenericLinkService::Options options;
  options.allowLocalFields = true;
  initialize(options);

  auto interest = makeInterest("/12345678");
  interest->setTag(make_shared<lp::IncomingFaceIdTag>(1000));

  face->sendInterest(*interest);

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet sent(transport->sentPackets.back());
  BOOST_REQUIRE(sent.has<lp::IncomingFaceIdField>());
  BOOST_CHECK_EQUAL(sent.get<lp::IncomingFaceIdField>(), 1000);
}

BOOST_AUTO_TEST_CASE(SendIncomingFaceIdDisabled)
{
  // Initialize with Options that disables local fields
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  auto interest = makeInterest("/12345678");
  interest->setTag(make_shared<lp::IncomingFaceIdTag>(1000));

  face->sendInterest(*interest);

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet sent(transport->sentPackets.back());
  BOOST_CHECK(!sent.has<lp::IncomingFaceIdField>());
}

BOOST_AUTO_TEST_CASE(ReceiveIncomingFaceIdIgnoreInterest)
{
  // Initialize with Options that enables local fields
  GenericLinkService::Options options;
  options.allowLocalFields = true;
  initialize(options);

  auto interest = makeInterest("/12345678");
  lp::Packet packet(interest->wireEncode());
  packet.set<lp::IncomingFaceIdField>(1000);

  transport->receivePacket(packet.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInNetInvalid, 0); // not an error
  BOOST_REQUIRE_EQUAL(receivedInterests.size(), 1);
  BOOST_CHECK(receivedInterests.back().getTag<lp::IncomingFaceIdTag>() == nullptr);
}

BOOST_AUTO_TEST_CASE(ReceiveIncomingFaceIdIgnoreData)
{
  // Initialize with Options that enables local fields
  GenericLinkService::Options options;
  options.allowLocalFields = true;
  initialize(options);

  auto data = makeData("/z1megUh9Bj");
  lp::Packet packet(data->wireEncode());
  packet.set<lp::IncomingFaceIdField>(1000);

  transport->receivePacket(packet.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInNetInvalid, 0); // not an error
  BOOST_REQUIRE_EQUAL(receivedData.size(), 1);
  BOOST_CHECK(receivedData.back().getTag<lp::IncomingFaceIdTag>() == nullptr);
}

BOOST_AUTO_TEST_CASE(ReceiveIncomingFaceIdIgnoreNack)
{
  // Initialize with Options that enables local fields
  GenericLinkService::Options options;
  options.allowLocalFields = true;
  initialize(options);

  lp::Nack nack = makeNack(*makeInterest("/TPAhdiHz", false, std::nullopt, 278),
                           lp::NackReason::CONGESTION);
  lp::Packet packet(nack.getInterest().wireEncode());
  packet.set<lp::NackField>(nack.getHeader());
  packet.set<lp::IncomingFaceIdField>(1000);

  transport->receivePacket(packet.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInNetInvalid, 0); // not an error
  BOOST_REQUIRE_EQUAL(receivedNacks.size(), 1);
  BOOST_CHECK(receivedNacks.back().getTag<lp::IncomingFaceIdTag>() == nullptr);
}

BOOST_AUTO_TEST_CASE(SendCongestionMarkInterest)
{
  auto interest = makeInterest("/12345678");
  interest->setTag(make_shared<lp::CongestionMarkTag>(1));

  face->sendInterest(*interest);

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet sent(transport->sentPackets.back());
  BOOST_REQUIRE(sent.has<lp::CongestionMarkField>());
  BOOST_CHECK_EQUAL(sent.get<lp::CongestionMarkField>(), 1);
}

BOOST_AUTO_TEST_CASE(SendCongestionMarkData)
{
  auto data = makeData("/12345678");
  data->setTag(make_shared<lp::CongestionMarkTag>(0));

  face->sendData(*data);

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet sent(transport->sentPackets.back());
  BOOST_REQUIRE(sent.has<lp::CongestionMarkField>());
  BOOST_CHECK_EQUAL(sent.get<lp::CongestionMarkField>(), 0);
}

BOOST_AUTO_TEST_CASE(SendCongestionMarkNack)
{
  lp::Nack nack = makeNack(*makeInterest("/localhost/test", false, std::nullopt, 123),
                           lp::NackReason::NO_ROUTE);
  nack.setTag(make_shared<lp::CongestionMarkTag>(std::numeric_limits<uint64_t>::max()));

  face->sendNack(nack);

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet sent(transport->sentPackets.back());
  BOOST_REQUIRE(sent.has<lp::CongestionMarkField>());
  BOOST_CHECK_EQUAL(sent.get<lp::CongestionMarkField>(), std::numeric_limits<uint64_t>::max());
}

BOOST_AUTO_TEST_CASE(ReceiveCongestionMarkInterest)
{
  auto interest = makeInterest("/12345678");
  lp::Packet packet(interest->wireEncode());
  packet.set<lp::CongestionMarkField>(1);

  transport->receivePacket(packet.wireEncode());

  BOOST_REQUIRE_EQUAL(receivedInterests.size(), 1);
  auto tag = receivedInterests.back().getTag<lp::CongestionMarkTag>();
  BOOST_REQUIRE(tag != nullptr);
  BOOST_CHECK_EQUAL(*tag, 1);
}

BOOST_AUTO_TEST_CASE(ReceiveCongestionMarkData)
{
  auto data = makeData("/12345678");
  lp::Packet packet(data->wireEncode());
  packet.set<lp::CongestionMarkField>(1);

  transport->receivePacket(packet.wireEncode());

  BOOST_REQUIRE_EQUAL(receivedData.size(), 1);
  auto tag = receivedData.back().getTag<lp::CongestionMarkTag>();
  BOOST_REQUIRE(tag != nullptr);
  BOOST_CHECK_EQUAL(*tag, 1);
}

BOOST_AUTO_TEST_CASE(ReceiveCongestionMarkNack)
{
  auto nack = makeNack(*makeInterest("/localhost/test", false, std::nullopt, 123),
                       lp::NackReason::NO_ROUTE);
  lp::Packet packet;
  packet.set<lp::FragmentField>({nack.getInterest().wireEncode().begin(),
                                 nack.getInterest().wireEncode().end()});
  packet.set<lp::NackField>(nack.getHeader());
  packet.set<lp::CongestionMarkField>(1);

  transport->receivePacket(packet.wireEncode());

  BOOST_REQUIRE_EQUAL(receivedNacks.size(), 1);
  auto tag = receivedNacks.back().getTag<lp::CongestionMarkTag>();
  BOOST_REQUIRE(tag != nullptr);
  BOOST_CHECK_EQUAL(*tag, 1);
}

BOOST_AUTO_TEST_CASE(SendNonDiscovery)
{
  GenericLinkService::Options options;
  options.allowSelfLearning = true;
  initialize(options);

  auto interest = makeInterest("/12345678");
  interest->setTag(make_shared<lp::NonDiscoveryTag>(lp::EmptyValue{}));

  face->sendInterest(*interest);

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet sent(transport->sentPackets.back());
  BOOST_CHECK(sent.has<lp::NonDiscoveryField>());
}

BOOST_AUTO_TEST_CASE(SendNonDiscoveryDisabled)
{
  GenericLinkService::Options options;
  options.allowSelfLearning = false;
  initialize(options);

  auto interest = makeInterest("/12345678");
  interest->setTag(make_shared<lp::NonDiscoveryTag>(lp::EmptyValue{}));

  face->sendInterest(*interest);

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet sent(transport->sentPackets.back());
  BOOST_CHECK(!sent.has<lp::NonDiscoveryField>());
}

BOOST_AUTO_TEST_CASE(ReceiveNonDiscovery)
{
  GenericLinkService::Options options;
  options.allowSelfLearning = true;
  initialize(options);

  auto interest = makeInterest("/12345678");
  lp::Packet packet(interest->wireEncode());
  packet.set<lp::NonDiscoveryField>(lp::EmptyValue{});

  transport->receivePacket(packet.wireEncode());

  BOOST_REQUIRE_EQUAL(receivedInterests.size(), 1);
  auto tag = receivedInterests.back().getTag<lp::NonDiscoveryTag>();
  BOOST_CHECK(tag != nullptr);
}

BOOST_AUTO_TEST_CASE(ReceiveNonDiscoveryDisabled)
{
  GenericLinkService::Options options;
  options.allowSelfLearning = false;
  initialize(options);

  auto interest = makeInterest("/12345678");
  lp::Packet packet(interest->wireEncode());
  packet.set<lp::NonDiscoveryField>(lp::EmptyValue{});

  transport->receivePacket(packet.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInNetInvalid, 0); // not an error
  BOOST_CHECK_EQUAL(receivedInterests.size(), 1);
  auto tag = receivedInterests.back().getTag<lp::NonDiscoveryTag>();
  BOOST_CHECK(tag == nullptr);
}

BOOST_AUTO_TEST_CASE(ReceiveNonDiscoveryDropData)
{
  GenericLinkService::Options options;
  options.allowSelfLearning = true;
  initialize(options);

  auto data = makeData("/12345678");
  lp::Packet packet(data->wireEncode());
  packet.set<lp::NonDiscoveryField>(lp::EmptyValue{});

  transport->receivePacket(packet.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInNetInvalid, 1);
  BOOST_CHECK(receivedData.empty());
}

BOOST_AUTO_TEST_CASE(ReceiveNonDiscoveryDropNack)
{
  GenericLinkService::Options options;
  options.allowSelfLearning = true;
  initialize(options);

  auto nack = makeNack(*makeInterest("/localhost/test", false, std::nullopt, 123),
                       lp::NackReason::NO_ROUTE);
  lp::Packet packet;
  packet.set<lp::FragmentField>({nack.getInterest().wireEncode().begin(),
                                 nack.getInterest().wireEncode().end()});
  packet.set<lp::NackField>(nack.getHeader());
  packet.set<lp::NonDiscoveryField>(lp::EmptyValue{});

  transport->receivePacket(packet.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInNetInvalid, 1);
  BOOST_CHECK(receivedNacks.empty());
}

BOOST_AUTO_TEST_CASE(SendPrefixAnnouncement)
{
  GenericLinkService::Options options;
  options.allowSelfLearning = true;
  initialize(options);

  auto data = makeData("/12345678");
  auto pah = makePrefixAnnHeader("/local/ndn/prefix");
  data->setTag(make_shared<lp::PrefixAnnouncementTag>(pah));

  face->sendData(*data);

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet sent(transport->sentPackets.back());
  BOOST_CHECK(sent.has<lp::PrefixAnnouncementField>());
}

BOOST_AUTO_TEST_CASE(SendPrefixAnnouncementDisabled)
{
  GenericLinkService::Options options;
  options.allowSelfLearning = false;
  initialize(options);

  auto data = makeData("/12345678");
  auto pah = makePrefixAnnHeader("/local/ndn/prefix");
  data->setTag(make_shared<lp::PrefixAnnouncementTag>(pah));

  face->sendData(*data);

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet sent(transport->sentPackets.back());
  BOOST_CHECK(!sent.has<lp::PrefixAnnouncementField>());
}

BOOST_AUTO_TEST_CASE(ReceivePrefixAnnouncement)
{
  GenericLinkService::Options options;
  options.allowSelfLearning = true;
  initialize(options);

  auto data = makeData("/12345678");
  lp::Packet packet(data->wireEncode());
  auto pah = makePrefixAnnHeader("/local/ndn/prefix");
  packet.set<lp::PrefixAnnouncementField>(pah);

  transport->receivePacket(packet.wireEncode());

  BOOST_REQUIRE_EQUAL(receivedData.size(), 1);
  auto tag = receivedData.back().getTag<lp::PrefixAnnouncementTag>();
  BOOST_CHECK_EQUAL(tag->get().getPrefixAnn()->getAnnouncedName(), "/local/ndn/prefix");
}

BOOST_AUTO_TEST_CASE(ReceivePrefixAnnouncementDisabled)
{
  GenericLinkService::Options options;
  options.allowSelfLearning = false;
  initialize(options);

  auto data = makeData("/12345678");
  lp::Packet packet(data->wireEncode());
  auto pah = makePrefixAnnHeader("/local/ndn/prefix");
  packet.set<lp::PrefixAnnouncementField>(pah);

  transport->receivePacket(packet.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInNetInvalid, 0); // not an error
  BOOST_CHECK_EQUAL(receivedData.size(), 1);
  auto tag = receivedData.back().getTag<lp::PrefixAnnouncementTag>();
  BOOST_CHECK(tag == nullptr);
}

BOOST_AUTO_TEST_CASE(ReceivePrefixAnnouncementDropInterest)
{
  GenericLinkService::Options options;
  options.allowSelfLearning = true;
  initialize(options);

  auto interest = makeInterest("/12345678");
  lp::Packet packet(interest->wireEncode());
  auto pah = makePrefixAnnHeader("/local/ndn/prefix");
  packet.set<lp::PrefixAnnouncementField>(pah);

  transport->receivePacket(packet.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInNetInvalid, 1);
  BOOST_CHECK(receivedInterests.empty());
}

BOOST_AUTO_TEST_CASE(ReceivePrefixAnnouncementDropNack)
{
  GenericLinkService::Options options;
  options.allowSelfLearning = true;
  initialize(options);

  auto nack = makeNack(*makeInterest("/localhost/test", false, std::nullopt, 123),
                       lp::NackReason::NO_ROUTE);
  lp::Packet packet;
  packet.set<lp::FragmentField>({nack.getInterest().wireEncode().begin(),
                                 nack.getInterest().wireEncode().end()});
  packet.set<lp::NackField>(nack.getHeader());
  auto pah = makePrefixAnnHeader("/local/ndn/prefix");
  packet.set<lp::PrefixAnnouncementField>(pah);

  transport->receivePacket(packet.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInNetInvalid, 1);
  BOOST_CHECK(receivedNacks.empty());
}

BOOST_AUTO_TEST_SUITE_END() // LpFields

BOOST_AUTO_TEST_SUITE(Malformed) // receive malformed packets

BOOST_AUTO_TEST_CASE(WrongTlvType)
{
  // Initialize with Options that disables all services
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  auto packet = ndn::encoding::makeEmptyBlock(tlv::Name);
  transport->receivePacket(packet);

  BOOST_CHECK_EQUAL(service->getCounters().nInLpInvalid, 1);
  BOOST_CHECK_EQUAL(receivedInterests.size(), 0);
  BOOST_CHECK_EQUAL(receivedData.size(), 0);
  BOOST_CHECK_EQUAL(receivedNacks.size(), 0);
}

BOOST_AUTO_TEST_CASE(Unparsable)
{
  // Initialize with Options that disables all services
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  auto packet = ndn::encoding::makeStringBlock(lp::tlv::LpPacket, "x");
  BOOST_CHECK_THROW(packet.parse(), tlv::Error);
  transport->receivePacket(packet);

  BOOST_CHECK_EQUAL(service->getCounters().nInLpInvalid, 1);
  BOOST_CHECK_EQUAL(receivedInterests.size(), 0);
  BOOST_CHECK_EQUAL(receivedData.size(), 0);
  BOOST_CHECK_EQUAL(receivedNacks.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // Malformed

BOOST_AUTO_TEST_SUITE_END() // TestGenericLinkService
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace nfd::tests
