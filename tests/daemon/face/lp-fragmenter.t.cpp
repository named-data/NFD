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

#include "face/lp-fragmenter.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace face {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Face)

class LpFragmenterFixture
{
public:
  LpFragmenter fragmenter;
};

BOOST_FIXTURE_TEST_SUITE(TestLpFragmenter, LpFragmenterFixture)

BOOST_AUTO_TEST_CASE(FragmentSingleFragment)
{
  size_t mtu = 256;

  lp::Packet packet;
  packet.add<lp::IncomingFaceIdField>(123);

  shared_ptr<Data> data = makeData("/test/data1");
  BOOST_REQUIRE_EQUAL(data->wireEncode().size(), 30);
  packet.add<lp::FragmentField>(std::make_pair(data->wireEncode().begin(),
                                               data->wireEncode().end()));

  bool isOk = false;
  std::vector<lp::Packet> frags;
  std::tie(isOk, frags) = fragmenter.fragmentPacket(packet, mtu);

  BOOST_REQUIRE(isOk);
  BOOST_REQUIRE_EQUAL(frags.size(), 1);
  BOOST_CHECK(frags[0].has<lp::FragmentField>());
  BOOST_CHECK_EQUAL(frags[0].get<lp::IncomingFaceIdField>(), 123);
  BOOST_CHECK(!frags[0].has<lp::FragIndexField>());
  BOOST_CHECK(!frags[0].has<lp::FragCountField>());
  BOOST_CHECK_LE(frags[0].wireEncode().size(), mtu);

  ndn::Buffer::const_iterator fragBegin, fragEnd;
  std::tie(fragBegin, fragEnd) = frags[0].get<lp::FragmentField>();
  BOOST_CHECK_EQUAL_COLLECTIONS(data->wireEncode().begin(), data->wireEncode().end(),
                                fragBegin, fragEnd);
}

BOOST_AUTO_TEST_CASE(FragmentMultipleFragments)
{
  size_t mtu = 90;

  lp::Packet packet;
  packet.add<lp::IncomingFaceIdField>(123);

  shared_ptr<Data> data = makeData("/test/data1/123456789/987654321/123456789");
  BOOST_REQUIRE_EQUAL(data->wireEncode().size(), 63);
  packet.add<lp::FragmentField>(std::make_pair(data->wireEncode().begin(),
                                               data->wireEncode().end()));

  bool isOk = false;
  std::vector<lp::Packet> frags;
  std::tie(isOk, frags) = fragmenter.fragmentPacket(packet, mtu);

  BOOST_REQUIRE(isOk);
  BOOST_REQUIRE_EQUAL(frags.size(), 2);

  ndn::Buffer reassembledPayload(63);

  BOOST_CHECK(frags[0].has<lp::FragmentField>());
  BOOST_CHECK_EQUAL(frags[0].get<lp::IncomingFaceIdField>(), 123);
  BOOST_CHECK_EQUAL(frags[0].get<lp::FragIndexField>(), 0);
  BOOST_CHECK_EQUAL(frags[0].get<lp::FragCountField>(), 2);
  BOOST_CHECK_LE(frags[0].wireEncode().size(), mtu);
  ndn::Buffer::const_iterator frag0Begin, frag0End;
  std::tie(frag0Begin, frag0End) = frags[0].get<lp::FragmentField>();
  BOOST_REQUIRE_LE(std::distance(frag0Begin, frag0End), reassembledPayload.size());
  auto reassembledPos = std::copy(frag0Begin, frag0End, reassembledPayload.begin());

  BOOST_CHECK(frags[1].has<lp::FragmentField>());
  BOOST_CHECK(!frags[1].has<lp::IncomingFaceIdField>());
  BOOST_CHECK_EQUAL(frags[1].get<lp::FragIndexField>(), 1);
  BOOST_CHECK_EQUAL(frags[1].get<lp::FragCountField>(), 2);
  BOOST_CHECK_LE(frags[1].wireEncode().size(), mtu);
  ndn::Buffer::const_iterator frag1Begin, frag1End;
  std::tie(frag1Begin, frag1End) = frags[1].get<lp::FragmentField>();
  BOOST_REQUIRE_LE(std::distance(frag1Begin, frag1End),
                   std::distance(reassembledPos, reassembledPayload.end()));
  std::copy(frag1Begin, frag1End, reassembledPos);

  BOOST_CHECK_EQUAL_COLLECTIONS(data->wireEncode().begin(), data->wireEncode().end(),
                                reassembledPayload.begin(), reassembledPayload.end());
}

BOOST_AUTO_TEST_CASE(FragmentMtuTooSmall)
{
  size_t mtu = 20;

  lp::Packet packet;
  packet.add<lp::IncomingFaceIdField>(123);

  shared_ptr<Data> data = makeData("/test/data1/123456789/987654321/123456789");
  packet.add<lp::FragmentField>(std::make_pair(data->wireEncode().begin(),
                                               data->wireEncode().end()));

  bool isOk = false;
  std::tie(isOk, std::ignore) = fragmenter.fragmentPacket(packet, mtu);
  BOOST_REQUIRE(!isOk);
}

BOOST_AUTO_TEST_CASE(FragmentOverFragCount)
{
  LpFragmenter::Options options;
  options.nMaxFragments = 2;
  fragmenter.setOptions(options);

  size_t mtu = 70;

  lp::Packet packet;
  packet.add<lp::IncomingFaceIdField>(123);

  shared_ptr<Data> data = makeData("/test/data1/123456789/987654321/123456789");
  packet.add<lp::FragmentField>(std::make_pair(data->wireEncode().begin(),
                                               data->wireEncode().end()));

  bool isOk = false;
  std::tie(isOk, std::ignore) = fragmenter.fragmentPacket(packet, mtu);
  BOOST_REQUIRE(!isOk);
}

BOOST_AUTO_TEST_SUITE_END() // TestLpFragmentation
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
