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

#include "transport-test-common.hpp"

#include "tcp-transport-fixture.hpp"

namespace nfd {
namespace face {
namespace tests {

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestTcpTransport, TcpTransportFixture)

BOOST_AUTO_TEST_CASE(StaticPropertiesLocalIpv4)
{
  auto address = getTestIp<ip::address_v4>(LoopbackAddress::Yes);
  SKIP_IF_IP_UNAVAILABLE(address);
  initialize(address);

  checkStaticPropertiesInitialized(*transport);

  BOOST_CHECK_EQUAL(transport->getLocalUri(), FaceUri("tcp4://127.0.0.1:" + to_string(localEp.port())));
  BOOST_CHECK_EQUAL(transport->getRemoteUri(), FaceUri("tcp4://127.0.0.1:7070"));
  BOOST_CHECK_EQUAL(transport->getScope(), ndn::nfd::FACE_SCOPE_LOCAL);
  BOOST_CHECK_EQUAL(transport->getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  BOOST_CHECK_EQUAL(transport->getLinkType(), ndn::nfd::LINK_TYPE_POINT_TO_POINT);
  BOOST_CHECK_EQUAL(transport->getMtu(), MTU_UNLIMITED);
}

BOOST_AUTO_TEST_CASE(StaticPropertiesLocalIpv6)
{
  auto address = getTestIp<ip::address_v6>(LoopbackAddress::Yes);
  SKIP_IF_IP_UNAVAILABLE(address);
  initialize(address);

  checkStaticPropertiesInitialized(*transport);

  BOOST_CHECK_EQUAL(transport->getLocalUri(), FaceUri("tcp6://[::1]:" + to_string(localEp.port())));
  BOOST_CHECK_EQUAL(transport->getRemoteUri(), FaceUri("tcp6://[::1]:7070"));
  BOOST_CHECK_EQUAL(transport->getScope(), ndn::nfd::FACE_SCOPE_LOCAL);
  BOOST_CHECK_EQUAL(transport->getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  BOOST_CHECK_EQUAL(transport->getLinkType(), ndn::nfd::LINK_TYPE_POINT_TO_POINT);
  BOOST_CHECK_EQUAL(transport->getMtu(), MTU_UNLIMITED);
}

BOOST_AUTO_TEST_CASE(StaticPropertiesNonLocalIpv4)
{
  auto address = getTestIp<ip::address_v4>(LoopbackAddress::No);
  SKIP_IF_IP_UNAVAILABLE(address);
  initialize(address);

  checkStaticPropertiesInitialized(*transport);

  BOOST_CHECK_EQUAL(transport->getLocalUri(),
                    FaceUri("tcp4://" + address.to_string() + ":" + to_string(localEp.port())));
  BOOST_CHECK_EQUAL(transport->getRemoteUri(),
                    FaceUri("tcp4://" + address.to_string() + ":7070"));
  BOOST_CHECK_EQUAL(transport->getScope(), ndn::nfd::FACE_SCOPE_NON_LOCAL);
  BOOST_CHECK_EQUAL(transport->getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  BOOST_CHECK_EQUAL(transport->getLinkType(), ndn::nfd::LINK_TYPE_POINT_TO_POINT);
  BOOST_CHECK_EQUAL(transport->getMtu(), MTU_UNLIMITED);
}

BOOST_AUTO_TEST_CASE(StaticPropertiesNonLocalIpv6)
{
  auto address = getTestIp<ip::address_v6>(LoopbackAddress::No);
  SKIP_IF_IP_UNAVAILABLE(address);
  initialize(address);

  checkStaticPropertiesInitialized(*transport);

  BOOST_CHECK_EQUAL(transport->getLocalUri(),
                    FaceUri("tcp6://[" + address.to_string() + "]:" + to_string(localEp.port())));
  BOOST_CHECK_EQUAL(transport->getRemoteUri(),
                    FaceUri("tcp6://[" + address.to_string() + "]:7070"));
  BOOST_CHECK_EQUAL(transport->getScope(), ndn::nfd::FACE_SCOPE_NON_LOCAL);
  BOOST_CHECK_EQUAL(transport->getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  BOOST_CHECK_EQUAL(transport->getLinkType(), ndn::nfd::LINK_TYPE_POINT_TO_POINT);
  BOOST_CHECK_EQUAL(transport->getMtu(), MTU_UNLIMITED);
}

BOOST_AUTO_TEST_CASE(PermanentReconnect)
{
  auto address = getTestIp<ip::address_v4>();
  SKIP_IF_IP_UNAVAILABLE(address);
  initialize(address, ndn::nfd::FACE_PERSISTENCY_PERMANENT);

  transport->afterStateChange.connectSingleShot([this] (TransportState oldState, TransportState newState) {
    BOOST_CHECK_EQUAL(oldState, TransportState::UP);
    BOOST_CHECK_EQUAL(newState, TransportState::DOWN);
    limitedIo.afterOp();
  });
  remoteSocket.close();
  BOOST_REQUIRE_EQUAL(limitedIo.run(1, time::seconds(1)), LimitedIo::EXCEED_OPS);

  transport->afterStateChange.connectSingleShot([this] (TransportState oldState, TransportState newState) {
    BOOST_CHECK_EQUAL(oldState, TransportState::DOWN);
    BOOST_CHECK_EQUAL(newState, TransportState::UP);
    limitedIo.afterOp();
  });
  BOOST_REQUIRE_EQUAL(limitedIo.run(1, time::seconds(1)), LimitedIo::EXCEED_OPS);
}

BOOST_AUTO_TEST_CASE(ChangePersistencyFromPermanentWhenDown)
{
  // when persistency is changed out of permanent while transport is DOWN,
  // the transport immediately goes into FAILED state

  auto address = getTestIp<ip::address_v4>();
  SKIP_IF_IP_UNAVAILABLE(address);
  initialize(address, ndn::nfd::FACE_PERSISTENCY_PERMANENT);

  transport->afterStateChange.connectSingleShot([this] (TransportState oldState, TransportState newState) {
    BOOST_CHECK_EQUAL(oldState, TransportState::UP);
    BOOST_CHECK_EQUAL(newState, TransportState::DOWN);
    limitedIo.afterOp();
  });
  remoteSocket.close();
  BOOST_REQUIRE_EQUAL(limitedIo.run(1, time::seconds(1)), LimitedIo::EXCEED_OPS);

  bool didStateChange = false;
  transport->afterStateChange.connectSingleShot(
    [this, &didStateChange] (TransportState oldState, TransportState newState) {
      didStateChange = true;
      BOOST_CHECK_EQUAL(oldState, TransportState::DOWN);
      BOOST_CHECK_EQUAL(newState, TransportState::FAILED);
    });
  transport->setPersistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  BOOST_CHECK(didStateChange);
}

class PermanentTcpTransportReconnectObserver : public TcpTransport
{
public:
  PermanentTcpTransportReconnectObserver(protocol::socket&& socket, LimitedIo& io)
    : TcpTransport(std::move(socket), ndn::nfd::FACE_PERSISTENCY_PERMANENT)
    , m_io(io)
  {
  }

protected:
  void
  reconnect() final
  {
    TcpTransport::reconnect();
    m_io.afterOp();
  }

  void
  handleReconnect(const boost::system::error_code& error) final
  {
    TcpTransport::handleReconnect(error);
    m_io.afterOp();
  }

  void
  handleReconnectTimeout() final
  {
    TcpTransport::handleReconnectTimeout();
    m_io.afterOp();
  }

private:
  LimitedIo& m_io;
};

static double
asFloatMilliseconds(const time::nanoseconds& t)
{
  return static_cast<double>(t.count()) / 1000000.0;
}

BOOST_AUTO_TEST_CASE(PermanentReconnectWithExponentialBackoff)
{
  auto address = getTestIp<ip::address_v4>();
  SKIP_IF_IP_UNAVAILABLE(address);

  tcp::endpoint remoteEp(address, 7070);
  startAccept(remoteEp);

  tcp::socket sock(g_io);
  sock.async_connect(remoteEp, [this] (const boost::system::error_code& error) {
    BOOST_REQUIRE_EQUAL(error, boost::system::errc::success);
    limitedIo.afterOp();
  });
  BOOST_REQUIRE_EQUAL(limitedIo.run(2, time::seconds(1)), LimitedIo::EXCEED_OPS);

  auto transportObserver = make_unique<PermanentTcpTransportReconnectObserver>(std::move(sock),
                                                                               std::ref(limitedIo));
  BOOST_REQUIRE_EQUAL(transportObserver->getState(), TransportState::UP);

  // break the TCP connection
  stopAccept();
  remoteSocket.close();

  // measure retry intervals
  BOOST_REQUIRE_EQUAL(limitedIo.run(2, time::seconds(5)), LimitedIo::EXCEED_OPS);
  auto retryTime1 = time::steady_clock::now();
  BOOST_CHECK_EQUAL(transportObserver->getState(), TransportState::DOWN);

  BOOST_REQUIRE_EQUAL(limitedIo.run(2, time::seconds(5)), LimitedIo::EXCEED_OPS);
  auto retryTime2 = time::steady_clock::now();
  BOOST_CHECK_EQUAL(transportObserver->getState(), TransportState::DOWN);

  BOOST_REQUIRE_EQUAL(limitedIo.run(2, time::seconds(5)), LimitedIo::EXCEED_OPS);
  auto retryTime3 = time::steady_clock::now();
  BOOST_CHECK_EQUAL(transportObserver->getState(), TransportState::DOWN);

  // check that the backoff algorithm works
  BOOST_CHECK_CLOSE(asFloatMilliseconds(retryTime2 - retryTime1),
                    asFloatMilliseconds(TcpTransport::s_initialReconnectWait),
                    10.0);
  BOOST_CHECK_CLOSE(asFloatMilliseconds(retryTime3 - retryTime2),
                    asFloatMilliseconds(TcpTransport::s_initialReconnectWait) * TcpTransport::s_reconnectWaitMultiplier,
                    10.0);

  // reestablish the TCP connection
  startAccept(remoteEp);

  BOOST_REQUIRE_EQUAL(limitedIo.run(3, time::seconds(10)), LimitedIo::EXCEED_OPS);
  BOOST_CHECK_EQUAL(transportObserver->getState(), TransportState::UP);

  // break the TCP connection again
  stopAccept();
  remoteSocket.close();

  // measure retry intervals
  BOOST_REQUIRE_EQUAL(limitedIo.run(2, time::seconds(5)), LimitedIo::EXCEED_OPS);
  auto retryTime4 = time::steady_clock::now();
  BOOST_CHECK_EQUAL(transportObserver->getState(), TransportState::DOWN);

  BOOST_REQUIRE_EQUAL(limitedIo.run(2, time::seconds(5)), LimitedIo::EXCEED_OPS);
  auto retryTime5 = time::steady_clock::now();
  BOOST_CHECK_EQUAL(transportObserver->getState(), TransportState::DOWN);

  // check that the timeout restarts from the initial value after a successful reconnection
  BOOST_CHECK_CLOSE(asFloatMilliseconds(retryTime5 - retryTime4),
                    asFloatMilliseconds(TcpTransport::s_initialReconnectWait),
                    10.0);
}

BOOST_AUTO_TEST_SUITE_END() // TestTcpTransport
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
