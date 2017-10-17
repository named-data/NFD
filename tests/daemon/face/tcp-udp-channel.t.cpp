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

#include "tcp-channel-fixture.hpp"
#include "udp-channel-fixture.hpp"

#include "test-ip.hpp"
#include <boost/mpl/vector.hpp>

namespace nfd {
namespace face {
namespace tests {

BOOST_AUTO_TEST_SUITE(Face)
BOOST_AUTO_TEST_SUITE(TestTcpUdpChannel)

template<typename F, AddressFamily AF>
struct FixtureAndAddress
{
  using Fixture = F;
  static constexpr AddressFamily ADDRESS_FAMILY = AF;
  using Address = typename IpAddressFromFamily<AF>::type;
};

using FixtureAndAddressList = boost::mpl::vector<
  FixtureAndAddress<TcpChannelFixture, AddressFamily::V4>,
  FixtureAndAddress<TcpChannelFixture, AddressFamily::V6>,
  FixtureAndAddress<UdpChannelFixture, AddressFamily::V4>,
  FixtureAndAddress<UdpChannelFixture, AddressFamily::V6>
>;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(Uri, T, FixtureAndAddressList, T::Fixture)
{
  decltype(this->listenerEp) ep(T::Address::loopback(), 7050);
  auto channel = this->makeChannel(ep.address(), ep.port());
  BOOST_CHECK_EQUAL(channel->getUri(), FaceUri(ep));
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(Listen, T, FixtureAndAddressList, T::Fixture)
{
  auto channel = this->makeChannel(typename T::Address());
  BOOST_CHECK_EQUAL(channel->isListening(), false);

  channel->listen(nullptr, nullptr);
  BOOST_CHECK_EQUAL(channel->isListening(), true);

  // listen() is idempotent
  BOOST_CHECK_NO_THROW(channel->listen(nullptr, nullptr));
  BOOST_CHECK_EQUAL(channel->isListening(), true);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(MultipleAccepts, T, FixtureAndAddressList, T::Fixture)
{
  auto address = getTestIp(T::ADDRESS_FAMILY, AddressScope::Loopback);
  SKIP_IF_IP_UNAVAILABLE(address);
  this->listen(address);

  BOOST_CHECK_EQUAL(this->listenerChannel->isListening(), true);
  BOOST_CHECK_EQUAL(this->listenerChannel->size(), 0);

  auto ch1 = this->makeChannel(typename T::Address());
  this->connect(*ch1);

  BOOST_CHECK_EQUAL(this->limitedIo.run(2, time::seconds(1)), LimitedIo::EXCEED_OPS);

  BOOST_CHECK_EQUAL(this->listenerChannel->size(), 1);
  BOOST_CHECK_EQUAL(ch1->size(), 1);
  BOOST_CHECK_EQUAL(ch1->isListening(), false);

  auto ch2 = this->makeChannel(typename T::Address());
  auto ch3 = this->makeChannel(typename T::Address());
  this->connect(*ch2);
  this->connect(*ch3);

  BOOST_CHECK_EQUAL(this->limitedIo.run(4, time::seconds(2)), LimitedIo::EXCEED_OPS);

  BOOST_CHECK_EQUAL(this->listenerChannel->size(), 3);
  BOOST_CHECK_EQUAL(ch1->size(), 1);
  BOOST_CHECK_EQUAL(ch2->size(), 1);
  BOOST_CHECK_EQUAL(ch3->size(), 1);
  BOOST_CHECK_EQUAL(this->clientFaces.size(), 3);

  // check face persistency
  for (const auto& face : this->listenerFaces) {
    BOOST_CHECK_EQUAL(face->getPersistency(), ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  }
  for (const auto& face : this->clientFaces) {
    BOOST_CHECK_EQUAL(face->getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  }

  // connect twice to the same endpoint
  this->connect(*ch3);

  BOOST_CHECK_EQUAL(this->limitedIo.run(1, time::seconds(1)), LimitedIo::EXCEED_OPS);

  BOOST_CHECK_EQUAL(this->listenerChannel->size(), 3);
  BOOST_CHECK_EQUAL(ch1->size(), 1);
  BOOST_CHECK_EQUAL(ch2->size(), 1);
  BOOST_CHECK_EQUAL(ch3->size(), 1);
  BOOST_CHECK_EQUAL(this->clientFaces.size(), 4);
  BOOST_CHECK_EQUAL(this->clientFaces.at(2), this->clientFaces.at(3));
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(FaceClosure, T, FixtureAndAddressList, T::Fixture)
{
  auto address = getTestIp(T::ADDRESS_FAMILY, AddressScope::Loopback);
  SKIP_IF_IP_UNAVAILABLE(address);
  this->listen(address);

  auto clientChannel = this->makeChannel(typename T::Address());
  this->connect(*clientChannel);

  BOOST_CHECK_EQUAL(this->limitedIo.run(2, time::seconds(1)), LimitedIo::EXCEED_OPS);

  BOOST_CHECK_EQUAL(this->listenerChannel->size(), 1);
  BOOST_CHECK_EQUAL(clientChannel->size(), 1);

  this->clientFaces.at(0)->close();

  BOOST_CHECK_EQUAL(this->limitedIo.run(2, time::seconds(5)), LimitedIo::EXCEED_OPS);

  BOOST_CHECK_EQUAL(this->listenerChannel->size(), 0);
  BOOST_CHECK_EQUAL(clientChannel->size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // TestTcpUdpChannel
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
