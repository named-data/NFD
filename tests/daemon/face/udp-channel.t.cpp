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

#include "face/udp-channel.hpp"
#include "face/transport.hpp"

#include "tests/limited-io.hpp"
#include "tests/test-common.hpp"

#include <boost/mpl/vector.hpp>

namespace nfd {
namespace tests {

BOOST_AUTO_TEST_SUITE(Face)

using nfd::Face;
namespace ip = boost::asio::ip;

typedef boost::mpl::vector<ip::address_v4,
                           ip::address_v6> AddressTypes;

class UdpChannelFixture : public BaseFixture
{
protected:
  UdpChannelFixture()
    : nextPort(7050)
  {
  }

  unique_ptr<UdpChannel>
  makeChannel(const ip::address& addr, uint16_t port = 0)
  {
    if (port == 0)
      port = nextPort++;

    return make_unique<UdpChannel>(udp::Endpoint(addr, port), time::seconds(2));
  }

  void
  listen(const ip::address& addr)
  {
    listenerEp = udp::Endpoint(addr, 7030);
    listenerChannel = makeChannel(addr, 7030);
    listenerChannel->listen(
      [this] (const shared_ptr<Face>& newFace) {
        BOOST_REQUIRE(newFace != nullptr);
        connectFaceClosedSignal(*newFace, [this] { limitedIo.afterOp(); });
        listenerFaces.push_back(newFace);
        limitedIo.afterOp();
      },
      [] (const std::string& reason) {
        BOOST_FAIL(reason);
      });
  }

  void
  connect(UdpChannel& channel)
  {
    g_io.post([&] {
      channel.connect(listenerEp, ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
        [this] (const shared_ptr<Face>& newFace) {
          BOOST_REQUIRE(newFace != nullptr);
          connectFaceClosedSignal(*newFace, [this] { limitedIo.afterOp(); });
          clientFaces.push_back(newFace);
          face::Transport::Packet pkt(ndn::encoding::makeStringBlock(300, "hello"));
          newFace->getTransport()->send(std::move(pkt));
          limitedIo.afterOp();
        },
        [] (const std::string& reason) {
          BOOST_FAIL(reason);
        });
    });
  }

protected:
  LimitedIo limitedIo;
  udp::Endpoint listenerEp;
  unique_ptr<UdpChannel> listenerChannel;
  std::vector<shared_ptr<Face>> listenerFaces;
  std::vector<shared_ptr<Face>> clientFaces;

private:
  uint16_t nextPort;
};

BOOST_FIXTURE_TEST_SUITE(TestUdpChannel, UdpChannelFixture)

BOOST_AUTO_TEST_CASE_TEMPLATE(Uri, A, AddressTypes)
{
  udp::Endpoint ep(A::loopback(), 7050);
  auto channel = makeChannel(ep.address(), ep.port());
  BOOST_CHECK_EQUAL(channel->getUri(), FaceUri(ep));
}

BOOST_AUTO_TEST_CASE(Listen)
{
  auto channel = makeChannel(ip::address_v4());
  BOOST_CHECK_EQUAL(channel->isListening(), false);

  channel->listen(nullptr, nullptr);
  BOOST_CHECK_EQUAL(channel->isListening(), true);

  // listen() is idempotent
  BOOST_CHECK_NO_THROW(channel->listen(nullptr, nullptr));
  BOOST_CHECK_EQUAL(channel->isListening(), true);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(MultipleAccepts, A, AddressTypes)
{
  this->listen(A::loopback());

  BOOST_CHECK_EQUAL(listenerChannel->isListening(), true);
  BOOST_CHECK_EQUAL(listenerChannel->size(), 0);

  auto ch1 = makeChannel(A());
  this->connect(*ch1);

  BOOST_CHECK(limitedIo.run(2, time::seconds(1)) == LimitedIo::EXCEED_OPS);

  BOOST_CHECK_EQUAL(listenerChannel->size(), 1);
  BOOST_CHECK_EQUAL(ch1->size(), 1);
  BOOST_CHECK_EQUAL(ch1->isListening(), false);

  auto ch2 = makeChannel(A());
  auto ch3 = makeChannel(A());
  this->connect(*ch2);
  this->connect(*ch3);

  BOOST_CHECK(limitedIo.run(4, time::seconds(1)) == LimitedIo::EXCEED_OPS);

  BOOST_CHECK_EQUAL(listenerChannel->size(), 3);
  BOOST_CHECK_EQUAL(ch1->size(), 1);
  BOOST_CHECK_EQUAL(ch2->size(), 1);
  BOOST_CHECK_EQUAL(ch3->size(), 1);
  BOOST_CHECK_EQUAL(clientFaces.size(), 3);

  // check face persistency
  for (const auto& face : listenerFaces) {
    BOOST_CHECK_EQUAL(face->getPersistency(), ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  }
  for (const auto& face : clientFaces) {
    BOOST_CHECK_EQUAL(face->getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  }

  // connect twice to the same endpoint
  this->connect(*ch3);

  BOOST_CHECK(limitedIo.run(1, time::seconds(1)) == LimitedIo::EXCEED_OPS);

  BOOST_CHECK_EQUAL(listenerChannel->size(), 3);
  BOOST_CHECK_EQUAL(ch1->size(), 1);
  BOOST_CHECK_EQUAL(ch2->size(), 1);
  BOOST_CHECK_EQUAL(ch3->size(), 1);
  BOOST_CHECK_EQUAL(clientFaces.size(), 4);
  BOOST_CHECK_EQUAL(clientFaces.at(2), clientFaces.at(3));
}

BOOST_AUTO_TEST_CASE(FaceClosure)
{
  this->listen(ip::address_v4::loopback());

  auto clientChannel = makeChannel(ip::address_v4());
  this->connect(*clientChannel);

  BOOST_CHECK(limitedIo.run(2, time::seconds(1)) == LimitedIo::EXCEED_OPS);

  BOOST_CHECK_EQUAL(listenerChannel->size(), 1);
  BOOST_CHECK_EQUAL(clientChannel->size(), 1);

  clientFaces.at(0)->close();

  BOOST_CHECK(limitedIo.run(2, time::seconds(5)) == LimitedIo::EXCEED_OPS);

  BOOST_CHECK_EQUAL(listenerChannel->size(), 0);
  BOOST_CHECK_EQUAL(clientChannel->size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // TestUdpChannel
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace nfd
