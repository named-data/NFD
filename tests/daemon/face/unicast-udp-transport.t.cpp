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

#include "transport-test-common.hpp"

#include "unicast-udp-transport-fixture.hpp"

#include <boost/mpl/vector.hpp>
#include <boost/mpl/vector_c.hpp>

namespace nfd {
namespace face {
namespace tests {

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestUnicastUdpTransport, IpTransportFixture<UnicastUdpTransportFixture>)

using UnicastUdpTransportFixtures = boost::mpl::vector<
  GENERATE_IP_TRANSPORT_FIXTURE_INSTANTIATIONS(UnicastUdpTransportFixture)
>;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(StaticProperties, T, UnicastUdpTransportFixtures, T)
{
  TRANSPORT_TEST_INIT();

  checkStaticPropertiesInitialized(*this->transport);

  BOOST_CHECK_EQUAL(this->transport->getLocalUri(), FaceUri(udp::endpoint(this->address, this->localEp.port())));
  BOOST_CHECK_EQUAL(this->transport->getRemoteUri(), FaceUri(udp::endpoint(this->address, 7070)));
  BOOST_CHECK_EQUAL(this->transport->getScope(), ndn::nfd::FACE_SCOPE_NON_LOCAL); // UDP is never local
  BOOST_CHECK_EQUAL(this->transport->getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  BOOST_CHECK_EQUAL(this->transport->getLinkType(), ndn::nfd::LINK_TYPE_POINT_TO_POINT);
  BOOST_CHECK_EQUAL(this->transport->getMtu(),
                    this->addressFamily == AddressFamily::V4 ? (65535 - 60 - 8) : (65535 - 8));
  BOOST_CHECK_GT(this->transport->getSendQueueCapacity(), 0);
}

BOOST_AUTO_TEST_CASE(PersistencyChange)
{
  TRANSPORT_TEST_INIT();

  BOOST_CHECK_EQUAL(transport->canChangePersistencyTo(ndn::nfd::FACE_PERSISTENCY_ON_DEMAND), true);
  BOOST_CHECK_EQUAL(transport->canChangePersistencyTo(ndn::nfd::FACE_PERSISTENCY_PERSISTENT), true);
  BOOST_CHECK_EQUAL(transport->canChangePersistencyTo(ndn::nfd::FACE_PERSISTENCY_PERMANENT), true);
}

BOOST_AUTO_TEST_CASE(ExpirationTime)
{
  TRANSPORT_TEST_INIT(ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  BOOST_CHECK_NE(transport->getExpirationTime(), time::steady_clock::TimePoint::max());

  transport->setPersistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  BOOST_CHECK_EQUAL(transport->getExpirationTime(), time::steady_clock::TimePoint::max());

  transport->setPersistency(ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  BOOST_CHECK_NE(transport->getExpirationTime(), time::steady_clock::TimePoint::max());
}

BOOST_AUTO_TEST_CASE(IdleClose)
{
  TRANSPORT_TEST_INIT(ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);

  int nStateChanges = 0;
  transport->afterStateChange.connect(
    [this, &nStateChanges] (TransportState oldState, TransportState newState) {
      switch (nStateChanges) {
      case 0:
        BOOST_CHECK_EQUAL(oldState, TransportState::UP);
        BOOST_CHECK_EQUAL(newState, TransportState::CLOSING);
        break;
      case 1:
        BOOST_CHECK_EQUAL(oldState, TransportState::CLOSING);
        BOOST_CHECK_EQUAL(newState, TransportState::CLOSED);
        break;
      default:
        BOOST_CHECK(false);
      }
      nStateChanges++;
      limitedIo.afterOp();
    });

  BOOST_REQUIRE_EQUAL(limitedIo.run(2, time::seconds(8)), LimitedIo::EXCEED_OPS);
  BOOST_CHECK_EQUAL(nStateChanges, 2);
}

using RemoteCloseFixture = IpTransportFixture<UnicastUdpTransportFixture,
                                              AddressFamily::Any, AddressScope::Loopback>;
using RemoteClosePersistencies = boost::mpl::vector_c<ndn::nfd::FacePersistency,
                                                      ndn::nfd::FACE_PERSISTENCY_ON_DEMAND,
                                                      ndn::nfd::FACE_PERSISTENCY_PERSISTENT>;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(RemoteClose, Persistency, RemoteClosePersistencies, RemoteCloseFixture)
{
  TRANSPORT_TEST_INIT(Persistency::value);

  transport->afterStateChange.connectSingleShot([this] (TransportState oldState, TransportState newState) {
    BOOST_CHECK_EQUAL(oldState, TransportState::UP);
    BOOST_CHECK_EQUAL(newState, TransportState::FAILED);
    this->limitedIo.afterOp();
  });

  remoteSocket.close();
  Transport::Packet pkt(ndn::encoding::makeStringBlock(300, "hello"));
  transport->send(std::move(pkt)); // trigger ICMP error
  BOOST_REQUIRE_EQUAL(limitedIo.run(1, time::seconds(1)), LimitedIo::EXCEED_OPS);

  transport->afterStateChange.connectSingleShot([this] (TransportState oldState, TransportState newState) {
    BOOST_CHECK_EQUAL(oldState, TransportState::FAILED);
    BOOST_CHECK_EQUAL(newState, TransportState::CLOSED);
    this->limitedIo.afterOp();
  });

  BOOST_REQUIRE_EQUAL(limitedIo.run(1, time::seconds(1)), LimitedIo::EXCEED_OPS);
}

BOOST_FIXTURE_TEST_CASE(RemoteClosePermanent, RemoteCloseFixture)
{
  TRANSPORT_TEST_INIT(ndn::nfd::FACE_PERSISTENCY_PERMANENT);

  remoteSocket.close();

  Block block1 = ndn::encoding::makeStringBlock(300, "hello");
  transport->send(Transport::Packet{Block{block1}}); // make a copy of the block
  BOOST_CHECK_EQUAL(transport->getCounters().nOutPackets, 1);
  BOOST_CHECK_EQUAL(transport->getCounters().nOutBytes, block1.size());

  limitedIo.defer(time::seconds(1));
  BOOST_CHECK_EQUAL(transport->getState(), TransportState::UP);

  remoteConnect();

  transport->send(Transport::Packet{Block{block1}}); // make a copy of the block
  BOOST_CHECK_EQUAL(transport->getCounters().nOutPackets, 2);
  BOOST_CHECK_EQUAL(transport->getCounters().nOutBytes, 2 * block1.size());

  std::vector<uint8_t> readBuf(block1.size());
  remoteSocket.async_receive(boost::asio::buffer(readBuf),
    [this] (const boost::system::error_code& error, size_t) {
      BOOST_REQUIRE_EQUAL(error, boost::system::errc::success);
      limitedIo.afterOp();
    });
  BOOST_REQUIRE_EQUAL(limitedIo.run(1, time::seconds(1)), LimitedIo::EXCEED_OPS);

  BOOST_CHECK_EQUAL_COLLECTIONS(readBuf.begin(), readBuf.end(), block1.begin(), block1.end());

  Block block2 = ndn::encoding::makeStringBlock(301, "world");
  ndn::Buffer buf(block2.begin(), block2.end());
  remoteWrite(buf);

  BOOST_CHECK_EQUAL(transport->getCounters().nInPackets, 1);
  BOOST_CHECK_EQUAL(transport->getCounters().nInBytes, block2.size());
  BOOST_CHECK_EQUAL(receivedPackets->size(), 1);
  BOOST_CHECK_EQUAL(transport->getState(), TransportState::UP);
}

BOOST_AUTO_TEST_SUITE_END() // TestUnicastUdpTransport
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
