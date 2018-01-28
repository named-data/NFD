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

#ifndef NFD_TESTS_DAEMON_FACE_UDP_CHANNEL_FIXTURE_HPP
#define NFD_TESTS_DAEMON_FACE_UDP_CHANNEL_FIXTURE_HPP

#include "face/udp-channel.hpp"
#include "face/transport.hpp"

#include "channel-fixture.hpp"

namespace nfd {
namespace face {
namespace tests {

class UdpChannelFixture : public ChannelFixture<UdpChannel, udp::Endpoint>
{
protected:
  unique_ptr<UdpChannel>
  makeChannel(const boost::asio::ip::address& addr, uint16_t port = 0) final
  {
    if (port == 0)
      port = getNextPort();

    return make_unique<UdpChannel>(udp::Endpoint(addr, port), time::seconds(2), false);
  }

  void
  connect(UdpChannel& channel) final
  {
    g_io.post([&] {
      channel.connect(listenerEp, {},
        [this] (const shared_ptr<Face>& newFace) {
          BOOST_REQUIRE(newFace != nullptr);
          connectFaceClosedSignal(*newFace, [this] { limitedIo.afterOp(); });
          clientFaces.push_back(newFace);
          face::Transport::Packet pkt(ndn::encoding::makeStringBlock(300, "hello"));
          newFace->getTransport()->send(std::move(pkt));
          limitedIo.afterOp();
        },
        ChannelFixture::unexpectedFailure);
    });
  }

protected:
  std::vector<shared_ptr<Face>> clientFaces;
};

} // namespace tests
} // namespace face
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FACE_UDP_CHANNEL_FIXTURE_HPP
