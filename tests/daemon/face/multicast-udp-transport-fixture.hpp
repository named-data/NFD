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

#ifndef NFD_TESTS_DAEMON_FACE_MULTICAST_UDP_TRANSPORT_FIXTURE_HPP
#define NFD_TESTS_DAEMON_FACE_MULTICAST_UDP_TRANSPORT_FIXTURE_HPP

#include "face/multicast-udp-transport.hpp"
#include "face/face.hpp"

#include "dummy-receive-link-service.hpp"
#include "test-ip.hpp"
#include "tests/limited-io.hpp"

namespace nfd {
namespace face {
namespace tests {

using namespace nfd::tests;
namespace ip = boost::asio::ip;
using ip::udp;

class MulticastUdpTransportFixture : public BaseFixture
{
protected:
  MulticastUdpTransportFixture()
    : transport(nullptr)
    , multicastEp(ip::address::from_string("230.15.19.47"), 7070)
    , defaultAddr(getTestIp<ip::address_v4>(LoopbackAddress::No, MulticastInterface::Yes))
    , receivedPackets(nullptr)
    , remoteSockRx(g_io)
    , remoteSockTx(g_io)
  {
  }

  void
  initialize(ip::address_v4 address)
  {
    openMulticastSockets(remoteSockRx, remoteSockTx, multicastEp.port());

    udp::socket sockRx(g_io);
    udp::socket sockTx(g_io);
    localEp = udp::endpoint(address, 7001);
    openMulticastSockets(sockRx, sockTx, localEp.port());

    face = make_unique<Face>(
             make_unique<DummyReceiveLinkService>(),
             make_unique<MulticastUdpTransport>(localEp, multicastEp, std::move(sockRx), std::move(sockTx)));
    transport = static_cast<MulticastUdpTransport*>(face->getTransport());
    receivedPackets = &static_cast<DummyReceiveLinkService*>(face->getLinkService())->receivedPackets;

    BOOST_REQUIRE_EQUAL(transport->getState(), TransportState::UP);
  }

  void
  openMulticastSockets(udp::socket& rx, udp::socket& tx, uint16_t port)
  {
    rx.open(udp::v4());
    rx.set_option(udp::socket::reuse_address(true));
    rx.bind(udp::endpoint(multicastEp.address(), port));
    rx.set_option(ip::multicast::join_group(multicastEp.address()));

    tx.open(udp::v4());
    tx.set_option(udp::socket::reuse_address(true));
    tx.set_option(ip::multicast::enable_loopback(true));
    tx.bind(udp::endpoint(ip::address_v4::any(), port));
  }

  void
  remoteRead(std::vector<uint8_t>& buf, bool needToCheck = true)
  {
    remoteSockRx.async_receive(boost::asio::buffer(buf),
      [this, needToCheck] (const boost::system::error_code& error, size_t) {
        if (needToCheck) {
          BOOST_REQUIRE_EQUAL(error, boost::system::errc::success);
        }
        limitedIo.afterOp();
      });
    BOOST_REQUIRE_EQUAL(limitedIo.run(1, time::seconds(1)), LimitedIo::EXCEED_OPS);
  }

  void
  remoteWrite(const std::vector<uint8_t>& buf, bool needToCheck = true)
  {
    remoteSockTx.async_send_to(boost::asio::buffer(buf), udp::endpoint(multicastEp.address(), 7001),
      [needToCheck] (const boost::system::error_code& error, size_t) {
        if (needToCheck) {
          BOOST_REQUIRE_EQUAL(error, boost::system::errc::success);
        }
      });
    limitedIo.defer(time::seconds(1));
  }

protected:
  LimitedIo limitedIo;
  MulticastUdpTransport* transport;
  udp::endpoint localEp;
  udp::endpoint multicastEp;
  const ip::address_v4 defaultAddr;
  std::vector<Transport::Packet>* receivedPackets;

private:
  unique_ptr<Face> face;
  udp::socket remoteSockRx;
  udp::socket remoteSockTx;
};

} // namespace tests
} // namespace face
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FACE_MULTICAST_UDP_TRANSPORT_FIXTURE_HPP
