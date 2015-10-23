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

#include "face/unix-stream-transport.hpp"
#include "transport-properties.hpp"

#include "tests/test-common.hpp"
#include <boost/filesystem.hpp>

namespace nfd {
namespace face {
namespace tests {

using namespace nfd::tests;
typedef boost::asio::local::stream_protocol unix_stream;

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestUnixStreamTransport, BaseFixture)

/** \brief automatically unlinks the socket file of a Unix stream acceptor
 */
class AcceptorWithCleanup : public unix_stream::acceptor
{
public:
  AcceptorWithCleanup(const std::string& path = "")
    : unix_stream::acceptor(getGlobalIoService())
  {
    this->open();
    if (path.empty()) {
      this->bind("unix-stream-acceptor." + to_string(time::system_clock::now().time_since_epoch().count()) + ".sock");
    }
    else {
      this->bind(path);
    }
    this->listen(1);
  }

  ~AcceptorWithCleanup()
  {
    std::string path = this->local_endpoint().path();

    boost::system::error_code ec;
    this->close(ec);
    boost::filesystem::remove(path, ec);
  }
};

bool
connectToAcceptor(unix_stream::acceptor& acceptor, unix_stream::socket& sock1, unix_stream::socket& sock2)
{
  bool isAccepted = false;
  acceptor.async_accept(sock1, bind([&isAccepted] { isAccepted = true; }));

  bool isConnected = false;
  sock2.async_connect(acceptor.local_endpoint(), bind([&isConnected] { isConnected = true; }));

  getGlobalIoService().poll();

  return isAccepted && isConnected;
}

BOOST_AUTO_TEST_CASE(StaticProperties)
{
  AcceptorWithCleanup acceptor1;
  unix_stream::socket sock1(getGlobalIoService());
  unix_stream::socket sock2(getGlobalIoService());
  BOOST_CHECK(connectToAcceptor(acceptor1, sock1, sock2));

  UnixStreamTransport transport(std::move(sock1));
  checkStaticPropertiesInitialized(transport);

  BOOST_CHECK_EQUAL(transport.getLocalUri().getScheme(), "unix");
  BOOST_CHECK_EQUAL(transport.getLocalUri().getHost(), "");
  BOOST_CHECK_EQUAL(transport.getLocalUri().getPath(), acceptor1.local_endpoint().path());
  BOOST_CHECK_EQUAL(transport.getRemoteUri().getScheme(), "fd");
  BOOST_CHECK_EQUAL(transport.getScope(), ndn::nfd::FACE_SCOPE_LOCAL);
  BOOST_CHECK_EQUAL(transport.getPersistency(), ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  BOOST_CHECK_EQUAL(transport.getLinkType(), ndn::nfd::LINK_TYPE_POINT_TO_POINT);
  BOOST_CHECK_EQUAL(transport.getMtu(), MTU_UNLIMITED);
}

BOOST_AUTO_TEST_SUITE_END() // TestUnixStreamTransport
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
