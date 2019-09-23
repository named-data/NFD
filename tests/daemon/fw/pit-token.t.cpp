/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#include "tests/daemon/global-io-fixture.hpp"
#include "topology-tester.hpp"

#include <ndn-cxx/lp/packet.hpp>
#include <ndn-cxx/lp/pit-token.hpp>

namespace nfd {
namespace fw {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_AUTO_TEST_SUITE(TestPitToken)

// Downstream requires PIT token.
BOOST_FIXTURE_TEST_CASE(Downstream, GlobalIoTimeFixture)
{
  TopologyTester topo;
  TopologyNode nodeR = topo.addForwarder("R");
  auto linkC = topo.addBareLink("C", nodeR, ndn::nfd::FACE_SCOPE_NON_LOCAL);
  auto linkS = topo.addBareLink("S", nodeR, ndn::nfd::FACE_SCOPE_NON_LOCAL);
  topo.registerPrefix(nodeR, linkS->getForwarderFace(), "/U", 5);
  // Client --- Router --- Server
  // Client requires PIT token; Router supports PIT token; Server disallows PIT token.

  // C sends Interest /U/0 with PIT token
  lp::Packet lppI("6414 pit-token=6206A0A1A2A3A4A5 payload=500A interest=0508 0706080155080130"_block);
  lp::PitToken tokenI(lppI.get<lp::PitTokenField>());
  linkC->receivePacket(lppI.wireEncode());
  advanceClocks(5_ms, 30_ms);

  // S should receive Interest without PIT token
  BOOST_REQUIRE_EQUAL(linkS->sentPackets.size(), 1);
  lp::Packet lppS(linkS->sentPackets.front());
  BOOST_CHECK_EQUAL(lppS.count<lp::PitTokenField>(), 0);

  // S responds Data
  linkS->receivePacket(makeData("/U/0")->wireEncode());
  advanceClocks(5_ms, 30_ms);

  // C should receive Data with same PIT token
  BOOST_REQUIRE_EQUAL(linkC->sentPackets.size(), 1);
  lp::Packet lppD(linkC->sentPackets.front());
  lp::PitToken tokenD(lppD.get<lp::PitTokenField>());
  BOOST_CHECK(tokenD == tokenI);
}

BOOST_AUTO_TEST_SUITE_END() // TestPitToken
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace fw
} // namespace nfd
