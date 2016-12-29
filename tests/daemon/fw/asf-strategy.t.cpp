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

#include "fw/asf-strategy.hpp"

#include "tests/test-common.hpp"
#include "strategy-tester.hpp"
#include "topology-tester.hpp"

namespace nfd {
namespace fw {
namespace asf {
namespace tests {

using namespace nfd::fw::tests;

// The tester is unused in this file, but it's used in various templated test suites.
typedef StrategyTester<AsfStrategy> AsfStrategyTester;
NFD_REGISTER_STRATEGY(AsfStrategyTester);

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_FIXTURE_TEST_SUITE(TestAsfStrategy, UnitTestTimeFixture)

class AsfGridFixture : public UnitTestTimeFixture
{
protected:
  AsfGridFixture()
  {
    /*
     *                  +---------+
     *           +----->|  nodeB  |<------+
     *           |      +---------+       |
     *      10ms |                        | 10ms
     *           v                        v
     *      +---------+              +---------+
     *      |  nodeA  |              |  nodeC  |
     *      +---------+              +---------+
     *           ^                        ^
     *     100ms |                        | 100ms
     *           |      +---------+       |
     *           +----->|  nodeD  |<------+
     *                  +---------+
     */

    nodeA = topo.addForwarder("A");
    nodeB = topo.addForwarder("B");
    nodeC = topo.addForwarder("C");
    nodeD = topo.addForwarder("D");

    topo.setStrategy<fw::AsfStrategy>(nodeA);
    topo.setStrategy<fw::AsfStrategy>(nodeB);
    topo.setStrategy<fw::AsfStrategy>(nodeC);
    topo.setStrategy<fw::AsfStrategy>(nodeD);

    linkAB = topo.addLink("AB", time::milliseconds(10), {nodeA, nodeB});
    linkAD = topo.addLink("AD", time::milliseconds(100), {nodeA, nodeD});
    linkBC = topo.addLink("BC", time::milliseconds(10), {nodeB, nodeC});
    linkCD = topo.addLink("CD", time::milliseconds(100), {nodeC, nodeD});

    consumer = topo.addAppFace("c", nodeA);
    producer = topo.addAppFace("p", nodeC, PRODUCER_PREFIX);
    topo.addEchoProducer(producer->getClientFace());

    // Register producer prefix on consumer node
    topo.registerPrefix(nodeA, linkAB->getFace(nodeA), PRODUCER_PREFIX, 10);
    topo.registerPrefix(nodeA, linkAD->getFace(nodeA), PRODUCER_PREFIX, 5);
  }

  void
  runConsumer()
  {
    topo.addIntervalConsumer(consumer->getClientFace(), PRODUCER_PREFIX, time::seconds(1), 30);
    this->advanceClocks(time::milliseconds(10), time::seconds(30));
  }

protected:
  TopologyTester topo;

  TopologyNode nodeA;
  TopologyNode nodeB;
  TopologyNode nodeC;
  TopologyNode nodeD;

  shared_ptr<TopologyLink> linkAB;
  shared_ptr<TopologyLink> linkAD;
  shared_ptr<TopologyLink> linkBC;
  shared_ptr<TopologyLink> linkCD;

  shared_ptr<TopologyAppLink> consumer;
  shared_ptr<TopologyAppLink> producer;

  static const Name PRODUCER_PREFIX;
};

const Name AsfGridFixture::PRODUCER_PREFIX = Name("ndn:/hr/C");

BOOST_FIXTURE_TEST_CASE(Basic, AsfGridFixture)
{
  // Both nodeB and nodeD have FIB entries to reach the producer
  topo.registerPrefix(nodeB, linkBC->getFace(nodeB), PRODUCER_PREFIX);
  topo.registerPrefix(nodeD, linkCD->getFace(nodeD), PRODUCER_PREFIX);

  runConsumer();

  // ASF should use the Face to nodeD because it has lower routing cost.
  // After 5 seconds, a probe Interest should be sent to the Face to nodeB,
  // and the probe should return Data quicker. ASF should then use the Face
  // to nodeB to forward the remaining Interests.
  BOOST_CHECK_EQUAL(consumer->getForwarderFace().getCounters().nOutData, 30);
  BOOST_CHECK_GE(linkAB->getFace(nodeA).getCounters().nOutInterests, 24);
  BOOST_CHECK_LE(linkAD->getFace(nodeA).getCounters().nOutInterests, 6);

  // If the link from nodeA to nodeB fails, ASF should start using the Face
  // to nodeD again.
  linkAB->fail();

  runConsumer();

  // Only 59 Data because the first Interest to nodeB after the failure should timeout
  BOOST_CHECK_EQUAL(consumer->getForwarderFace().getCounters().nOutData, 59);
  BOOST_CHECK_LE(linkAB->getFace(nodeA).getCounters().nOutInterests, 30);
  BOOST_CHECK_GE(linkAD->getFace(nodeA).getCounters().nOutInterests, 30);

  // If the link from nodeA to nodeB recovers, ASF should probe the Face
  // to nodeB and start using it again.
  linkAB->recover();

  // Advance time to ensure probing is due
  this->advanceClocks(time::milliseconds(10), time::seconds(10));

  runConsumer();

  BOOST_CHECK_EQUAL(consumer->getForwarderFace().getCounters().nOutData, 89);
  BOOST_CHECK_GE(linkAB->getFace(nodeA).getCounters().nOutInterests, 50);
  BOOST_CHECK_LE(linkAD->getFace(nodeA).getCounters().nOutInterests, 40);

  // If both links fail, nodeA should forward to the next hop with the lowest cost
  linkAB->fail();
  linkAD->fail();

  runConsumer();

  BOOST_CHECK_EQUAL(consumer->getForwarderFace().getCounters().nOutData, 89);
  BOOST_CHECK_LE(linkAB->getFace(nodeA).getCounters().nOutInterests, 61); // FIXME #3830
  BOOST_CHECK_GE(linkAD->getFace(nodeA).getCounters().nOutInterests, 59); // FIXME #3830
}

BOOST_FIXTURE_TEST_CASE(Nack, AsfGridFixture)
{
  // nodeB has a FIB entry to reach the producer, but nodeD does not
  topo.registerPrefix(nodeB, linkBC->getFace(nodeB), PRODUCER_PREFIX);

  // The strategy should first try to send to nodeD. But since nodeD does not have a route for
  // the producer's prefix, it should return a NO_ROUTE Nack. The strategy should then start using the Face to
  // nodeB.
  runConsumer();

  BOOST_CHECK_GE(linkAD->getFace(nodeA).getCounters().nInNacks, 1);
  BOOST_CHECK_EQUAL(consumer->getForwarderFace().getCounters().nOutData, 29);
  BOOST_CHECK_EQUAL(linkAB->getFace(nodeA).getCounters().nOutInterests, 29);

  // nodeD should receive 2 Interests: one for the very first Interest and
  // another from a probe
  BOOST_CHECK_GE(linkAD->getFace(nodeA).getCounters().nOutInterests, 2);
}

BOOST_AUTO_TEST_CASE(NoPitOutRecordAndProbeInterestNewNonce)
{
  /*                 +---------+
  *                  |  nodeD  |
  *                  +---------+
  *                       |
  *                       | 80ms
  *                       |
  *                       |
  *                  +---------+
  *           +----->|  nodeB  |<------+
  *           |      +---------+       |
  *      15ms |                        | 16ms
  *           v                        v
  *      +---------+              +---------+
  *      |  nodeA  |--------------|  nodeC  |
  *      +---------+     14ms      +---------+
  */

  const Name PRODUCER_PREFIX = "/ndn/edu/nodeD/ping";

  TopologyTester topo;
  TopologyNode nodeA = topo.addForwarder("A"),
               nodeB = topo.addForwarder("B"),
               nodeC = topo.addForwarder("C"),
               nodeD = topo.addForwarder("D");

  for (TopologyNode node : {nodeA, nodeB, nodeC, nodeD}) {
    topo.setStrategy<fw::AsfStrategy>(node);
  }

  shared_ptr<TopologyLink> linkAB = topo.addLink("AB", time::milliseconds(15), {nodeA, nodeB}),
                           linkAC = topo.addLink("AC", time::milliseconds(14), {nodeA, nodeC}),
                           linkBC = topo.addLink("BC", time::milliseconds(16), {nodeB, nodeC}),
                           linkBD = topo.addLink("BD", time::milliseconds(80), {nodeB, nodeD});

  shared_ptr<TopologyAppLink> ping = topo.addAppFace("c", nodeA),
                        pingServer = topo.addAppFace("p", nodeD, PRODUCER_PREFIX);
  topo.addEchoProducer(pingServer->getClientFace());

  // Register prefixes
  topo.registerPrefix(nodeA, linkAB->getFace(nodeA), PRODUCER_PREFIX, 15);
  topo.registerPrefix(nodeA, linkAC->getFace(nodeA), PRODUCER_PREFIX, 14);
  topo.registerPrefix(nodeC, linkBC->getFace(nodeC), PRODUCER_PREFIX, 16);
  topo.registerPrefix(nodeB, linkBD->getFace(nodeB), PRODUCER_PREFIX, 80);

  uint32_t nonce;

  // Send 6 interest since probes can be scheduled b/w 0-5 seconds
  for (int i = 1; i < 7; i++) {
    // Send ping number i
    Name name(PRODUCER_PREFIX);
    name.appendTimestamp();
    shared_ptr<Interest> interest = makeInterest(name);
    ping->getClientFace().expressInterest(*interest, nullptr, nullptr, nullptr);
    nonce = interest->getNonce();

    // Don't know when the probe will be triggered since it is random between 0-5 seconds
    // or whether it will be triggered for this interest
    int j = 1;
    while (linkAB->getFace(nodeA).getCounters().nOutInterests != 1) {
      this->advanceClocks(time::milliseconds(1));
      ++j;
      // Probe was not scheduled with this ping interest
      if (j > 1000) {
        break;
      }
    }

    // Check if probe is sent to B else send another ping
    if (linkAB->getFace(nodeA).getCounters().nOutInterests == 1) {
      // Get pitEntry of node A
      shared_ptr<pit::Entry> pitEntry = topo.getForwarder(nodeA).getPit().find(*interest);
      //get outRecord associated with face towards B
      pit::OutRecordCollection::const_iterator outRecord = pitEntry->getOutRecord(linkAB->getFace(nodeA));

      BOOST_CHECK(outRecord != pitEntry->out_end());

      //Check that Nonce of interest is not equal to Nonce of Probe
      BOOST_CHECK_NE(nonce, outRecord->getLastNonce());

      // B should not have received the probe interest yet
      BOOST_CHECK_EQUAL(linkAB->getFace(nodeB).getCounters().nInInterests, 0);

      // i-1 interests through B when no probe
      BOOST_CHECK_EQUAL(linkBD->getFace(nodeB).getCounters().nOutInterests, i - 1);

      // After 15ms, B should get the probe interest
      this->advanceClocks(time::milliseconds(1), time::milliseconds(15));
      BOOST_CHECK_EQUAL(linkAB->getFace(nodeB).getCounters().nInInterests, 1);
      BOOST_CHECK_EQUAL(linkBD->getFace(nodeB).getCounters().nOutInterests, i);

      pitEntry = topo.getForwarder(nodeB).getPit().find(*interest);

      // Get outRecord associated with face towards D.
      outRecord = pitEntry->getOutRecord(linkBD->getFace(nodeB));

      BOOST_CHECK(outRecord != pitEntry->out_end());

      // RTT between B and D
      this->advanceClocks(time::milliseconds(5), time::milliseconds(160));
      outRecord = pitEntry->getOutRecord(linkBD->getFace(nodeB));

      BOOST_CHECK_EQUAL(linkBD->getFace(nodeB).getCounters().nInData, i);

      BOOST_CHECK(outRecord == pitEntry->out_end());

      // Data is returned for the ping after 15 ms - will result in false measurement
      // 14+16-15 = 15ms
      // Since outRecord == pitEntry->out_end()
      this->advanceClocks(time::milliseconds(1), time::milliseconds(15));
      BOOST_CHECK_EQUAL(linkBD->getFace(nodeB).getCounters().nInData, i+1);

      break;
    }
  }
}

BOOST_AUTO_TEST_SUITE_END() // TestAsfStrategy
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace asf
} // namespace fw
} // namespace nfd
