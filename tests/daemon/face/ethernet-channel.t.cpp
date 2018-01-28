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

#include "face/ethernet-channel.hpp"

#include "ethernet-fixture.hpp"

namespace nfd {
namespace face {
namespace tests {

class EthernetChannelFixture : public EthernetFixture
{
protected:
  unique_ptr<EthernetChannel>
  makeChannel()
  {
    BOOST_ASSERT(netifs.size() > 0);
    return make_unique<EthernetChannel>(netifs.front(), time::seconds(2));
  }
};

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestEthernetChannel, EthernetChannelFixture)

BOOST_AUTO_TEST_CASE(Uri)
{
  SKIP_IF_ETHERNET_NETIF_COUNT_LT(1);

  auto channel = makeChannel();
  BOOST_CHECK_EQUAL(channel->getUri(), FaceUri::fromDev(netifs.front()->getName()));
}

BOOST_AUTO_TEST_CASE(Listen)
{
  SKIP_IF_ETHERNET_NETIF_COUNT_LT(1);

  auto channel = makeChannel();
  BOOST_CHECK_EQUAL(channel->isListening(), false);

  channel->listen(nullptr, nullptr);
  BOOST_CHECK_EQUAL(channel->isListening(), true);

  // listen() is idempotent
  BOOST_CHECK_NO_THROW(channel->listen(nullptr, nullptr));
  BOOST_CHECK_EQUAL(channel->isListening(), true);
}

BOOST_AUTO_TEST_CASE(FaceClosure)
{
  SKIP_IF_ETHERNET_NETIF_COUNT_LT(1);

  auto channel = makeChannel();
  BOOST_CHECK_EQUAL(channel->size(), 0);

  shared_ptr<nfd::Face> face;
  channel->connect({0x00, 0x00, 0x5e, 0x00, 0x53, 0x5e}, {},
                   [&face] (const shared_ptr<nfd::Face>& newFace) {
                     BOOST_REQUIRE(newFace != nullptr);
                     face = newFace;
                   },
                   [] (uint32_t status, const std::string& reason) {
                     BOOST_FAIL("No error expected, but got: [" << status << ": " << reason << "]");
                   });
  BOOST_CHECK_EQUAL(channel->size(), 1);
  BOOST_REQUIRE(face != nullptr);

  face->close();
  g_io.poll();
  BOOST_CHECK_EQUAL(channel->size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // TestEthernetChannel
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
