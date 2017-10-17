/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#include "dummy-receive-link-service.hpp"
#include "tests/limited-io.hpp"

#include <boost/filesystem.hpp>

namespace nfd {
namespace face {
namespace tests {

using namespace nfd::tests;
typedef boost::asio::local::stream_protocol unix_stream;

/** \brief automatically unlinks the socket file of a Unix stream acceptor
 */
class AcceptorWithCleanup : public unix_stream::acceptor
{
public:
  explicit
  AcceptorWithCleanup(boost::asio::io_service& io, const std::string& path = "")
    : unix_stream::acceptor(io)
  {
    this->open();

    if (path.empty()) {
      this->bind("nfd-unix-stream-test." + to_string(time::system_clock::now().time_since_epoch().count()) + ".sock");
    }
    else {
      this->bind(path);
    }

    this->listen(1);
  }

  ~AcceptorWithCleanup()
  {
    boost::system::error_code ec;

    std::string path = this->local_endpoint(ec).path();
    if (ec) {
      return;
    }

    this->close(ec);
    boost::filesystem::remove(path, ec);
  }
};

class UnixStreamTransportFixture : public BaseFixture
{
protected:
  UnixStreamTransportFixture()
    : transport(nullptr)
    , remoteSocket(g_io)
    , receivedPackets(nullptr)
    , acceptor(g_io)
  {
  }

  std::pair<bool, std::string>
  checkPreconditions() const
  {
    return {true, ""};
  }

  void
  initialize()
  {
    unix_stream::socket sock(g_io);
    acceptor.async_accept(sock, [this] (const boost::system::error_code& error) {
      BOOST_REQUIRE_EQUAL(error, boost::system::errc::success);
      limitedIo.afterOp();
    });

    unix_stream::endpoint remoteEp(acceptor.local_endpoint());
    remoteSocket.async_connect(remoteEp, [this] (const boost::system::error_code& error) {
      BOOST_REQUIRE_EQUAL(error, boost::system::errc::success);
      limitedIo.afterOp();
    });

    BOOST_REQUIRE_EQUAL(limitedIo.run(2, time::seconds(1)), LimitedIo::EXCEED_OPS);

    localEp = sock.local_endpoint();
    face = make_unique<Face>(make_unique<DummyReceiveLinkService>(),
                             make_unique<UnixStreamTransport>(std::move(sock)));
    transport = static_cast<UnixStreamTransport*>(face->getTransport());
    receivedPackets = &static_cast<DummyReceiveLinkService*>(face->getLinkService())->receivedPackets;

    BOOST_REQUIRE_EQUAL(transport->getState(), TransportState::UP);
  }

  void
  remoteWrite(const ndn::Buffer& buf, bool needToCheck = true)
  {
    boost::asio::async_write(remoteSocket, boost::asio::buffer(buf),
      [needToCheck] (const boost::system::error_code& error, size_t) {
        if (needToCheck) {
          BOOST_REQUIRE_EQUAL(error, boost::system::errc::success);
        }
      });
    limitedIo.defer(time::seconds(1));
  }

protected:
  LimitedIo limitedIo;
  UnixStreamTransport* transport;
  unix_stream::endpoint localEp;
  unix_stream::socket remoteSocket;
  std::vector<Transport::Packet>* receivedPackets;

private:
  AcceptorWithCleanup acceptor;
  unique_ptr<Face> face;
};

} // namespace tests
} // namespace face
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FACE_UNIX_STREAM_TRANSPORT_FIXTURE_HPP
