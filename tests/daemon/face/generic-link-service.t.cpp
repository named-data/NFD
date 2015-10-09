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

#include "face/generic-link-service.hpp"
#include "face/lp-face.hpp"
#include "dummy-transport.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace face {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Face)

class GenericLinkServiceFixture : public BaseFixture
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
  initialize(const GenericLinkService::Options& options)
  {
    face.reset(new LpFace(make_unique<GenericLinkService>(options),
                          make_unique<DummyTransport>()));
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
  unique_ptr<LpFace> face;
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

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  BOOST_CHECK(transport->sentPackets.back().packet == interest1->wireEncode());
}

BOOST_AUTO_TEST_CASE(SendData)
{
  // Initialize with Options that disables all services
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  shared_ptr<Data> data1 = makeData("/localhost/test");

  face->sendData(*data1);

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  BOOST_CHECK(transport->sentPackets.back().packet == data1->wireEncode());
}

BOOST_AUTO_TEST_CASE(SendNack)
{
  // Initialize with Options that disables all services
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  lp::Nack nack1 = makeNack("/localhost/test", 323, lp::NackReason::NO_ROUTE);

  face->sendNack(nack1);

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  lp::Packet nack1pkt;
  BOOST_REQUIRE_NO_THROW(nack1pkt.wireDecode(transport->sentPackets.back().packet));
  BOOST_CHECK_EQUAL(nack1pkt.has<lp::NackField>(), true);
  BOOST_CHECK_EQUAL(nack1pkt.has<lp::FragmentField>(), true);
}

BOOST_AUTO_TEST_CASE(ReceiveBareInterest)
{
  // Initialize with Options that disables all services
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  shared_ptr<Interest> interest1 = makeInterest("/23Rd9hEiR");

  transport->receivePacket(interest1->wireEncode());

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

  BOOST_REQUIRE_EQUAL(receivedNacks.size(), 1);
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

  // IDLE packet should be ignored
  BOOST_CHECK_EQUAL(receivedInterests.size(), 0);
  BOOST_CHECK_EQUAL(receivedData.size(), 0);
  BOOST_CHECK_EQUAL(receivedNacks.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // SimpleSendReceive


BOOST_AUTO_TEST_SUITE(Fragmentation)

BOOST_AUTO_TEST_CASE(ReassemblyDisabledDropFragIndex)
{
  // TODO#3171 Initialize with Options that disables reassembly

  shared_ptr<Interest> interest = makeInterest("/IgFe6NvH");
  lp::Packet packet(interest->wireEncode());
  packet.set<lp::FragIndexField>(140);

  transport->receivePacket(packet.wireEncode());

  BOOST_CHECK(receivedInterests.empty());
}

BOOST_AUTO_TEST_CASE(ReassemblyDisabledDropFragCount)
{
  // TODO#3171 Initialize with Options that disables reassembly

  shared_ptr<Interest> interest = makeInterest("/SeGmEjvIVX");
  lp::Packet packet(interest->wireEncode());
  packet.set<lp::FragCountField>(276);

  transport->receivePacket(packet.wireEncode());

  BOOST_CHECK(receivedInterests.empty());
}

BOOST_AUTO_TEST_SUITE_END() // Fragmentation


BOOST_AUTO_TEST_SUITE(LocalFields)

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
  BOOST_REQUIRE(receivedInterests.back().getLocalControlHeader().hasNextHopFaceId());
  BOOST_CHECK_EQUAL(receivedInterests.back().getNextHopFaceId(), 1000);
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

  BOOST_CHECK(receivedNacks.empty());
}

BOOST_AUTO_TEST_CASE(ReceiveCacheControl)
{
  // Initialize with Options that enables local fields
  GenericLinkService::Options options;
  options.allowLocalFields = true;
  initialize(options);

  shared_ptr<Data> data = makeData("/12345678");
  lp::Packet packet(data->wireEncode());
  lp::CachePolicy policy;
  policy.setPolicy(lp::CachePolicyType::NO_CACHE);
  packet.set<lp::CachePolicyField>(policy);

  transport->receivePacket(packet.wireEncode());

  BOOST_REQUIRE_EQUAL(receivedData.size(), 1);
  BOOST_REQUIRE(receivedData.back().getLocalControlHeader().hasCachingPolicy());
  BOOST_CHECK_EQUAL(receivedData.back().getCachingPolicy(),
                    ndn::nfd::LocalControlHeader::CachingPolicy::NO_CACHE);
}

BOOST_AUTO_TEST_CASE(ReceiveCacheControlDisabled)
{
  // Initialize with Options that disables local fields
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  shared_ptr<Data> data = makeData("/12345678");
  lp::Packet packet(data->wireEncode());
  lp::CachePolicy policy;
  policy.setPolicy(lp::CachePolicyType::NO_CACHE);
  packet.set<lp::CachePolicyField>(policy);

  transport->receivePacket(packet.wireEncode());

  BOOST_REQUIRE_EQUAL(receivedData.size(), 1);
  BOOST_CHECK(!receivedData.back().getLocalControlHeader().hasCachingPolicy());
}

BOOST_AUTO_TEST_CASE(ReceiveCacheControlDropInterest)
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

  BOOST_CHECK(receivedInterests.empty());
}

BOOST_AUTO_TEST_CASE(ReceiveCacheControlDropNack)
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

  BOOST_CHECK(receivedNacks.empty());
}

BOOST_AUTO_TEST_CASE(SendIncomingFaceId)
{
  // Initialize with Options that enables local fields
  GenericLinkService::Options options;
  options.allowLocalFields = true;
  initialize(options);

  shared_ptr<Interest> interest = makeInterest("/12345678");
  interest->setIncomingFaceId(1000);

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
  interest->setIncomingFaceId(1000);

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

  BOOST_REQUIRE_EQUAL(receivedInterests.size(), 1);
  BOOST_CHECK(!receivedInterests.back().getLocalControlHeader().hasIncomingFaceId());
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

  BOOST_REQUIRE_EQUAL(receivedData.size(), 1);
  BOOST_CHECK(!receivedData.back().getLocalControlHeader().hasIncomingFaceId());
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

  BOOST_REQUIRE_EQUAL(receivedNacks.size(), 1);
  BOOST_CHECK(!receivedNacks.back().getLocalControlHeader().hasIncomingFaceId());
}

BOOST_AUTO_TEST_SUITE_END() // LocalFields


BOOST_AUTO_TEST_SUITE(Malformed) // receive malformed packets

BOOST_AUTO_TEST_CASE(WrongTlvType)
{
  // Initialize with Options that disables all services
  GenericLinkService::Options options;
  options.allowLocalFields = false;
  initialize(options);

  Block packet = ndn::encoding::makeEmptyBlock(tlv::Name);

  BOOST_CHECK_NO_THROW(transport->receivePacket(packet));

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
