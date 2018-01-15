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

#ifndef NFD_TESTS_DAEMON_FACE_MULTICAST_UDP_TRANSPORT_FIXTURE_HPP
#define NFD_TESTS_DAEMON_FACE_MULTICAST_UDP_TRANSPORT_FIXTURE_HPP

#include "face/multicast-udp-transport.hpp"
#include "face/face.hpp"

#include "dummy-receive-link-service.hpp"
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
    , txPort(7001)
    , receivedPackets(nullptr)
    , remoteSockRx(g_io)
    , remoteSockTx(g_io)
  {
  }

  void
  initialize(ip::address address)
  {
    ip::address mcastAddr;
    if (address.is_v4()) {
      // the administratively scoped group 224.0.0.254 is reserved for experimentation (RFC 4727)
      mcastAddr = ip::address_v4(0xE00000FE);
    }
    else {
      // the group FF0X::114 is reserved for experimentation at all scope levels (RFC 4727)
      auto v6Addr = ip::address_v6::from_string("FF01::114");
      v6Addr.scope_id(address.to_v6().scope_id());
      mcastAddr = v6Addr;
    }
    mcastEp = udp::endpoint(mcastAddr, 7373);
    remoteMcastEp = udp::endpoint(mcastAddr, 8383);

    MulticastUdpTransport::openRxSocket(remoteSockRx, mcastEp, address);
    MulticastUdpTransport::openTxSocket(remoteSockTx, udp::endpoint(address, 0), nullptr, true);

    udp::socket sockRx(g_io);
    udp::socket sockTx(g_io);
    MulticastUdpTransport::openRxSocket(sockRx, remoteMcastEp, address);
    MulticastUdpTransport::openTxSocket(sockTx, udp::endpoint(address, txPort), nullptr, true);

    face = make_unique<Face>(
             make_unique<DummyReceiveLinkService>(),
             make_unique<MulticastUdpTransport>(mcastEp, std::move(sockRx), std::move(sockTx),
                                                ndn::nfd::LINK_TYPE_MULTI_ACCESS));
    transport = static_cast<MulticastUdpTransport*>(face->getTransport());
    receivedPackets = &static_cast<DummyReceiveLinkService*>(face->getLinkService())->receivedPackets;

    BOOST_REQUIRE_EQUAL(transport->getState(), TransportState::UP);
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
    sendToGroup(remoteSockTx, buf, needToCheck);
    limitedIo.defer(time::seconds(1));
  }

  void
  sendToGroup(udp::socket& sock, const std::vector<uint8_t>& buf, bool needToCheck = true) const
  {
    sock.async_send_to(boost::asio::buffer(buf), remoteMcastEp,
      [needToCheck] (const boost::system::error_code& error, size_t) {
        if (needToCheck) {
          BOOST_REQUIRE_EQUAL(error, boost::system::errc::success);
        }
      });
  }

protected:
  LimitedIo limitedIo;
  MulticastUdpTransport* transport;
  udp::endpoint mcastEp;
  uint16_t txPort;
  std::vector<Transport::Packet>* receivedPackets;

private:
  unique_ptr<Face> face;
  udp::endpoint remoteMcastEp;
  udp::socket remoteSockRx;
  udp::socket remoteSockTx;
};

} // namespace tests
} // namespace face
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FACE_MULTICAST_UDP_TRANSPORT_FIXTURE_HPP
