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

#include "face/unix-stream-channel.hpp"

#include "channel-fixture.hpp"

#include <boost/filesystem.hpp>
#include <fstream>

namespace nfd::tests {

namespace fs = boost::filesystem;
namespace local = boost::asio::local;
using face::UnixStreamChannel;

class UnixStreamChannelFixture : public ChannelFixture<UnixStreamChannel, unix_stream::Endpoint>
{
protected:
  UnixStreamChannelFixture()
  {
    listenerEp = unix_stream::Endpoint(socketPath.string());

    // in case an earlier test run crashed without a chance to run the destructor
    boost::system::error_code ec;
    fs::remove_all(testDir, ec);
  }

  ~UnixStreamChannelFixture() override
  {
    // cleanup
    boost::system::error_code ec;
    fs::remove_all(testDir, ec);
  }

  shared_ptr<UnixStreamChannel>
  makeChannel() final
  {
    return std::make_shared<UnixStreamChannel>(listenerEp, false);
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
      [this] (const auto& error) {
        BOOST_REQUIRE_EQUAL(error, boost::system::errc::success);
        limitedIo.afterOp();
      });
  }

protected:
  fs::path testDir = fs::path(UNIT_TESTS_TMPDIR) / "unix-stream-channel";
  fs::path socketPath = testDir / "test" / "foo.sock";
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
  channel->listen(nullptr, nullptr);
  BOOST_CHECK_EQUAL(channel->isListening(), true);
}

BOOST_AUTO_TEST_CASE(MultipleAccepts)
{
  this->listen();

  BOOST_CHECK_EQUAL(listenerChannel->isListening(), true);
  BOOST_CHECK_EQUAL(listenerChannel->size(), 0);

  local::stream_protocol::socket client1(g_io);
  this->clientConnect(client1);

  BOOST_CHECK_EQUAL(limitedIo.run(2, 1_s), LimitedIo::EXCEED_OPS);
  BOOST_CHECK_EQUAL(listenerChannel->size(), 1);

  local::stream_protocol::socket client2(g_io);
  local::stream_protocol::socket client3(g_io);
  this->clientConnect(client2);
  this->clientConnect(client3);

  BOOST_CHECK_EQUAL(limitedIo.run(4, 1_s), LimitedIo::EXCEED_OPS);
  BOOST_CHECK_EQUAL(listenerChannel->size(), 3);
  BOOST_CHECK_EQUAL(listenerFaces.size(), 3);

  // check face persistency and channel association
  for (const auto& face : listenerFaces) {
    BOOST_CHECK_EQUAL(face->getPersistency(), ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
    BOOST_CHECK_EQUAL(face->getChannel().lock(), listenerChannel);
  }
}

BOOST_AUTO_TEST_SUITE(SocketFile)

BOOST_AUTO_TEST_CASE(CreateAndRemove)
{
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

BOOST_AUTO_TEST_CASE(InUse)
{
  auto channel = makeChannel();
  fs::create_directories(socketPath.parent_path());

  local::stream_protocol::acceptor acceptor(g_io, listenerEp);
  BOOST_CHECK_EQUAL(fs::symlink_status(socketPath).type(), fs::socket_file);

  BOOST_CHECK_EXCEPTION(channel->listen(nullptr, nullptr), fs::filesystem_error, [&] (const auto& e) {
    return e.code() == boost::system::errc::address_in_use &&
           e.path1() == socketPath &&
           std::string_view(e.what()).find("UnixStreamChannel::listen") != std::string_view::npos;
  });
}

BOOST_AUTO_TEST_CASE(Stale)
{
  auto channel = makeChannel();
  fs::create_directories(socketPath.parent_path());

  local::stream_protocol::acceptor acceptor(g_io, listenerEp);
  acceptor.close();
  // the socket file is not removed when the acceptor is closed
  BOOST_CHECK_EQUAL(fs::symlink_status(socketPath).type(), fs::socket_file);

  // drop write permission from the parent directory
  fs::permissions(socketPath.parent_path(), fs::owner_all & ~fs::owner_write);
  // removal of the "stale" socket fails due to insufficient permissions
  BOOST_CHECK_EXCEPTION(channel->listen(nullptr, nullptr), fs::filesystem_error, [&] (const auto& e) {
    return e.code() == boost::system::errc::permission_denied &&
           e.path1() == socketPath &&
           std::string_view(e.what()).find("remove") != std::string_view::npos;
  });
  BOOST_CHECK_EQUAL(fs::symlink_status(socketPath).type(), fs::socket_file);

  // restore all permissions
  fs::permissions(socketPath.parent_path(), fs::owner_all);
  // the socket file should be considered "stale" and overwritten
  channel->listen(nullptr, nullptr);
  BOOST_CHECK_EQUAL(fs::symlink_status(socketPath).type(), fs::socket_file);
}

BOOST_AUTO_TEST_CASE(NotASocket)
{
  auto channel = makeChannel();

  fs::create_directories(socketPath);
  BOOST_CHECK_EQUAL(fs::symlink_status(socketPath).type(), fs::directory_file);
  BOOST_CHECK_EXCEPTION(channel->listen(nullptr, nullptr), fs::filesystem_error, [&] (const auto& e) {
    return e.code() == boost::system::errc::not_a_socket &&
           e.path1() == socketPath &&
           std::string_view(e.what()).find("UnixStreamChannel::listen") != std::string_view::npos;
  });

  fs::remove(socketPath);
  std::ofstream f(socketPath.string());
  f.close();
  BOOST_CHECK_EQUAL(fs::symlink_status(socketPath).type(), fs::regular_file);
  BOOST_CHECK_EXCEPTION(channel->listen(nullptr, nullptr), fs::filesystem_error, [&] (const auto& e) {
    return e.code() == boost::system::errc::not_a_socket &&
           e.path1() == socketPath &&
           std::string_view(e.what()).find("UnixStreamChannel::listen") != std::string_view::npos;
  });
}

BOOST_AUTO_TEST_CASE(ParentConflict)
{
  auto channel = makeChannel();
  fs::create_directories(testDir);

  auto parent = socketPath.parent_path();
  std::ofstream f(parent.string());
  f.close();
  BOOST_CHECK_EQUAL(fs::symlink_status(parent).type(), fs::regular_file);
  BOOST_CHECK_EXCEPTION(channel->listen(nullptr, nullptr), fs::filesystem_error, [&] (const auto& e) {
    return e.code() == boost::system::errc::file_exists &&
           e.path1() == parent &&
           std::string_view(e.what()).find("create_dir") != std::string_view::npos;
  });
}

BOOST_AUTO_TEST_CASE(PermissionDenied)
{
  auto channel = makeChannel();
  fs::create_directories(testDir);

  fs::permissions(testDir, fs::no_perms);
  BOOST_CHECK_EXCEPTION(channel->listen(nullptr, nullptr), fs::filesystem_error, [&] (const auto& e) {
    return e.code() == boost::system::errc::permission_denied &&
           e.path1() == socketPath.parent_path() &&
           std::string_view(e.what()).find("create_dir") != std::string_view::npos;
  });

  fs::permissions(testDir, fs::owner_read | fs::owner_exe);
  BOOST_CHECK_EXCEPTION(channel->listen(nullptr, nullptr), fs::filesystem_error, [&] (const auto& e) {
    return e.code() == boost::system::errc::permission_denied &&
           e.path1() == socketPath.parent_path() &&
           std::string_view(e.what()).find("create_dir") != std::string_view::npos;
  });

  fs::permissions(testDir, fs::owner_all);
  fs::create_directories(socketPath.parent_path());

  fs::permissions(socketPath.parent_path(), fs::no_perms);
  BOOST_CHECK_EXCEPTION(channel->listen(nullptr, nullptr), fs::filesystem_error, [&] (const auto& e) {
    return e.code() == boost::system::errc::permission_denied &&
           e.path1() == socketPath &&
           std::string_view(e.what()).find("status") != std::string_view::npos;
  });

  fs::permissions(socketPath.parent_path(), fs::owner_read | fs::owner_exe);
  BOOST_CHECK_EXCEPTION(channel->listen(nullptr, nullptr), fs::filesystem_error, [&] (const auto& e) {
    return e.code() == boost::system::errc::permission_denied &&
           e.path1() == socketPath &&
           std::string_view(e.what()).find("bind") != std::string_view::npos;
  });

  fs::permissions(socketPath.parent_path(), fs::owner_all);
}

BOOST_AUTO_TEST_SUITE_END() // SocketFile

BOOST_AUTO_TEST_SUITE_END() // TestUnixStreamChannel
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace nfd::tests
