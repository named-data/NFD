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

#include "face/unix-stream-channel.hpp"

#include "channel-fixture.hpp"

#include <boost/filesystem.hpp>
#include <fstream>

namespace nfd {
namespace face {
namespace tests {

namespace fs = boost::filesystem;
namespace local = boost::asio::local;

class UnixStreamChannelFixture : public ChannelFixture<UnixStreamChannel, unix_stream::Endpoint>
{
protected:
  UnixStreamChannelFixture()
  {
    listenerEp = unix_stream::Endpoint("nfd-test-unix-stream-channel.sock");
  }

  unique_ptr<UnixStreamChannel>
  makeChannel() final
  {
    return make_unique<UnixStreamChannel>(listenerEp, false);
  }

  void
  listen()
  {
    listenerChannel = makeChannel();
    listenerChannel->listen(
      [this] (const shared_ptr<Face>& newFace) {
        BOOST_REQUIRE(newFace != nullptr);
        connectFaceClosedSignal(*newFace, [this] { limitedIo.afterOp(); });
        listenerFaces.push_back(newFace);
        limitedIo.afterOp();
      },
      ChannelFixture::unexpectedFailure);
  }

  void
  clientConnect(local::stream_protocol::socket& client)
  {
    client.async_connect(listenerEp,
      [this] (const boost::system::error_code& error) {
        BOOST_REQUIRE_EQUAL(error, boost::system::errc::success);
        limitedIo.afterOp();
      });
  }
};

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestUnixStreamChannel, UnixStreamChannelFixture)

BOOST_AUTO_TEST_CASE(Uri)
{
  auto channel = makeChannel();
  BOOST_CHECK_EQUAL(channel->getUri(), FaceUri(listenerEp));
}

BOOST_AUTO_TEST_CASE(Listen)
{
  auto channel = makeChannel();
  BOOST_CHECK_EQUAL(channel->isListening(), false);

  channel->listen(nullptr, nullptr);
  BOOST_CHECK_EQUAL(channel->isListening(), true);

  // listen() is idempotent
  BOOST_CHECK_NO_THROW(channel->listen(nullptr, nullptr));
  BOOST_CHECK_EQUAL(channel->isListening(), true);
}

BOOST_AUTO_TEST_CASE(MultipleAccepts)
{
  this->listen();

  BOOST_CHECK_EQUAL(listenerChannel->isListening(), true);
  BOOST_CHECK_EQUAL(listenerChannel->size(), 0);

  local::stream_protocol::socket client1(g_io);
  this->clientConnect(client1);

  BOOST_CHECK_EQUAL(limitedIo.run(2, time::seconds(1)), LimitedIo::EXCEED_OPS);
  BOOST_CHECK_EQUAL(listenerChannel->size(), 1);

  local::stream_protocol::socket client2(g_io);
  local::stream_protocol::socket client3(g_io);
  this->clientConnect(client2);
  this->clientConnect(client3);

  BOOST_CHECK_EQUAL(limitedIo.run(4, time::seconds(1)), LimitedIo::EXCEED_OPS);
  BOOST_CHECK_EQUAL(listenerChannel->size(), 3);
  BOOST_CHECK_EQUAL(listenerFaces.size(), 3);

  // check face persistency
  for (const auto& face : listenerFaces) {
    BOOST_CHECK_EQUAL(face->getPersistency(), ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  }
}

BOOST_AUTO_TEST_CASE(SocketFile)
{
  fs::path socketPath(listenerEp.path());
  fs::remove(socketPath);

  auto channel = makeChannel();
  BOOST_CHECK_EQUAL(fs::symlink_status(socketPath).type(), fs::file_not_found);

  channel->listen(nullptr, nullptr);
  auto status = fs::symlink_status(socketPath);
  BOOST_CHECK_EQUAL(status.type(), fs::socket_file);
  BOOST_CHECK_EQUAL(status.permissions(), fs::owner_read | fs::group_read | fs::others_read |
                                          fs::owner_write | fs::group_write | fs::others_write);

  channel.reset();
  BOOST_CHECK_EQUAL(fs::symlink_status(socketPath).type(), fs::file_not_found);
}

BOOST_AUTO_TEST_CASE(ExistingStaleSocketFile)
{
  fs::path socketPath(listenerEp.path());
  fs::remove(socketPath);

  local::stream_protocol::acceptor acceptor(g_io, listenerEp);
  acceptor.close();
  BOOST_CHECK_EQUAL(fs::symlink_status(socketPath).type(), fs::socket_file);

  auto channel = makeChannel();
  BOOST_CHECK_NO_THROW(channel->listen(nullptr, nullptr));
  BOOST_CHECK_EQUAL(fs::symlink_status(socketPath).type(), fs::socket_file);
}

BOOST_AUTO_TEST_CASE(ExistingRegularFile)
{
  fs::path socketPath(listenerEp.path());
  fs::remove(socketPath);

  std::ofstream f(listenerEp.path());
  f.close();

  auto channel = makeChannel();
  BOOST_CHECK_THROW(channel->listen(nullptr, nullptr), UnixStreamChannel::Error);

  fs::remove(socketPath);
}

BOOST_AUTO_TEST_SUITE_END() // TestUnixStreamChannel
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
