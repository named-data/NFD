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
    this->initialize();
    // By default, GenericLinkService is created with default options.
    // Test cases may invoke .initialize with alternate options.
  }

  void
  initialize()
  {
    // TODO#3104 add GenericLinkService::Options parameter,
    //           and create GenericLinkService with options
    face.reset(new LpFace(make_unique<GenericLinkService>(),
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
  // TODO#3104 initialize with Options that disables all services

  shared_ptr<Interest> interest1 = makeInterest("/localhost/test");

  face->sendInterest(*interest1);

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  BOOST_CHECK(transport->sentPackets.back().packet == interest1->wireEncode());
}

BOOST_AUTO_TEST_CASE(SendData)
{
  // TODO#3104 initialize with Options that disables all services

  shared_ptr<Data> data1 = makeData("/localhost/test");

  face->sendData(*data1);

  BOOST_REQUIRE_EQUAL(transport->sentPackets.size(), 1);
  BOOST_CHECK(transport->sentPackets.back().packet == data1->wireEncode());
}

BOOST_AUTO_TEST_CASE(SendNack)
{
  // TODO#3104 initialize with Options that disables all services

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
  // TODO#3104 initialize with Options that disables all services

  shared_ptr<Interest> interest1 = makeInterest("/23Rd9hEiR");

  transport->receivePacket(interest1->wireEncode());

  BOOST_REQUIRE_EQUAL(receivedInterests.size(), 1);
  BOOST_CHECK_EQUAL(receivedInterests.back(), *interest1);
}

BOOST_AUTO_TEST_CASE(ReceiveInterest)
{
  // TODO#3104 initialize with Options that disables all services

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
  // TODO#3104 initialize with Options that disables all services

  shared_ptr<Data> data1 = makeData("/12345678");

  transport->receivePacket(data1->wireEncode());

  BOOST_REQUIRE_EQUAL(receivedData.size(), 1);
  BOOST_CHECK_EQUAL(receivedData.back(), *data1);
}

BOOST_AUTO_TEST_CASE(ReceiveData)
{
  // TODO#3104 initialize with Options that disables all services

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
  // TODO#3104 initialize with Options that disables all services

  lp::Nack nack1 = makeNack("/localhost/test", 323, lp::NackReason::NO_ROUTE);
  lp::Packet lpPacket;
  lpPacket.set<lp::FragmentField>(std::make_pair(
    nack1.getInterest().wireEncode().begin(), nack1.getInterest().wireEncode().end()));
  lpPacket.set<lp::NackField>(nack1.getHeader());

  transport->receivePacket(lpPacket.wireEncode());

  BOOST_REQUIRE_EQUAL(receivedNacks.size(), 1);
}

BOOST_AUTO_TEST_SUITE_END() // SimpleSendReceive


BOOST_AUTO_TEST_SUITE(LocalFields)

BOOST_AUTO_TEST_CASE(SendIncomingFaceId)
{
  // TODO#3104 initialize with Options that enables local fields
  // TODO#3104 send Interest with IncomingFaceId
  //           expect transport->sentPackets.back() has IncomingFaceId field
}

BOOST_AUTO_TEST_CASE(SendIncomingFaceIdDisabled)
{
  // TODO#3104 initialize with Options that disables local fields
  // TODO#3104 send Interest with IncomingFaceId
  //           expect transport->sentPackets.back() has no IncomingFaceId field
}

BOOST_AUTO_TEST_CASE(ReceiveIncomingFaceIdIgnore)
{
  // TODO#3104 initialize with Options that enables local fields
  // TODO#3104 receive Interest with IncomingFaceId
  //           expect receivedInterests.back() has no IncomingFaceId tag
}

BOOST_AUTO_TEST_CASE(ReceiveNextHopFaceId)
{
  // TODO#3104 initialize with Options that enables local fields
  // TODO#3104 receive Interest with NextHopFaceId
  //           expect receivedInterests.back() has NextHopFaceId tag
}

BOOST_AUTO_TEST_CASE(ReceiveNextHopFaceIdDisabled)
{
  // TODO#3104 initialize with Options that disables local fields
  // TODO#3104 receive Interest with NextHopFaceId
  //           expect receivedInterests.empty()
  //       or, expect receivedInterests.back() has no NextHopFaceId tag
}

BOOST_AUTO_TEST_CASE(ReceiveNextHopFaceIdDropData)
{
  // TODO#3104 initialize with Options that enables local fields
  // TODO#3104 receive Data with NextHopFaceId
  //           expect receivedData.empty()
}

BOOST_AUTO_TEST_CASE(ReceiveNextHopFaceIdDropNack)
{
  // TODO#3104 initialize with Options that enables local fields
  // TODO#3104 receive Nack with NextHopFaceId
  //           expect receivedNacks.empty()
}

BOOST_AUTO_TEST_CASE(ReceiveCacheControl)
{
  // TODO#3104 initialize with Options that enables local fields
  // TODO#3104 receive Data with CacheControl
  //           expect receivedData.back() has CacheControl tag
}

BOOST_AUTO_TEST_CASE(ReceiveCacheControlDisabled)
{
  // TODO#3104 initialize with Options that disables local fields
  // TODO#3104 receive Data with CacheControl
  //           expect receivedData.back() has no CacheControl tag
}

BOOST_AUTO_TEST_CASE(ReceiveCacheControlDropInterest)
{
  // TODO#3104 initialize with Options that enables local fields
  // TODO#3104 receive Interest with CacheControl
  //           expect receivedInterests.empty()
}

BOOST_AUTO_TEST_CASE(ReceiveCacheControlDropNack)
{
  // TODO#3104 initialize with Options that enables local fields
  // TODO#3104 receive Nack with CacheControl
  //           expect receivedNacks.empty()
}

BOOST_AUTO_TEST_SUITE_END() // LocalFields


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace face
} // namespace nfd
