/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2023,  Regents of the University of California,
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

#include "tests/test-common.hpp"
#include "tests/daemon/limited-io.hpp"
#include "tests/daemon/face/dummy-link-service.hpp"
#include "tests/daemon/face/transport-test-common.hpp"

#include <ndn-cxx/util/random.hpp>

namespace nfd::tests {

namespace ip = boost::asio::ip;
using ip::udp;
using face::MulticastUdpTransport;

class MulticastUdpTransportFixture : public GlobalIoFixture
{
protected:
  void
  initialize(const shared_ptr<const ndn::net::NetworkInterface>& netif, const ip::address& address)
  {
    ip::address mcastAddr;
    if (address.is_v4()) {
      // the administratively scoped group 224.0.0.254 is reserved for experimentation (RFC 4727)
      mcastAddr = ip::make_address_v4("224.0.0.254");
    }
    else {
      // the group FF0X::114 is reserved for experimentation at all scope levels (RFC 4727)
      auto v6Addr = ip::make_address_v6("FF01::114");
      v6Addr.scope_id(netif->getIndex());
      mcastAddr = v6Addr;
    }
    mcastEp = udp::endpoint(mcastAddr, m_randomPort(ndn::random::getRandomNumberEngine()));
    m_remoteMcastEp = udp::endpoint(mcastAddr, m_randomPort(ndn::random::getRandomNumberEngine()));

    MulticastUdpTransport::openRxSocket(m_remoteSockRx, mcastEp, address);
    MulticastUdpTransport::openTxSocket(m_remoteSockTx, udp::endpoint(address, 0), &*netif, true);

    udp::socket sockRx(g_io);
    udp::socket sockTx(g_io);
    MulticastUdpTransport::openRxSocket(sockRx, m_remoteMcastEp, address);
    MulticastUdpTransport::openTxSocket(sockTx, udp::endpoint(address, txPort), &*netif, true);

    m_face = make_unique<Face>(make_unique<DummyLinkService>(),
                               make_unique<MulticastUdpTransport>(mcastEp, std::move(sockRx), std::move(sockTx),
                                                                  ndn::nfd::LINK_TYPE_MULTI_ACCESS));
    transport = static_cast<MulticastUdpTransport*>(m_face->getTransport());
    receivedPackets = &static_cast<DummyLinkService*>(m_face->getLinkService())->receivedPackets;

    BOOST_REQUIRE_EQUAL(transport->getState(), face::TransportState::UP);
  }

  void
  remoteRead(std::vector<uint8_t>& buf, bool needToCheck = true)
  {
    m_remoteSockRx.async_receive(boost::asio::buffer(buf),
      [this, needToCheck] (const auto& error, size_t) {
        if (needToCheck) {
          BOOST_REQUIRE_EQUAL(error, boost::system::errc::success);
        }
        limitedIo.afterOp();
      });
    BOOST_REQUIRE_EQUAL(limitedIo.run(1, 1_s), LimitedIo::EXCEED_OPS);
  }

  void
  remoteWrite(const std::vector<uint8_t>& buf, bool needToCheck = true)
  {
    sendToGroup(m_remoteSockTx, buf, needToCheck);
    limitedIo.defer(1_s);
  }

  void
  sendToGroup(udp::socket& sock, const std::vector<uint8_t>& buf, bool needToCheck = true) const
  {
    sock.async_send_to(boost::asio::buffer(buf), m_remoteMcastEp,
      [needToCheck] (const auto& error, size_t) {
        if (needToCheck) {
          BOOST_REQUIRE_EQUAL(error, boost::system::errc::success);
        }
      });
  }

protected:
  LimitedIo limitedIo;
  MulticastUdpTransport* transport = nullptr;
  udp::endpoint mcastEp;
  uint16_t txPort = 7001;
  std::vector<RxPacket>* receivedPackets = nullptr;

private:
  std::uniform_int_distribution<uint16_t> m_randomPort{10000};
  udp::endpoint m_remoteMcastEp;
  udp::socket m_remoteSockRx{g_io};
  udp::socket m_remoteSockTx{g_io};
  unique_ptr<Face> m_face;
};

} // namespace nfd::tests

#endif // NFD_TESTS_DAEMON_FACE_MULTICAST_UDP_TRANSPORT_FIXTURE_HPP
