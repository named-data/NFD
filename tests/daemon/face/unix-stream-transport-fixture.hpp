/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024,  Regents of the University of California,
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

#ifndef NFD_TESTS_DAEMON_FACE_UNIX_STREAM_TRANSPORT_FIXTURE_HPP
#define NFD_TESTS_DAEMON_FACE_UNIX_STREAM_TRANSPORT_FIXTURE_HPP

#include "face/unix-stream-transport.hpp"
#include "face/face.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/limited-io.hpp"
#include "tests/daemon/face/dummy-link-service.hpp"
#include "tests/daemon/face/transport-test-common.hpp"

#include <boost/filesystem.hpp>

namespace nfd::tests {

using unix_stream = boost::asio::local::stream_protocol;
using face::UnixStreamTransport;

/**
 * \brief Automatically unlinks the socket file of a Unix stream acceptor.
 */
class AcceptorWithCleanup : public unix_stream::acceptor
{
public:
  explicit
  AcceptorWithCleanup(boost::asio::io_context& io, const std::string& path = "")
    : unix_stream::acceptor(io)
  {
    open();

    if (path.empty()) {
      bind("nfd-unix-stream-test." + std::to_string(time::system_clock::now().time_since_epoch().count()) + ".sock");
    }
    else {
      bind(path);
    }

    listen(1);
  }

  ~AcceptorWithCleanup()
  {
    boost::system::error_code ec;
    std::string path = local_endpoint(ec).path();
    if (ec) {
      return;
    }

    close(ec);
    boost::filesystem::remove(path, ec);
  }
};

class UnixStreamTransportFixture : public GlobalIoFixture
{
protected:
  [[nodiscard]] std::tuple<bool, std::string>
  checkPreconditions() const
  {
    return {true, ""};
  }

  void
  initialize()
  {
    unix_stream::socket sock(g_io);
    m_acceptor.async_accept(sock, [this] (const auto& error) {
      BOOST_REQUIRE_EQUAL(error, boost::system::errc::success);
      limitedIo.afterOp();
    });

    unix_stream::endpoint remoteEp(m_acceptor.local_endpoint());
    remoteSocket.async_connect(remoteEp, [this] (const auto& error) {
      BOOST_REQUIRE_EQUAL(error, boost::system::errc::success);
      limitedIo.afterOp();
    });

    BOOST_REQUIRE_EQUAL(limitedIo.run(2, 1_s), LimitedIo::EXCEED_OPS);

    localEp = sock.local_endpoint();
    m_face = make_unique<Face>(make_unique<DummyLinkService>(),
                               make_unique<UnixStreamTransport>(std::move(sock)));
    transport = static_cast<UnixStreamTransport*>(m_face->getTransport());
    receivedPackets = &static_cast<DummyLinkService*>(m_face->getLinkService())->receivedPackets;

    BOOST_REQUIRE_EQUAL(transport->getState(), face::TransportState::UP);
  }

  void
  remoteWrite(const ndn::Buffer& buf, bool needToCheck = true)
  {
    boost::asio::async_write(remoteSocket, boost::asio::buffer(buf),
      [needToCheck] (const auto& error, size_t) {
        if (needToCheck) {
          BOOST_REQUIRE_EQUAL(error, boost::system::errc::success);
        }
      });
    limitedIo.defer(1_s);
  }

protected:
  LimitedIo limitedIo;
  UnixStreamTransport* transport = nullptr;
  unix_stream::endpoint localEp;
  unix_stream::socket remoteSocket{g_io};
  std::vector<RxPacket>* receivedPackets = nullptr;

private:
  AcceptorWithCleanup m_acceptor{g_io};
  unique_ptr<Face> m_face;
};

} // namespace nfd::tests

#endif // NFD_TESTS_DAEMON_FACE_UNIX_STREAM_TRANSPORT_FIXTURE_HPP
