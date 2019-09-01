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

// Strategies that can correctly handle multi-access faces on a broadcast medium,
// sorted alphabetically.
#include "fw/asf-strategy.hpp"
#include "fw/best-route-strategy2.hpp"
#include "fw/multicast-strategy.hpp"
#include "fw/random-strategy.hpp"

#include "tests/daemon/global-io-fixture.hpp"
#include "topology-tester.hpp"

#include <boost/mpl/vector.hpp>

namespace nfd {
namespace fw {
namespace tests {

template<typename S>
class BroadcastMediumFixture : public GlobalIoTimeFixture
{
protected:
  BroadcastMediumFixture()
  {
    nodeC = topo.addForwarder("C");
    nodeD = topo.addForwarder("D");
    nodeP = topo.addForwarder("P");

    for (TopologyNode node : {nodeC, nodeD, nodeP}) {
      topo.setStrategy<S>(node);
    }

    auto w = topo.addLink("W", 10_ms, {nodeC, nodeD, nodeP}, ndn::nfd::LINK_TYPE_MULTI_ACCESS);
    faceC = &w->getFace(nodeC);
    faceD = &w->getFace(nodeD);
    faceP = &w->getFace(nodeP);

    appC = topo.addAppFace("consumer1", nodeC);
    topo.registerPrefix(nodeC, *faceC, "/P");
    topo.addIntervalConsumer(appC->getClientFace(), "/P", 0_ms, 1, 1);
    appD = topo.addAppFace("consumer2", nodeD);
    topo.registerPrefix(nodeD, *faceD, "/P");
    topo.addIntervalConsumer(appD->getClientFace(), "/P", 0_ms, 1, 1);
    appP = topo.addAppFace("producer", nodeP, "/P");
    topo.addEchoProducer(appP->getClientFace(), "/P", 100_ms);
  }

protected:
  TopologyTester topo;
  TopologyNode nodeC;
  TopologyNode nodeD;
  TopologyNode nodeP;
  Face* faceC;
  Face* faceD;
  Face* faceP;
  shared_ptr<TopologyAppLink> appC;
  shared_ptr<TopologyAppLink> appD;
  shared_ptr<TopologyAppLink> appP;
};

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_AUTO_TEST_SUITE(TestBroadcastMedium)

using Strategies = boost::mpl::vector<
  AsfStrategy,
  BestRouteStrategy2,
  MulticastStrategy,
  RandomStrategy
>;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(SameFaceDifferentEndpoint, S, Strategies, BroadcastMediumFixture<S>)
{
  //   C   D   P
  //   |   |   |
  // --+---+---+--
  //
  // C and D are consumers. P is the producer. They communicate with each other
  // through a multi-access face over a (wired or wireless) broadcast medium.

  this->advanceClocks(10_ms, 50_ms);

  BOOST_CHECK_EQUAL(this->faceC->getCounters().nOutInterests, 1);
  BOOST_CHECK_EQUAL(this->faceD->getCounters().nOutInterests, 1);
  BOOST_CHECK_EQUAL(this->faceP->getCounters().nInInterests, 2);
  BOOST_CHECK_EQUAL(this->faceC->getCounters().nInData, 0);
  BOOST_CHECK_EQUAL(this->faceD->getCounters().nInData, 0);
  BOOST_CHECK_EQUAL(this->faceP->getCounters().nOutData, 0);

  this->advanceClocks(10_ms, 200_ms);

  BOOST_CHECK_EQUAL(this->faceC->getCounters().nInData, 1);
  BOOST_CHECK_EQUAL(this->faceD->getCounters().nInData, 1);
  BOOST_CHECK_EQUAL(this->faceP->getCounters().nOutData, 1);
}

BOOST_AUTO_TEST_SUITE_END() // TestBroadcastMedium
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace fw
} // namespace nfd
