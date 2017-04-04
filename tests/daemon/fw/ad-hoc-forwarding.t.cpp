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

/** \file
 *  This test suite checks that forwarding can relay Interest and Data via ad hoc face.
 */

// Strategies that can forward Interest to an ad hoc face even if it's the downstream,
// sorted alphabetically.
#include "fw/asf-strategy.hpp"
#include "fw/best-route-strategy2.hpp"
#include "fw/multicast-strategy.hpp"

#include "tests/test-common.hpp"
#include "topology-tester.hpp"
#include <boost/mpl/vector.hpp>

namespace nfd {
namespace fw {
namespace tests {

using namespace nfd::tests;

template<typename S>
class AdHocForwardingFixture : public UnitTestTimeFixture
{
protected:
  AdHocForwardingFixture()
  {
    nodeA = topo.addForwarder("A");
    nodeB = topo.addForwarder("B");
    nodeC = topo.addForwarder("C");

    for (TopologyNode node : {nodeA, nodeB, nodeC}) {
      topo.setStrategy<S>(node);
    }

    auto wireless = topo.addLink("ABC", time::milliseconds(10), {nodeA, nodeB, nodeC},
                                 ndn::nfd::LINK_TYPE_AD_HOC);
    wireless->block(nodeA, nodeC);
    wireless->block(nodeC, nodeA);
    faceA = &wireless->getFace(nodeA);
    faceB = &wireless->getFace(nodeB);
    faceC = &wireless->getFace(nodeC);

    appA = topo.addAppFace("consumer", nodeA);
    topo.registerPrefix(nodeA, *faceA, "/P");
    appC = topo.addAppFace("producer", nodeC, "/P");
    topo.addEchoProducer(appC->getClientFace(), "/P");
  }

protected:
  TopologyTester topo;
  TopologyNode nodeA;
  TopologyNode nodeB;
  TopologyNode nodeC;
  Face* faceA;
  Face* faceB;
  Face* faceC;
  shared_ptr<TopologyAppLink> appA;
  shared_ptr<TopologyAppLink> appC;
};

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_FIXTURE_TEST_SUITE(TestAdHocForwarding, BaseFixture)

using Strategies = boost::mpl::vector<
  AsfStrategy,
  BestRouteStrategy2,
  MulticastStrategy
>;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(SingleNexthop, S, Strategies,
                                 AdHocForwardingFixture<S>)
{
  // +---+---+
  // A   B   C
  //
  // A is the consumer. C is the producer.
  // B should relay Interest/Data between A and C.

  this->topo.registerPrefix(this->nodeB, *this->faceB, "/P");
  this->topo.addIntervalConsumer(this->appA->getClientFace(), "/P", time::milliseconds(100), 10);
  this->advanceClocks(time::milliseconds(5), time::milliseconds(1200));

  // Consumer should receive Data, and B should be relaying.
  BOOST_CHECK_EQUAL(this->faceB->getCounters().nInInterests, 10);
  BOOST_CHECK_EQUAL(this->faceB->getCounters().nOutInterests, 10);
  BOOST_CHECK_EQUAL(this->appC->getForwarderFace().getCounters().nOutInterests, 10);
  BOOST_CHECK_EQUAL(this->faceB->getCounters().nInData, 10);
  BOOST_CHECK_EQUAL(this->faceB->getCounters().nOutData, 10);
  BOOST_CHECK_EQUAL(this->appA->getForwarderFace().getCounters().nOutData, 10);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(SecondNexthop, S, Strategies,
                                 AdHocForwardingFixture<S>)
{
  // +---+---+
  // A   B   C
  //     |
  //     D
  //
  // A is the consumer. C is the producer.
  // B's first nexthop is D, but B-D link has failed, so B should relay Interest/Data between A and C.

  TopologyNode nodeD = this->topo.addForwarder("D");
  shared_ptr<TopologyLink> linkBD = this->topo.addLink("BD", time::milliseconds(5), {this->nodeB, nodeD});
  this->topo.registerPrefix(this->nodeB, linkBD->getFace(this->nodeB), "/P", 5);
  linkBD->fail();
  this->topo.registerPrefix(this->nodeB, *this->faceB, "/P", 10);

  // Two interval consumers are expressing Interests with same name 40ms apart,
  // so that Interests from the second interval consumer are considered retransmission.
  this->topo.addIntervalConsumer(this->appA->getClientFace(), "/P", time::milliseconds(100), 50, 1);
  this->advanceClocks(time::milliseconds(5), time::milliseconds(40));
  this->topo.addIntervalConsumer(this->appA->getClientFace(), "/P", time::milliseconds(100), 50, 1);
  this->advanceClocks(time::milliseconds(5), time::milliseconds(5400));

  // Consumer should receive Data, and B should be relaying at least some Interest/Data.
  BOOST_CHECK_GE(this->faceB->getCounters().nInInterests, 50);
  BOOST_CHECK_GE(this->faceB->getCounters().nOutInterests, 25);
  BOOST_CHECK_GE(this->appC->getForwarderFace().getCounters().nOutInterests, 25);
  BOOST_CHECK_GE(this->faceB->getCounters().nInData, 25);
  BOOST_CHECK_GE(this->faceB->getCounters().nOutData, 25);
  BOOST_CHECK_GE(this->appA->getForwarderFace().getCounters().nOutData, 25);
}

BOOST_AUTO_TEST_SUITE_END() // TestAdHocForwarding
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace fw
} // namespace nfd
