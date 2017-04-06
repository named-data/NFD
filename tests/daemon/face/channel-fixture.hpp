/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
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

#ifndef NFD_TESTS_DAEMON_FACE_CHANNEL_FIXTURE_HPP
#define NFD_TESTS_DAEMON_FACE_CHANNEL_FIXTURE_HPP

#include "face/channel.hpp"
#include "tests/limited-io.hpp"

#include <type_traits>

namespace nfd {
namespace face {
namespace tests {

using namespace nfd::tests;

template<class ChannelT, class EndpointT>
class ChannelFixture : public BaseFixture
{
  static_assert(std::is_base_of<Channel, ChannelT>::value,
                "ChannelFixture must be instantiated with a type derived from Channel");

public:
  virtual
  ~ChannelFixture() = default;

  static void
  unexpectedFailure(uint32_t status, const std::string& reason)
  {
    BOOST_FAIL("No error expected, but got: [" << status << ": " << reason << "]");
  }

protected:
  uint16_t
  getNextPort()
  {
    return m_nextPort++;
  }

  virtual unique_ptr<ChannelT>
  makeChannel()
  {
    BOOST_THROW_EXCEPTION(std::logic_error("unimplemented"));
  }

  /**
   * if port == 0, use the port number returned by getNextPort()
   */
  virtual unique_ptr<ChannelT>
  makeChannel(const boost::asio::ip::address& addr, uint16_t port = 0)
  {
    BOOST_THROW_EXCEPTION(std::logic_error("unimplemented"));
  }

  void
  listen(const boost::asio::ip::address& addr)
  {
    listenerEp = EndpointT{addr, 7030};
    listenerChannel = makeChannel(addr, 7030);
    listenerChannel->listen(
      [this] (const shared_ptr<Face>& newFace) {
        BOOST_REQUIRE(newFace != nullptr);
        connectFaceClosedSignal(*newFace, [this] { limitedIo.afterOp(); });
        listenerFaces.push_back(newFace);
        limitedIo.afterOp();
      },
      ChannelFixture::unexpectedFailure);
  }

  virtual void
  connect(ChannelT&)
  {
    BOOST_THROW_EXCEPTION(std::logic_error("unimplemented"));
  }

protected:
  LimitedIo limitedIo;
  EndpointT listenerEp;
  unique_ptr<ChannelT> listenerChannel;
  std::vector<shared_ptr<Face>> listenerFaces;

private:
  uint16_t m_nextPort = 7050;
};

} // namespace tests
} // namespace face
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FACE_CHANNEL_FIXTURE_HPP
