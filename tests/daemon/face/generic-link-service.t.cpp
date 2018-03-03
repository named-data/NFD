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

#include "face/generic-link-service.hpp"
#include "face/face.hpp"

#include "dummy-transport.hpp"
#include "tests/test-common.hpp"

#include <ndn-cxx/lp/empty-value.hpp>
#include <ndn-cxx/lp/prefix-announcement.hpp>
#include <ndn-cxx/lp/tags.hpp>

namespace nfd {
namespace face {
namespace tests {

using namespace nfd::tests;

static lp::PrefixAnnouncement
makePrefixAnnouncement(Name announcedName)
{
  Name paName = Name("self-learning").append(announcedName).appendVersion();
  return lp::PrefixAnnouncement(makeData(paName));
}

BOOST_AUTO_TEST_SUITE(Face)

using nfd::Face;

class GenericLinkServiceFixture : public UnitTestTimeFixture
{
protected:
  GenericLinkServiceFixture()
    : service(nullptr)
    , transport(nullptr)
  {
    this->initialize(GenericLinkService::Options());
    // By default, GenericLinkService is created with default options.
    // Test cases may invoke .initialize with alternate options.
  }

  void
  initialize(const GenericLinkService::Options& options,
             ssize_t mtu = MTU_UNLIMITED,
             ssize_t sendQueueCapacity = QUEUE_UNSUPPORTED)
  {
    face.reset(new Face(make_unique<GenericLinkService>(options),
                        make_unique<DummyTransport>("dummy://",
                                                    "dummy://",
                                                    ndn::nfd::FACE_SCOPE_NON_LOCAL,
                                                    ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                                                    ndn::nfd::LINK_TYPE_POINT_TO_POINT,
                                                    mtu,
                                                    sendQueueCapacity)));
    service = static_cast<GenericLinkService*>(face->getLinkService());
    transport = static_cast<DummyTransport*>(face->getTransport());

    face->afterReceiveInterest.connect(
      [this] (const Interest& interest) { receivedInterests.push_back(interest); });
    face->afterReceiveData.connect(
      [this] (const Data& data) { receivedData.push_back(data); });
    face->afterReceiveNack.connect(
      [this] (const lp::Nack& nack) { receivedNacks.push_back(nack); });
  }

protected:
  unique_ptr<Face> face;
  GenericLinkService* service;
  DummyTransport* transport;
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

  shared_ptr<Interest> interest1 = makeInterest("/localhost/test");

  face->sendInterest(*interest1);

  BOOST_CHECK_EQUAL(service->getCounters().nOutInterests, 1);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet interest1pkt;
  BOOST_REQUIRE_NO_THROW(interest1pkt.wireDecode(transport->sentPackets.back().packet));
  BOOST_CHECK(interest1pkt.has<lp::FragmentField>());
  BOOST_CHECK(!interest1pkt.has<lp::SequenceField>());
}

BOOST_AUTO_TEST_CASE(SendData)
{
  // Initialize with Options that disables all services
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  shared_ptr<Data> data1 = makeData("/localhost/test");

  face->sendData(*data1);

  BOOST_CHECK_EQUAL(service->getCounters().nOutData, 1);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet data1pkt;
  BOOST_REQUIRE_NO_THROW(data1pkt.wireDecode(transport->sentPackets.back().packet));
  BOOST_CHECK(data1pkt.has<lp::FragmentField>());
  BOOST_CHECK(!data1pkt.has<lp::SequenceField>());
}

BOOST_AUTO_TEST_CASE(SendNack)
{
  // Initialize with Options that disables all services
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  lp::Nack nack1 = makeNack("/localhost/test", 323, lp::NackReason::NO_ROUTE);

  face->sendNack(nack1);

  BOOST_CHECK_EQUAL(service->getCounters().nOutNacks, 1);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet nack1pkt;
  BOOST_REQUIRE_NO_THROW(nack1pkt.wireDecode(transport->sentPackets.back().packet));
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

  shared_ptr<Interest> interest1 = makeInterest("/23Rd9hEiR");

  transport->receivePacket(interest1->wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInInterests, 1);
  BOOST_REQUIRE_EQUAL(receivedInterests.size(), 1);
  BOOST_CHECK_EQUAL(receivedInterests.back(), *interest1);
}

BOOST_AUTO_TEST_CASE(ReceiveInterest)
{
  // Initialize with Options that disables all services
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  shared_ptr<Interest> interest1 = makeInterest("/23Rd9hEiR");
  lp::Packet lpPacket;
  lpPacket.set<lp::FragmentField>(std::make_pair(
    interest1->wireEncode().begin(), interest1->wireEncode().end()));
  lpPacket.set<lp::SequenceField>(0); // force LpPacket encoding

  transport->receivePacket(lpPacket.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInInterests, 1);
  BOOST_REQUIRE_EQUAL(receivedInterests.size(), 1);
  BOOST_CHECK_EQUAL(receivedInterests.back(), *interest1);
}

BOOST_AUTO_TEST_CASE(ReceiveBareData)
{
  // Initialize with Options that disables all services
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  shared_ptr<Data> data1 = makeData("/12345678");

  transport->receivePacket(data1->wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInData, 1);
  BOOST_REQUIRE_EQUAL(receivedData.size(), 1);
  BOOST_CHECK_EQUAL(receivedData.back(), *data1);
}

BOOST_AUTO_TEST_CASE(ReceiveData)
{
  // Initialize with Options that disables all services
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  shared_ptr<Data> data1 = makeData("/12345689");
  lp::Packet lpPacket;
  lpPacket.set<lp::FragmentField>(std::make_pair(
    data1->wireEncode().begin(), data1->wireEncode().end()));
  lpPacket.set<lp::SequenceField>(0); // force LpPacket encoding

  transport->receivePacket(lpPacket.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInData, 1);
  BOOST_REQUIRE_EQUAL(receivedData.size(), 1);
  BOOST_CHECK_EQUAL(receivedData.back(), *data1);
}

BOOST_AUTO_TEST_CASE(ReceiveNack)
{
  // Initialize with Options that disables all services
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  lp::Nack nack1 = makeNack("/localhost/test", 323, lp::NackReason::NO_ROUTE);
  lp::Packet lpPacket;
  lpPacket.set<lp::FragmentField>(std::make_pair(
    nack1.getInterest().wireEncode().begin(), nack1.getInterest().wireEncode().end()));
  lpPacket.set<lp::NackField>(nack1.getHeader());

  transport->receivePacket(lpPacket.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInNacks, 1);
  BOOST_REQUIRE_EQUAL(receivedNacks.size(), 1);
  BOOST_CHECK(receivedNacks.back().getReason() == nack1.getReason());
  BOOST_CHECK(receivedNacks.back().getInterest() == nack1.getInterest());
}

BOOST_AUTO_TEST_CASE(ReceiveIdlePacket)
{
  // Initialize with Options that disables all services
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  lp::Packet lpPacket;
  lpPacket.set<lp::SequenceField>(0);

  BOOST_CHECK_NO_THROW(transport->receivePacket(lpPacket.wireEncode()));

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

  shared_ptr<Data> data = makeData("/test/data/123456789/987654321/123456789");
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

  shared_ptr<Data> data = makeData("/test/data/123456789/987654321/123456789");
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

  shared_ptr<Data> data = makeData("/test/data/123456789/987654321/123456789");
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

  shared_ptr<Data> data = makeData("/test/data/123456789/987654321/123456789");
  face->sendData(*data);

  BOOST_CHECK_GT(transport->sentPackets.size(), 1);
}

BOOST_AUTO_TEST_CASE(ReassembleFragments)
{
  // Initialize with Options that enables reassembly
  GenericLinkService::Options options;
  options.allowReassembly = true;
  initialize(options);

  shared_ptr<Interest> interest = makeInterest(
    "/mt7P130BHXmtLm5dwaY5dpUM6SWYNN2B05g7y3UhsQuLvDdnTWdNnTeEiLuW3FAbJRSG3tzQ0UfaSEgG9rvYHmsKtgPMag1Hj4Tr");
  lp::Packet packet(interest->wireEncode());

  // fragment the packet
  LpFragmenter fragmenter;
  size_t mtu = 100;
  bool isOk = false;
  std::vector<lp::Packet> frags;
  std::tie(isOk, frags) = fragmenter.fragmentPacket(packet, mtu);
  BOOST_REQUIRE(isOk);
  BOOST_CHECK_GT(frags.size(), 1);

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
      BOOST_CHECK_EQUAL(receivedInterests.back(), *interest);
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

  shared_ptr<Interest> interest = makeInterest("/IgFe6NvH");
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

  shared_ptr<Interest> interest = makeInterest("/SeGmEjvIVX");
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

  shared_ptr<Interest> interest1 = makeInterest("/localhost/test");

  face->sendInterest(*interest1);

  BOOST_CHECK_EQUAL(service->getCounters().nOutInterests, 1);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet interest1pkt;
  BOOST_REQUIRE_NO_THROW(interest1pkt.wireDecode(transport->sentPackets.back().packet));
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

  shared_ptr<Data> data1 = makeData("/localhost/test");

  face->sendData(*data1);

  BOOST_CHECK_EQUAL(service->getCounters().nOutData, 1);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet data1pkt;
  BOOST_REQUIRE_NO_THROW(data1pkt.wireDecode(transport->sentPackets.back().packet));
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

  lp::Nack nack1 = makeNack("/localhost/test", 323, lp::NackReason::NO_ROUTE);

  face->sendNack(nack1);

  BOOST_CHECK_EQUAL(service->getCounters().nOutNacks, 1);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet nack1pkt;
  BOOST_REQUIRE_NO_THROW(nack1pkt.wireDecode(transport->sentPackets.back().packet));
  BOOST_CHECK(nack1pkt.has<lp::NackField>());
  BOOST_CHECK(nack1pkt.has<lp::FragmentField>());
  BOOST_CHECK(nack1pkt.has<lp::TxSequenceField>());
}

BOOST_AUTO_TEST_SUITE_END() // Reliability

// congestion detection and marking
BOOST_AUTO_TEST_SUITE(CongestionMark)

BOOST_AUTO_TEST_CASE(NoCongestion)
{
  GenericLinkService::Options options;
  options.allowCongestionMarking = true;
  options.baseCongestionMarkingInterval = time::milliseconds(100);
  initialize(options, MTU_UNLIMITED, 65536);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, time::steady_clock::TimePoint::max());
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 0);

  shared_ptr<Interest> interest = makeInterest("/12345678");

  // congestion threshold will be 32768 bytes, since min(65536, 65536 / 2) = 32768 bytes

  // no congestion
  transport->setSendQueueLength(0);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet pkt1;
  BOOST_REQUIRE_NO_THROW(pkt1.wireDecode(transport->sentPackets.back().packet));
  BOOST_CHECK_EQUAL(pkt1.count<lp::CongestionMarkField>(), 0);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, time::steady_clock::TimePoint::max());
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 0);

  // no congestion
  transport->setSendQueueLength(32768);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 2);
  lp::Packet pkt2;
  BOOST_REQUIRE_NO_THROW(pkt2.wireDecode(transport->sentPackets.back().packet));
  BOOST_CHECK_EQUAL(pkt2.count<lp::CongestionMarkField>(), 0);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, time::steady_clock::TimePoint::max());
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 0);
}

BOOST_AUTO_TEST_CASE(CongestionCoDel)
{
  GenericLinkService::Options options;
  options.allowCongestionMarking = true;
  options.baseCongestionMarkingInterval = time::milliseconds(100);
  initialize(options, MTU_UNLIMITED, 65536);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, time::steady_clock::TimePoint::max());
  BOOST_CHECK_EQUAL(service->m_lastMarkTime, time::steady_clock::TimePoint::min());
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 0);

  shared_ptr<Interest> interest = makeInterest("/12345678");

  // congestion threshold will be 32768 bytes, since min(65536, 65536 / 2) = 32768 bytes

  // first congested packet, will be marked
  transport->setSendQueueLength(32769);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet pkt1;
  BOOST_REQUIRE_NO_THROW(pkt1.wireDecode(transport->sentPackets.back().packet));
  BOOST_REQUIRE_EQUAL(pkt1.count<lp::CongestionMarkField>(), 1);
  BOOST_CHECK_EQUAL(pkt1.get<lp::CongestionMarkField>(), 1);
  time::steady_clock::TimePoint nextMarkTime = time::steady_clock::now() + time::milliseconds(100);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);
  time::steady_clock::TimePoint lastMarkTime = time::steady_clock::now();
  BOOST_CHECK_EQUAL(service->m_lastMarkTime, lastMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 1);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 1);

  // advance clock to half of marking interval cycle
  advanceClocks(time::milliseconds(50));

  // second congested packet, but within marking interval, will not be marked
  transport->setSendQueueLength(33000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 2);
  lp::Packet pkt2;
  BOOST_REQUIRE_NO_THROW(pkt2.wireDecode(transport->sentPackets.back().packet));
  BOOST_CHECK_EQUAL(pkt2.count<lp::CongestionMarkField>(), 0);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);
  BOOST_CHECK_EQUAL(service->m_lastMarkTime, lastMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 1);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 1);

  // advance clocks past end of initial interval cycle
  this->advanceClocks(time::milliseconds(51));

  // first congested packet after waiting marking interval, will be marked
  transport->setSendQueueLength(40000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 3);
  lp::Packet pkt3;
  BOOST_REQUIRE_NO_THROW(pkt3.wireDecode(transport->sentPackets.back().packet));
  BOOST_REQUIRE_EQUAL(pkt3.count<lp::CongestionMarkField>(), 1);
  BOOST_CHECK_EQUAL(pkt3.get<lp::CongestionMarkField>(), 1);
  time::nanoseconds markingInterval(
    static_cast<time::nanoseconds::rep>(options.baseCongestionMarkingInterval.count() /
                                        std::sqrt(service->m_nMarkedSinceInMarkingState)));
  nextMarkTime += markingInterval;
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);
  lastMarkTime = time::steady_clock::now();
  BOOST_CHECK_EQUAL(service->m_lastMarkTime, lastMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 2);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 2);

  // advance clock partway through current marking interval
  this->advanceClocks(markingInterval - time::milliseconds(10));

  // still congested, but within marking interval cycle
  transport->setSendQueueLength(38000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 4);
  lp::Packet pkt4;
  BOOST_REQUIRE_NO_THROW(pkt4.wireDecode(transport->sentPackets.back().packet));
  BOOST_CHECK_EQUAL(pkt4.count<lp::CongestionMarkField>(), 0);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);
  BOOST_CHECK_EQUAL(service->m_lastMarkTime, lastMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 2);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 2);

  // advance clocks past end of current marking interval cycle
  this->advanceClocks(time::milliseconds(11));

  // still congested, after marking interval cycle
  transport->setSendQueueLength(39000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 5);
  lp::Packet pkt5;
  BOOST_REQUIRE_NO_THROW(pkt5.wireDecode(transport->sentPackets.back().packet));
  BOOST_REQUIRE_EQUAL(pkt5.count<lp::CongestionMarkField>(), 1);
  BOOST_CHECK_EQUAL(pkt5.get<lp::CongestionMarkField>(), 1);
  markingInterval = time::nanoseconds(
    static_cast<time::nanoseconds::rep>(options.baseCongestionMarkingInterval.count() /
                                        std::sqrt(service->m_nMarkedSinceInMarkingState)));
  nextMarkTime += markingInterval;
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);
  lastMarkTime = time::steady_clock::now();
  BOOST_CHECK_EQUAL(service->m_lastMarkTime, lastMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 3);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 3);

  this->advanceClocks(time::milliseconds(1));

  // still congested, but within marking interval cycle
  transport->setSendQueueLength(38000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 6);
  lp::Packet pkt6;
  BOOST_REQUIRE_NO_THROW(pkt6.wireDecode(transport->sentPackets.back().packet));
  BOOST_CHECK_EQUAL(pkt6.count<lp::CongestionMarkField>(), 0);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);
  BOOST_CHECK_EQUAL(service->m_lastMarkTime, lastMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 3);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 3);

  this->advanceClocks(markingInterval);

  // still congested, after marking interval cycle
  transport->setSendQueueLength(34000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 7);
  lp::Packet pkt7;
  BOOST_REQUIRE_NO_THROW(pkt7.wireDecode(transport->sentPackets.back().packet));
  BOOST_REQUIRE_EQUAL(pkt7.count<lp::CongestionMarkField>(), 1);
  BOOST_CHECK_EQUAL(pkt7.get<lp::CongestionMarkField>(), 1);
  markingInterval = time::nanoseconds(
    static_cast<time::nanoseconds::rep>(options.baseCongestionMarkingInterval.count() /
                                        std::sqrt(service->m_nMarkedSinceInMarkingState)));
  nextMarkTime += markingInterval;
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);
  lastMarkTime = time::steady_clock::now();
  BOOST_CHECK_EQUAL(service->m_lastMarkTime, lastMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 4);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 4);

  // no more congestion
  transport->setSendQueueLength(30000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 8);
  lp::Packet pkt8;
  BOOST_REQUIRE_NO_THROW(pkt8.wireDecode(transport->sentPackets.back().packet));
  BOOST_CHECK_EQUAL(pkt8.count<lp::CongestionMarkField>(), 0);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, time::steady_clock::TimePoint::max());
  BOOST_CHECK_EQUAL(service->m_lastMarkTime, lastMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 4);

  this->advanceClocks(time::milliseconds(50));

  // send queue congested again, but can't mark packet because within one full interval of last mark
  transport->setSendQueueLength(50000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 9);
  lp::Packet pkt9;
  BOOST_REQUIRE_NO_THROW(pkt9.wireDecode(transport->sentPackets.back().packet));
  BOOST_CHECK_EQUAL(pkt9.count<lp::CongestionMarkField>(), 0);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, time::steady_clock::TimePoint::max());
  BOOST_CHECK_EQUAL(service->m_lastMarkTime, lastMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 4);

  // advance clock past full 100ms interval since last mark
  this->advanceClocks(time::milliseconds(51));

  transport->setSendQueueLength(40000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 10);
  lp::Packet pkt10;
  BOOST_REQUIRE_NO_THROW(pkt10.wireDecode(transport->sentPackets.back().packet));
  BOOST_REQUIRE_EQUAL(pkt10.count<lp::CongestionMarkField>(), 1);
  BOOST_CHECK_EQUAL(pkt10.get<lp::CongestionMarkField>(), 1);
  nextMarkTime = time::steady_clock::now() + time::milliseconds(100);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);
  lastMarkTime = time::steady_clock::now();
  BOOST_CHECK_EQUAL(service->m_lastMarkTime, lastMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 1);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 5);

  // advance clock partway through 100ms marking interval
  this->advanceClocks(time::milliseconds(50));

  // not marked since within 100ms window before can mark again
  transport->setSendQueueLength(50000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 11);
  lp::Packet pkt11;
  BOOST_REQUIRE_NO_THROW(pkt11.wireDecode(transport->sentPackets.back().packet));
  BOOST_CHECK_EQUAL(pkt11.count<lp::CongestionMarkField>(), 0);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);
  BOOST_CHECK_EQUAL(service->m_lastMarkTime, lastMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 1);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 5);

  // advance clocks past m_nextMarkTime
  this->advanceClocks(time::milliseconds(51));

  // markable packet, queue length still above threshold
  transport->setSendQueueLength(33000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 12);
  lp::Packet pkt12;
  BOOST_REQUIRE_NO_THROW(pkt12.wireDecode(transport->sentPackets.back().packet));
  BOOST_REQUIRE_EQUAL(pkt12.count<lp::CongestionMarkField>(), 1);
  BOOST_CHECK_EQUAL(pkt12.get<lp::CongestionMarkField>(), 1);
  markingInterval = time::nanoseconds(
    static_cast<time::nanoseconds::rep>(options.baseCongestionMarkingInterval.count() /
                                        std::sqrt(service->m_nMarkedSinceInMarkingState)));
  nextMarkTime += markingInterval;
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);
  lastMarkTime = time::steady_clock::now();
  BOOST_CHECK_EQUAL(service->m_lastMarkTime, lastMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 2);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 6);

  // no more congestion
  transport->setSendQueueLength(10000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 13);
  lp::Packet pkt13;
  BOOST_REQUIRE_NO_THROW(pkt13.wireDecode(transport->sentPackets.back().packet));
  BOOST_CHECK_EQUAL(pkt13.count<lp::CongestionMarkField>(), 0);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, time::steady_clock::TimePoint::max());
  BOOST_CHECK_EQUAL(service->m_lastMarkTime, lastMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 6);

  // advance clocks past one full interval since last mark
  this->advanceClocks(time::milliseconds(101));

  // start congestion again
  transport->setSendQueueLength(50000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 14);
  lp::Packet pkt14;
  BOOST_REQUIRE_NO_THROW(pkt14.wireDecode(transport->sentPackets.back().packet));
  BOOST_REQUIRE_EQUAL(pkt14.count<lp::CongestionMarkField>(), 1);
  BOOST_CHECK_EQUAL(pkt14.get<lp::CongestionMarkField>(), 1);
  nextMarkTime = time::steady_clock::now() + time::milliseconds(100);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);
  lastMarkTime = time::steady_clock::now();
  BOOST_CHECK_EQUAL(service->m_lastMarkTime, lastMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 1);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 7);

  // no more congestion, cancel marking interval
  transport->setSendQueueLength(5000);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 15);
  lp::Packet pkt15;
  BOOST_REQUIRE_NO_THROW(pkt15.wireDecode(transport->sentPackets.back().packet));
  BOOST_CHECK_EQUAL(pkt15.count<lp::CongestionMarkField>(), 0);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, time::steady_clock::TimePoint::max());
  BOOST_CHECK_EQUAL(service->m_lastMarkTime, lastMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 7);
}

BOOST_AUTO_TEST_CASE(DefaultThreshold)
{
  GenericLinkService::Options options;
  options.allowCongestionMarking = true;
  options.baseCongestionMarkingInterval = time::milliseconds(100);
  initialize(options, MTU_UNLIMITED, QUEUE_UNSUPPORTED);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, time::steady_clock::TimePoint::max());
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 0);

  shared_ptr<Interest> interest = makeInterest("/12345678");

  // congestion threshold will be 65536 bytes, since the transport reports that it cannot measure
  // the queue capacity

  // no congestion
  transport->setSendQueueLength(0);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet pkt1;
  BOOST_REQUIRE_NO_THROW(pkt1.wireDecode(transport->sentPackets.back().packet));
  BOOST_CHECK_EQUAL(pkt1.count<lp::CongestionMarkField>(), 0);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, time::steady_clock::TimePoint::max());
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 0);

  // no congestion
  transport->setSendQueueLength(65536);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 2);
  lp::Packet pkt2;
  BOOST_REQUIRE_NO_THROW(pkt2.wireDecode(transport->sentPackets.back().packet));
  BOOST_CHECK_EQUAL(pkt2.count<lp::CongestionMarkField>(), 0);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, time::steady_clock::TimePoint::max());
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 0);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 0);

  // first congested (and marked) packet
  transport->setSendQueueLength(65537);
  face->sendInterest(*interest);
  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 3);
  lp::Packet pkt3;
  BOOST_REQUIRE_NO_THROW(pkt3.wireDecode(transport->sentPackets.back().packet));
  BOOST_REQUIRE_EQUAL(pkt3.count<lp::CongestionMarkField>(), 1);
  BOOST_CHECK_EQUAL(pkt3.get<lp::CongestionMarkField>(), 1);
  time::steady_clock::TimePoint nextMarkTime = time::steady_clock::now() + time::milliseconds(100);
  BOOST_CHECK_EQUAL(service->m_nextMarkTime, nextMarkTime);
  BOOST_CHECK_EQUAL(service->m_nMarkedSinceInMarkingState, 1);
  BOOST_CHECK_EQUAL(service->getCounters().nCongestionMarked, 1);
}

BOOST_AUTO_TEST_SUITE_END() // CongestionMark

BOOST_AUTO_TEST_SUITE(LpFields)

BOOST_AUTO_TEST_CASE(ReceiveNextHopFaceId)
{
  // Initialize with Options that enables local fields
  GenericLinkService::Options options;
  options.allowLocalFields = true;
  initialize(options);

  shared_ptr<Interest> interest = makeInterest("/12345678");
  lp::Packet packet(interest->wireEncode());
  packet.set<lp::NextHopFaceIdField>(1000);

  transport->receivePacket(packet.wireEncode());

  BOOST_REQUIRE_EQUAL(receivedInterests.size(), 1);
  shared_ptr<lp::NextHopFaceIdTag> tag = receivedInterests.back().getTag<lp::NextHopFaceIdTag>();
  BOOST_REQUIRE(tag != nullptr);
  BOOST_CHECK_EQUAL(*tag, 1000);
}

BOOST_AUTO_TEST_CASE(ReceiveNextHopFaceIdDisabled)
{
  // Initialize with Options that disables local fields
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  shared_ptr<Interest> interest = makeInterest("/12345678");
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

  shared_ptr<Data> data = makeData("/12345678");
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

  lp::Nack nack = makeNack("/localhost/test", 123, lp::NackReason::NO_ROUTE);
  lp::Packet packet;
  packet.set<lp::FragmentField>(std::make_pair(
    nack.getInterest().wireEncode().begin(), nack.getInterest().wireEncode().end()));
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

  shared_ptr<Data> data = makeData("/12345678");
  lp::Packet packet(data->wireEncode());
  packet.set<lp::CachePolicyField>(lp::CachePolicy().setPolicy(lp::CachePolicyType::NO_CACHE));

  transport->receivePacket(packet.wireEncode());

  BOOST_REQUIRE_EQUAL(receivedData.size(), 1);
  shared_ptr<lp::CachePolicyTag> tag = receivedData.back().getTag<lp::CachePolicyTag>();
  BOOST_REQUIRE(tag != nullptr);
  BOOST_CHECK_EQUAL(tag->get().getPolicy(), lp::CachePolicyType::NO_CACHE);
}

BOOST_AUTO_TEST_CASE(ReceiveCachePolicyDropInterest)
{
  // Initialize with Options that enables local fields
  GenericLinkService::Options options;
  options.allowLocalFields = true;
  initialize(options);

  shared_ptr<Interest> interest = makeInterest("/12345678");
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

  lp::Nack nack = makeNack("/localhost/test", 123, lp::NackReason::NO_ROUTE);
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

  shared_ptr<Interest> interest = makeInterest("/12345678");
  interest->setTag(make_shared<lp::IncomingFaceIdTag>(1000));

  face->sendInterest(*interest);

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet sent(transport->sentPackets.back().packet);
  BOOST_REQUIRE(sent.has<lp::IncomingFaceIdField>());
  BOOST_CHECK_EQUAL(sent.get<lp::IncomingFaceIdField>(), 1000);
}

BOOST_AUTO_TEST_CASE(SendIncomingFaceIdDisabled)
{
  // Initialize with Options that disables local fields
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  shared_ptr<Interest> interest = makeInterest("/12345678");
  interest->setTag(make_shared<lp::IncomingFaceIdTag>(1000));

  face->sendInterest(*interest);

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet sent(transport->sentPackets.back().packet);
  BOOST_CHECK(!sent.has<lp::IncomingFaceIdField>());
}

BOOST_AUTO_TEST_CASE(ReceiveIncomingFaceIdIgnoreInterest)
{
  // Initialize with Options that enables local fields
  GenericLinkService::Options options;
  options.allowLocalFields = true;
  initialize(options);

  shared_ptr<Interest> interest = makeInterest("/12345678");
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

  shared_ptr<Data> data = makeData("/z1megUh9Bj");
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

  lp::Nack nack = makeNack("/TPAhdiHz", 278, lp::NackReason::CONGESTION);
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
  shared_ptr<Interest> interest = makeInterest("/12345678");
  interest->setTag(make_shared<lp::CongestionMarkTag>(1));

  face->sendInterest(*interest);

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet sent(transport->sentPackets.back().packet);
  BOOST_REQUIRE(sent.has<lp::CongestionMarkField>());
  BOOST_CHECK_EQUAL(sent.get<lp::CongestionMarkField>(), 1);
}

BOOST_AUTO_TEST_CASE(SendCongestionMarkData)
{
  shared_ptr<Data> data = makeData("/12345678");
  data->setTag(make_shared<lp::CongestionMarkTag>(0));

  face->sendData(*data);

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet sent(transport->sentPackets.back().packet);
  BOOST_REQUIRE(sent.has<lp::CongestionMarkField>());
  BOOST_CHECK_EQUAL(sent.get<lp::CongestionMarkField>(), 0);
}

BOOST_AUTO_TEST_CASE(SendCongestionMarkNack)
{
  lp::Nack nack = makeNack("/localhost/test", 123, lp::NackReason::NO_ROUTE);
  nack.setTag(make_shared<lp::CongestionMarkTag>(std::numeric_limits<uint64_t>::max()));

  face->sendNack(nack);

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet sent(transport->sentPackets.back().packet);
  BOOST_REQUIRE(sent.has<lp::CongestionMarkField>());
  BOOST_CHECK_EQUAL(sent.get<lp::CongestionMarkField>(), std::numeric_limits<uint64_t>::max());
}

BOOST_AUTO_TEST_CASE(ReceiveCongestionMarkInterest)
{
  shared_ptr<Interest> interest = makeInterest("/12345678");
  lp::Packet packet(interest->wireEncode());
  packet.set<lp::CongestionMarkField>(1);

  transport->receivePacket(packet.wireEncode());

  BOOST_REQUIRE_EQUAL(receivedInterests.size(), 1);
  shared_ptr<lp::CongestionMarkTag> tag = receivedInterests.back().getTag<lp::CongestionMarkTag>();
  BOOST_REQUIRE(tag != nullptr);
  BOOST_CHECK_EQUAL(*tag, 1);
}

BOOST_AUTO_TEST_CASE(ReceiveCongestionMarkData)
{
  shared_ptr<Data> data = makeData("/12345678");
  lp::Packet packet(data->wireEncode());
  packet.set<lp::CongestionMarkField>(1);

  transport->receivePacket(packet.wireEncode());

  BOOST_REQUIRE_EQUAL(receivedData.size(), 1);
  shared_ptr<lp::CongestionMarkTag> tag = receivedData.back().getTag<lp::CongestionMarkTag>();
  BOOST_REQUIRE(tag != nullptr);
  BOOST_CHECK_EQUAL(*tag, 1);
}

BOOST_AUTO_TEST_CASE(ReceiveCongestionMarkNack)
{
  lp::Nack nack = makeNack("/localhost/test", 123, lp::NackReason::NO_ROUTE);
  lp::Packet packet;
  packet.set<lp::FragmentField>(std::make_pair(
    nack.getInterest().wireEncode().begin(), nack.getInterest().wireEncode().end()));
  packet.set<lp::NackField>(nack.getHeader());
  packet.set<lp::CongestionMarkField>(1);

  transport->receivePacket(packet.wireEncode());

  BOOST_REQUIRE_EQUAL(receivedNacks.size(), 1);
  shared_ptr<lp::CongestionMarkTag> tag = receivedNacks.back().getTag<lp::CongestionMarkTag>();
  BOOST_REQUIRE(tag != nullptr);
  BOOST_CHECK_EQUAL(*tag, 1);
}

BOOST_AUTO_TEST_CASE(SendNonDiscovery)
{
  GenericLinkService::Options options;
  options.allowSelfLearning = true;
  initialize(options);

  shared_ptr<Interest> interest = makeInterest("/12345678");
  interest->setTag(make_shared<lp::NonDiscoveryTag>(lp::EmptyValue{}));

  face->sendInterest(*interest);

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet sent(transport->sentPackets.back().packet);
  BOOST_CHECK(sent.has<lp::NonDiscoveryField>());
}

BOOST_AUTO_TEST_CASE(SendNonDiscoveryDisabled)
{
  GenericLinkService::Options options;
  options.allowSelfLearning = false;
  initialize(options);

  shared_ptr<Interest> interest = makeInterest("/12345678");
  interest->setTag(make_shared<lp::NonDiscoveryTag>(lp::EmptyValue{}));

  face->sendInterest(*interest);

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet sent(transport->sentPackets.back().packet);
  BOOST_CHECK(!sent.has<lp::NonDiscoveryField>());
}

BOOST_AUTO_TEST_CASE(ReceiveNonDiscovery)
{
  GenericLinkService::Options options;
  options.allowSelfLearning = true;
  initialize(options);

  shared_ptr<Interest> interest = makeInterest("/12345678");
  lp::Packet packet(interest->wireEncode());
  packet.set<lp::NonDiscoveryField>(lp::EmptyValue{});

  transport->receivePacket(packet.wireEncode());

  BOOST_REQUIRE_EQUAL(receivedInterests.size(), 1);
  shared_ptr<lp::NonDiscoveryTag> tag = receivedInterests.back().getTag<lp::NonDiscoveryTag>();
  BOOST_CHECK(tag != nullptr);
}

BOOST_AUTO_TEST_CASE(ReceiveNonDiscoveryDisabled)
{
  GenericLinkService::Options options;
  options.allowSelfLearning = false;
  initialize(options);

  shared_ptr<Interest> interest = makeInterest("/12345678");
  lp::Packet packet(interest->wireEncode());
  packet.set<lp::NonDiscoveryField>(lp::EmptyValue{});

  transport->receivePacket(packet.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInNetInvalid, 0); // not an error
  BOOST_CHECK_EQUAL(receivedInterests.size(), 1);

  shared_ptr<lp::NonDiscoveryTag> tag = receivedInterests.back().getTag<lp::NonDiscoveryTag>();
  BOOST_CHECK(tag == nullptr);
}

BOOST_AUTO_TEST_CASE(ReceiveNonDiscoveryDropData)
{
  GenericLinkService::Options options;
  options.allowSelfLearning = true;
  initialize(options);

  shared_ptr<Data> data = makeData("/12345678");
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

  lp::Nack nack = makeNack("/localhost/test", 123, lp::NackReason::NO_ROUTE);
  lp::Packet packet;
  packet.set<lp::FragmentField>(std::make_pair(
    nack.getInterest().wireEncode().begin(), nack.getInterest().wireEncode().end()));
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

  shared_ptr<Data> data = makeData("/12345678");
  lp::PrefixAnnouncement pa = makePrefixAnnouncement("/local/ndn/prefix");
  data->setTag(make_shared<lp::PrefixAnnouncementTag>(pa));

  face->sendData(*data);

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet sent(transport->sentPackets.back().packet);
  BOOST_CHECK(sent.has<lp::PrefixAnnouncementField>());
}

BOOST_AUTO_TEST_CASE(SendPrefixAnnouncementDisabled)
{
  GenericLinkService::Options options;
  options.allowSelfLearning = false;
  initialize(options);

  shared_ptr<Data> data = makeData("/12345678");
  lp::PrefixAnnouncement pa = makePrefixAnnouncement("/local/ndn/prefix");
  data->setTag(make_shared<lp::PrefixAnnouncementTag>(pa));

  face->sendData(*data);

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet sent(transport->sentPackets.back().packet);
  BOOST_CHECK(!sent.has<lp::PrefixAnnouncementField>());
}

BOOST_AUTO_TEST_CASE(ReceivePrefixAnnouncement)
{
  GenericLinkService::Options options;
  options.allowSelfLearning = true;
  initialize(options);

  shared_ptr<Data> data = makeData("/12345678");
  lp::PrefixAnnouncement pa = makePrefixAnnouncement("/local/ndn/prefix");
  lp::Packet packet(data->wireEncode());
  packet.set<lp::PrefixAnnouncementField>(pa);

  transport->receivePacket(packet.wireEncode());

  BOOST_REQUIRE_EQUAL(receivedData.size(), 1);
  shared_ptr<lp::PrefixAnnouncementTag> tag = receivedData.back().getTag<lp::PrefixAnnouncementTag>();
  BOOST_REQUIRE_EQUAL(tag->get().getAnnouncedName(), "/local/ndn/prefix");
}

BOOST_AUTO_TEST_CASE(ReceivePrefixAnnouncementDisabled)
{
  GenericLinkService::Options options;
  options.allowSelfLearning = false;
  initialize(options);

  shared_ptr<Data> data = makeData("/12345678");
  lp::PrefixAnnouncement pa = makePrefixAnnouncement("/local/ndn/prefix");
  lp::Packet packet(data->wireEncode());
  packet.set<lp::PrefixAnnouncementField>(pa);

  transport->receivePacket(packet.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInNetInvalid, 0); // not an error
  BOOST_CHECK_EQUAL(receivedData.size(), 1);

  shared_ptr<lp::NonDiscoveryTag> tag = receivedData.back().getTag<lp::NonDiscoveryTag>();
  BOOST_CHECK(tag == nullptr);
}

BOOST_AUTO_TEST_CASE(ReceivePrefixAnnouncementDropInterest)
{
  GenericLinkService::Options options;
  options.allowSelfLearning = true;
  initialize(options);

  shared_ptr<Interest> interest = makeInterest("/12345678");
  lp::PrefixAnnouncement pa = makePrefixAnnouncement("/local/ndn/prefix");
  lp::Packet packet(interest->wireEncode());
  packet.set<lp::PrefixAnnouncementField>(pa);

  transport->receivePacket(packet.wireEncode());

  BOOST_CHECK_EQUAL(service->getCounters().nInNetInvalid, 1);
  BOOST_CHECK(receivedInterests.empty());
}

BOOST_AUTO_TEST_CASE(ReceivePrefixAnnouncementDropNack)
{
  GenericLinkService::Options options;
  options.allowSelfLearning = true;
  initialize(options);

  lp::Nack nack = makeNack("/localhost/test", 123, lp::NackReason::NO_ROUTE);
  lp::PrefixAnnouncement pa = makePrefixAnnouncement("/local/ndn/prefix");
  lp::Packet packet;
  packet.set<lp::FragmentField>(std::make_pair(
    nack.getInterest().wireEncode().begin(), nack.getInterest().wireEncode().end()));
  packet.set<lp::NackField>(nack.getHeader());
  packet.set<lp::PrefixAnnouncementField>(pa);

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

  Block packet = ndn::encoding::makeEmptyBlock(tlv::Name);

  BOOST_CHECK_NO_THROW(transport->receivePacket(packet));

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

  Block packet = ndn::encoding::makeStringBlock(lp::tlv::LpPacket, "x");
  BOOST_CHECK_THROW(packet.parse(), tlv::Error);

  BOOST_CHECK_NO_THROW(transport->receivePacket(packet));

  BOOST_CHECK_EQUAL(service->getCounters().nInLpInvalid, 1);
  BOOST_CHECK_EQUAL(receivedInterests.size(), 0);
  BOOST_CHECK_EQUAL(receivedData.size(), 0);
  BOOST_CHECK_EQUAL(receivedNacks.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // Malformed


BOOST_AUTO_TEST_SUITE_END() // TestGenericLinkService
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
