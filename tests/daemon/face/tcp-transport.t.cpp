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

#include "face/tcp-transport.hpp"
#include "face/lp-face.hpp"

#include "dummy-receive-link-service.hpp"
#include "get-available-interface-ip.hpp"
#include "transport-test-common.hpp"

#include "tests/limited-io.hpp"

namespace nfd {
namespace face {
namespace tests {

using namespace nfd::tests;
namespace ip = boost::asio::ip;
using ip::tcp;

BOOST_AUTO_TEST_SUITE(Face)

class TcpTransportFixture : public BaseFixture
{
protected:
  TcpTransportFixture()
    : transport(nullptr)
    , remoteSocket(g_io)
    , receivedPackets(nullptr)
    , acceptor(g_io)
  {
  }

  void
  initialize(ip::address address = ip::address_v4::loopback())
  {
    tcp::endpoint remoteEp(address, 7070);
    acceptor.open(remoteEp.protocol());
    acceptor.set_option(tcp::acceptor::reuse_address(true));
    acceptor.bind(remoteEp);
    acceptor.listen(1);
    acceptor.async_accept(remoteSocket, bind([]{}));

    tcp::socket sock(g_io);
    sock.async_connect(remoteEp, [this] (const boost::system::error_code& error) {
      BOOST_REQUIRE_EQUAL(error, boost::system::errc::success);
      limitedIo.afterOp();
    });

    BOOST_REQUIRE_EQUAL(limitedIo.run(1, time::seconds(1)), LimitedIo::EXCEED_OPS);

    localEp = sock.local_endpoint();
    face = make_unique<LpFace>(make_unique<DummyReceiveLinkService>(),
                               make_unique<TcpTransport>(std::move(sock),
                                                         ndn::nfd::FACE_PERSISTENCY_PERSISTENT));
    transport = static_cast<TcpTransport*>(face->getTransport());
    receivedPackets = &static_cast<DummyReceiveLinkService*>(face->getLinkService())->receivedPackets;

    BOOST_REQUIRE_EQUAL(transport->getState(), TransportState::UP);
  }

  void
  remoteWrite(const ndn::Buffer& buf)
  {
    // use write() because socket::send() does not guarantee that all data is written before returning
    BOOST_REQUIRE_EQUAL(boost::asio::write(remoteSocket, boost::asio::buffer(buf)), buf.size());
    limitedIo.defer(time::milliseconds(50));
  }

protected:
  LimitedIo limitedIo;
  TcpTransport* transport;
  tcp::endpoint localEp;
  tcp::socket remoteSocket;
  std::vector<Transport::Packet>* receivedPackets;

private:
  tcp::acceptor acceptor;
  unique_ptr<LpFace> face;
};

BOOST_FIXTURE_TEST_SUITE(TestTcpTransport, TcpTransportFixture)

BOOST_AUTO_TEST_CASE(StaticPropertiesLocalIpv4)
{
  initialize();

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
  initialize(ip::address_v6::loopback());

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
  auto address = getAvailableInterfaceIp<ip::address_v4>();
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
  auto address = getAvailableInterfaceIp<ip::address_v6>();
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

BOOST_AUTO_TEST_CASE(Send)
{
  initialize();

  auto block1 = ndn::encoding::makeStringBlock(300, "hello");
  transport->send(Transport::Packet{Block{block1}}); // make a copy of the block
  BOOST_CHECK_EQUAL(transport->getCounters().nOutPackets, 1);
  BOOST_CHECK_EQUAL(transport->getCounters().nOutBytes, block1.size());

  auto block2 = ndn::encoding::makeStringBlock(301, "world");
  transport->send(Transport::Packet{Block{block2}}); // make a copy of the block
  BOOST_CHECK_EQUAL(transport->getCounters().nOutPackets, 2);
  BOOST_CHECK_EQUAL(transport->getCounters().nOutBytes, block1.size() + block2.size());

  std::vector<uint8_t> readBuf(block1.size() + block2.size());
  boost::asio::async_read(remoteSocket, boost::asio::buffer(readBuf),
    [this] (const boost::system::error_code& error, size_t) {
      BOOST_REQUIRE_EQUAL(error, boost::system::errc::success);
      limitedIo.afterOp();
    });

  BOOST_REQUIRE_EQUAL(limitedIo.run(1, time::seconds(1)), LimitedIo::EXCEED_OPS);

  BOOST_CHECK_EQUAL_COLLECTIONS(readBuf.begin(), readBuf.begin() + block1.size(), block1.begin(), block1.end());
  BOOST_CHECK_EQUAL_COLLECTIONS(readBuf.begin() + block1.size(), readBuf.end(),   block2.begin(), block2.end());
  BOOST_CHECK_EQUAL(transport->getState(), TransportState::UP);
}

BOOST_AUTO_TEST_CASE(ReceiveNormal)
{
  initialize();

  Block pkt = ndn::encoding::makeStringBlock(300, "hello");
  ndn::Buffer buf(pkt.begin(), pkt.end());
  remoteWrite(buf);

  BOOST_CHECK_EQUAL(transport->getCounters().nInPackets, 1);
  BOOST_CHECK_EQUAL(transport->getCounters().nInBytes, pkt.size());
  BOOST_CHECK_EQUAL(receivedPackets->size(), 1);
  BOOST_CHECK_EQUAL(transport->getState(), TransportState::UP);
}

BOOST_AUTO_TEST_CASE(ReceiveMultipleSegments)
{
  initialize();

  Block pkt = ndn::encoding::makeStringBlock(300, "hello");
  ndn::Buffer buf1(pkt.begin(), pkt.end() - 2);
  ndn::Buffer buf2(pkt.end() - 2, pkt.end());

  remoteWrite(buf1);

  BOOST_CHECK_EQUAL(transport->getCounters().nInPackets, 0);
  BOOST_CHECK_EQUAL(transport->getCounters().nInBytes, 0);
  BOOST_CHECK_EQUAL(receivedPackets->size(), 0);
  BOOST_CHECK_EQUAL(transport->getState(), TransportState::UP);

  remoteWrite(buf2);

  BOOST_CHECK_EQUAL(transport->getCounters().nInPackets, 1);
  BOOST_CHECK_EQUAL(transport->getCounters().nInBytes, pkt.size());
  BOOST_CHECK_EQUAL(receivedPackets->size(), 1);
  BOOST_CHECK_EQUAL(transport->getState(), TransportState::UP);
}

BOOST_AUTO_TEST_CASE(ReceiveMultipleBlocks)
{
  initialize();

  Block pkt1 = ndn::encoding::makeStringBlock(300, "hello");
  Block pkt2 = ndn::encoding::makeStringBlock(301, "world");
  ndn::Buffer buf(pkt1.size() + pkt2.size());
  std::copy(pkt1.begin(), pkt1.end(), buf.buf());
  std::copy(pkt2.begin(), pkt2.end(), buf.buf() + pkt1.size());

  remoteWrite(buf);

  BOOST_CHECK_EQUAL(transport->getCounters().nInPackets, 2);
  BOOST_CHECK_EQUAL(transport->getCounters().nInBytes, buf.size());
  BOOST_CHECK_EQUAL(receivedPackets->size(), 2);
  BOOST_CHECK_EQUAL(transport->getState(), TransportState::UP);
}

BOOST_AUTO_TEST_CASE(ReceiveTooLarge)
{
  initialize();

  std::vector<uint8_t> bytes(ndn::MAX_NDN_PACKET_SIZE + 1, 0);
  Block pkt = ndn::encoding::makeBinaryBlock(300, bytes.data(), bytes.size());
  ndn::Buffer buf(pkt.begin(), pkt.end());

  remoteWrite(buf);

  BOOST_CHECK_EQUAL(transport->getCounters().nInPackets, 0);
  BOOST_CHECK_EQUAL(transport->getCounters().nInBytes, 0);
  BOOST_CHECK_EQUAL(receivedPackets->size(), 0);
  BOOST_CHECK_EQUAL(transport->getState(), TransportState::CLOSED);
}

BOOST_AUTO_TEST_CASE(Close)
{
  initialize();

  transport->close();
  BOOST_CHECK_EQUAL(transport->getState(), TransportState::CLOSING);

  g_io.poll();
  BOOST_CHECK_EQUAL(transport->getState(), TransportState::CLOSED);
}

BOOST_AUTO_TEST_CASE(RemoteClose)
{
  initialize();

  transport->afterStateChange.connectSingleShot([this] (TransportState oldState, TransportState newState) {
    BOOST_CHECK_EQUAL(oldState, TransportState::UP);
    BOOST_CHECK_EQUAL(newState, TransportState::FAILED);
    limitedIo.afterOp();
  });

  remoteSocket.close();
  BOOST_REQUIRE_EQUAL(limitedIo.run(1, time::seconds(1)), LimitedIo::EXCEED_OPS);

  g_io.poll();
  BOOST_CHECK_EQUAL(transport->getState(), TransportState::CLOSED);
}

BOOST_AUTO_TEST_SUITE_END() // TestTcpTransport
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
