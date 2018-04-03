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

#include "tcp-transport-fixture.hpp"

#include <boost/mpl/vector.hpp>

namespace nfd {
namespace face {
namespace tests {

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestTcpTransport, IpTransportFixture<TcpTransportFixture>)

using TcpTransportFixtures = boost::mpl::vector<
  GENERATE_IP_TRANSPORT_FIXTURE_INSTANTIATIONS(TcpTransportFixture)
>;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(StaticProperties, T, TcpTransportFixtures, T)
{
  TRANSPORT_TEST_INIT();

  checkStaticPropertiesInitialized(*this->transport);

  BOOST_CHECK_EQUAL(this->transport->getLocalUri(), FaceUri(tcp::endpoint(this->address, this->localEp.port())));
  BOOST_CHECK_EQUAL(this->transport->getRemoteUri(), FaceUri(tcp::endpoint(this->address, 7070)));
  BOOST_CHECK_EQUAL(this->transport->getScope(),
                    this->addressScope == AddressScope::Loopback ? ndn::nfd::FACE_SCOPE_LOCAL
                                                                 : ndn::nfd::FACE_SCOPE_NON_LOCAL);
  BOOST_CHECK_EQUAL(this->transport->getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  BOOST_CHECK_EQUAL(this->transport->getLinkType(), ndn::nfd::LINK_TYPE_POINT_TO_POINT);
  BOOST_CHECK_EQUAL(this->transport->getMtu(), MTU_UNLIMITED);
  BOOST_CHECK_EQUAL(this->transport->getSendQueueCapacity(), QUEUE_UNSUPPORTED);
}

BOOST_AUTO_TEST_CASE(PersistencyChange)
{
  TRANSPORT_TEST_INIT();

  BOOST_CHECK_EQUAL(transport->canChangePersistencyTo(ndn::nfd::FACE_PERSISTENCY_ON_DEMAND), true);
  BOOST_CHECK_EQUAL(transport->canChangePersistencyTo(ndn::nfd::FACE_PERSISTENCY_PERSISTENT), true);
  BOOST_CHECK_EQUAL(transport->canChangePersistencyTo(ndn::nfd::FACE_PERSISTENCY_PERMANENT), true);
}

BOOST_AUTO_TEST_CASE(ChangePersistencyFromPermanentWhenDown)
{
  // when persistency is changed out of permanent while transport is DOWN,
  // the transport immediately goes into FAILED state

  TRANSPORT_TEST_INIT(ndn::nfd::FACE_PERSISTENCY_PERMANENT);

  transport->afterStateChange.connectSingleShot(
    [this] (TransportState oldState, TransportState newState) {
      BOOST_CHECK_EQUAL(oldState, TransportState::UP);
      BOOST_CHECK_EQUAL(newState, TransportState::DOWN);
      limitedIo.afterOp();
    });
  remoteSocket.close();
  BOOST_REQUIRE_EQUAL(limitedIo.run(1, 1_s), LimitedIo::EXCEED_OPS);

  bool didStateChange = false;
  transport->afterStateChange.connectSingleShot(
    [&didStateChange] (TransportState oldState, TransportState newState) {
      didStateChange = true;
      BOOST_CHECK_EQUAL(oldState, TransportState::DOWN);
      BOOST_CHECK_EQUAL(newState, TransportState::FAILED);
    });
  transport->setPersistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  BOOST_CHECK(didStateChange);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(PermanentReconnect, T, TcpTransportFixtures, T)
{
  TRANSPORT_TEST_INIT(ndn::nfd::FACE_PERSISTENCY_PERMANENT);

  this->transport->afterStateChange.connectSingleShot(
    [this] (TransportState oldState, TransportState newState) {
      BOOST_CHECK_EQUAL(oldState, TransportState::UP);
      BOOST_CHECK_EQUAL(newState, TransportState::DOWN);
      this->limitedIo.afterOp();
    });
  this->remoteSocket.close();
  BOOST_REQUIRE_EQUAL(this->limitedIo.run(1, 1_s), LimitedIo::EXCEED_OPS);

  this->transport->afterStateChange.connectSingleShot(
    [this] (TransportState oldState, TransportState newState) {
      BOOST_CHECK_EQUAL(oldState, TransportState::DOWN);
      BOOST_CHECK_EQUAL(newState, TransportState::UP);
      this->limitedIo.afterOp();
    });
  BOOST_REQUIRE_EQUAL(this->limitedIo.run(1, 1_s), LimitedIo::EXCEED_OPS);
}

class PermanentTcpTransportReconnectObserver : public TcpTransport
{
public:
  PermanentTcpTransportReconnectObserver(protocol::socket&& socket, LimitedIo& io)
    : TcpTransport(std::move(socket), ndn::nfd::FACE_PERSISTENCY_PERMANENT, ndn::nfd::FACE_SCOPE_LOCAL)
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

    // don't count this invocation as an "op" if the reconnection attempt failed,
    // because in that case we will instead count the handleReconnectTimeout()
    // that will eventually be called as well
    if (getState() == TransportState::UP)
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

BOOST_FIXTURE_TEST_CASE_TEMPLATE(PermanentReconnectWithExponentialBackoff, T, TcpTransportFixtures, T)
{
  TRANSPORT_TEST_CHECK_PRECONDITIONS();
  // do not initialize

  tcp::endpoint remoteEp(this->address, 7070);
  this->startAccept(remoteEp);

  tcp::socket sock(this->g_io);
  sock.async_connect(remoteEp, [this] (const boost::system::error_code& error) {
    BOOST_REQUIRE_EQUAL(error, boost::system::errc::success);
    this->limitedIo.afterOp();
  });
  BOOST_REQUIRE_EQUAL(this->limitedIo.run(2, 1_s), LimitedIo::EXCEED_OPS);

  auto transportObserver =
    make_unique<PermanentTcpTransportReconnectObserver>(std::move(sock), std::ref(this->limitedIo));
  BOOST_REQUIRE_EQUAL(transportObserver->getState(), TransportState::UP);

  // break the TCP connection
  this->stopAccept();
  this->remoteSocket.close();

  // measure retry intervals
  auto retryTime1 = time::steady_clock::now();

  auto expectedWait1 = TcpTransport::s_initialReconnectWait;
  BOOST_REQUIRE_EQUAL(this->limitedIo.run(2, expectedWait1 + 1_s), // add some slack
                      LimitedIo::EXCEED_OPS);
  auto retryTime2 = time::steady_clock::now();
  BOOST_CHECK_EQUAL(transportObserver->getState(), TransportState::DOWN);

  auto expectedWait2 = time::duration_cast<time::nanoseconds>(expectedWait1 *
                                                              TcpTransport::s_reconnectWaitMultiplier);
  BOOST_REQUIRE_EQUAL(this->limitedIo.run(2, expectedWait2 + 1_s), // add some slack
                      LimitedIo::EXCEED_OPS);
  auto retryTime3 = time::steady_clock::now();
  BOOST_CHECK_EQUAL(transportObserver->getState(), TransportState::DOWN);

  // check that the backoff algorithm works
  BOOST_CHECK_CLOSE(asFloatMilliseconds(retryTime2 - retryTime1),
                    asFloatMilliseconds(expectedWait1), 20.0); // 200ms tolerance
  BOOST_CHECK_CLOSE(asFloatMilliseconds(retryTime3 - retryTime2),
                    asFloatMilliseconds(expectedWait2), 10.0); // 200ms tolerance

  // reestablish the TCP connection
  this->startAccept(remoteEp);

  auto expectedWait3 = time::duration_cast<time::nanoseconds>(expectedWait2 *
                                                              TcpTransport::s_reconnectWaitMultiplier);
  BOOST_REQUIRE_EQUAL(this->limitedIo.run(3, // reconnect, handleReconnect, async_accept
                                          expectedWait3 + 1_s), LimitedIo::EXCEED_OPS);
  BOOST_CHECK_EQUAL(transportObserver->getState(), TransportState::UP);

  // break the TCP connection again
  this->stopAccept();
  this->remoteSocket.close();

  // measure retry intervals
  auto retryTime4 = time::steady_clock::now();
  BOOST_REQUIRE_EQUAL(this->limitedIo.run(2, expectedWait1 + 1_s), // add some slack
                      LimitedIo::EXCEED_OPS);
  auto retryTime5 = time::steady_clock::now();
  BOOST_CHECK_EQUAL(transportObserver->getState(), TransportState::DOWN);

  // check that the timeout restarts from the initial value after a successful reconnection
  BOOST_CHECK_CLOSE(asFloatMilliseconds(retryTime5 - retryTime4),
                    asFloatMilliseconds(expectedWait1), 20.0); // 200ms tolerance
}

BOOST_AUTO_TEST_SUITE_END() // TestTcpTransport
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
