/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2021,  Regents of the University of California,
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

#include "strategy-tester.hpp"
#include "topology-tester.hpp"

namespace nfd {
namespace fw {
namespace asf {
namespace tests {

using namespace nfd::fw::tests;

// The tester is unused in this file, but it's used in various templated test suites.
using AsfStrategyTester = StrategyTester<AsfStrategy>;
NFD_REGISTER_STRATEGY(AsfStrategyTester);

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_FIXTURE_TEST_SUITE(TestAsfStrategy, GlobalIoTimeFixture)

class AsfGridFixture : public GlobalIoTimeFixture
{
protected:
  explicit
  AsfGridFixture(const Name& params = AsfStrategy::getStrategyName(),
                 time::nanoseconds replyDelay = 0_ns)
    : parameters(params)
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

    for (auto node : {nodeA, nodeB, nodeC, nodeD}) {
      topo.setStrategy<AsfStrategy>(node, Name("/"), parameters);
    }

    linkAB = topo.addLink("AB", 10_ms, {nodeA, nodeB});
    linkAD = topo.addLink("AD", 100_ms, {nodeA, nodeD});
    linkBC = topo.addLink("BC", 10_ms, {nodeB, nodeC});
    linkCD = topo.addLink("CD", 100_ms, {nodeC, nodeD});

    consumer = topo.addAppFace("c", nodeA);
    producer = topo.addAppFace("p", nodeC, PRODUCER_PREFIX);

    topo.addEchoProducer(producer->getClientFace(), PRODUCER_PREFIX, replyDelay);

    // Register producer prefix on consumer node
    topo.registerPrefix(nodeA, linkAB->getFace(nodeA), PRODUCER_PREFIX, 10);
    topo.registerPrefix(nodeA, linkAD->getFace(nodeA), PRODUCER_PREFIX, 5);
  }

  void
  runConsumer(size_t numInterests = 30)
  {
    topo.addIntervalConsumer(consumer->getClientFace(), PRODUCER_PREFIX, 1_s, numInterests);
    this->advanceClocks(10_ms, time::seconds(numInterests));
  }

protected:
  Name parameters;
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

const Name AsfGridFixture::PRODUCER_PREFIX("/hr/C");

class AsfStrategyParametersGridFixture : public AsfGridFixture
{
protected:
  AsfStrategyParametersGridFixture()
    : AsfGridFixture(Name(AsfStrategy::getStrategyName())
                     .append("probing-interval~30000")
                     .append("max-timeouts~5"))
  {
  }
};

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
  // Because of exploration, will forward to AB and AD simultaneously at least once
  BOOST_CHECK_GE(linkAB->getFace(nodeA).getCounters().nOutInterests, 25);
  BOOST_CHECK_LE(linkAD->getFace(nodeA).getCounters().nOutInterests, 6);

  // If the link from nodeA to nodeB fails, ASF should start using the Face
  // to nodeD again.
  linkAB->fail();

  runConsumer();
  // We experience 3 timeouts and marked AB as timed out
  BOOST_CHECK_EQUAL(consumer->getForwarderFace().getCounters().nOutData, 57);
  BOOST_CHECK_LE(linkAB->getFace(nodeA).getCounters().nOutInterests, 36);
  BOOST_CHECK_GE(linkAD->getFace(nodeA).getCounters().nOutInterests, 24);

  // If the link from nodeA to nodeB recovers, ASF should probe the Face
  // to nodeB and start using it again.
  linkAB->recover();

  // Advance time to ensure probing is due
  this->advanceClocks(10_ms, 10_s);

  runConsumer();
  BOOST_CHECK_EQUAL(consumer->getForwarderFace().getCounters().nOutData, 87);
  BOOST_CHECK_GE(linkAB->getFace(nodeA).getCounters().nOutInterests, 50);
  BOOST_CHECK_LE(linkAD->getFace(nodeA).getCounters().nOutInterests, 40);

  // If both links fail, nodeA should forward to the next hop with the lowest cost
  linkAB->fail();
  linkAD->fail();

  runConsumer();

  BOOST_CHECK_EQUAL(consumer->getForwarderFace().getCounters().nOutData, 87);
  BOOST_CHECK_LE(linkAB->getFace(nodeA).getCounters().nOutInterests, 65); // FIXME #3830
  BOOST_CHECK_GE(linkAD->getFace(nodeA).getCounters().nOutInterests, 57); // FIXME #3830
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

class AsfStrategyDelayedDataFixture : public AsfGridFixture
{
protected:
  AsfStrategyDelayedDataFixture()
    : AsfGridFixture(Name(AsfStrategy::getStrategyName()), 400_ms)
  {
  }
};

BOOST_FIXTURE_TEST_CASE(InterestForwarding, AsfStrategyDelayedDataFixture)
{
  Name name(PRODUCER_PREFIX);
  name.appendTimestamp();
  auto interest = makeInterest(name);

  topo.registerPrefix(nodeB, linkBC->getFace(nodeB), PRODUCER_PREFIX);
  topo.registerPrefix(nodeD, linkCD->getFace(nodeD), PRODUCER_PREFIX);

  // The first interest should go via link AD
  consumer->getClientFace().expressInterest(*interest, nullptr, nullptr, nullptr);
  this->advanceClocks(10_ms, 100_ms);
  BOOST_CHECK_EQUAL(linkAD->getFace(nodeA).getCounters().nOutInterests, 1);

  // Second interest should go via link AB
  interest->refreshNonce();
  consumer->getClientFace().expressInterest(*interest, nullptr, nullptr, nullptr);
  this->advanceClocks(10_ms, 100_ms);
  BOOST_CHECK_EQUAL(linkAB->getFace(nodeA).getCounters().nOutInterests, 1);

  // The third interest should again go via AD, since both the face from A is already used
  // and so asf should choose the earliest used face i.e. AD
  interest->refreshNonce();
  consumer->getClientFace().expressInterest(*interest, nullptr, nullptr, nullptr);
  this->advanceClocks(10_ms, 100_ms);
  BOOST_CHECK_EQUAL(linkAD->getFace(nodeA).getCounters().nOutInterests, 2);

  this->advanceClocks(time::milliseconds(500), time::seconds(5));
  BOOST_CHECK_EQUAL(linkAD->getFace(nodeA).getCounters().nInData, 1);
  BOOST_CHECK_EQUAL(linkAB->getFace(nodeA).getCounters().nInData, 1);
  BOOST_CHECK_EQUAL(consumer->getForwarderFace().getCounters().nOutData, 1);
}

BOOST_AUTO_TEST_CASE(Retransmission) // Bug #4874
{
  // Avoid clearing pit entry for those incoming interest that have pit entry but no next hops
  /*
   *        +---------+   10ms   +---------+
   *        |  nodeB  | ------>  |  nodeC  |
   *        +---------+          +---------+
   */

  const Name PRODUCER_PREFIX = "/pnr/C";
  TopologyTester topo;

  TopologyNode nodeB = topo.addForwarder("B"),
               nodeC = topo.addForwarder("C");

  for (auto node : {nodeB, nodeC}) {
    topo.setStrategy<AsfStrategy>(node);
  }

  auto linkBC = topo.addLink("BC", time::milliseconds(10), {nodeB, nodeC});

  auto consumer = topo.addAppFace("c", nodeB),
       producer = topo.addAppFace("p", nodeC, PRODUCER_PREFIX);

  topo.addEchoProducer(producer->getClientFace(), PRODUCER_PREFIX, 100_ms);

  Name name(PRODUCER_PREFIX);
  name.appendTimestamp();
  auto interest = makeInterest(name);

  auto& pit = topo.getForwarder(nodeB).getPit();
  auto pitEntry = pit.insert(*interest).first;

  topo.getForwarder(nodeB).onOutgoingInterest(*interest, linkBC->getFace(nodeB), pitEntry);
  this->advanceClocks(time::milliseconds(100));

  interest->refreshNonce();
  consumer->getClientFace().expressInterest(*interest, nullptr, nullptr, nullptr);
  this->advanceClocks(time::milliseconds(100));

  auto outRecord = pitEntry->getOutRecord(linkBC->getFace(nodeB));
  BOOST_CHECK(outRecord != pitEntry->out_end());

  this->advanceClocks(time::milliseconds(100));
  BOOST_CHECK_EQUAL(linkBC->getFace(nodeC).getCounters().nOutData, 1);
  BOOST_CHECK_EQUAL(linkBC->getFace(nodeB).getCounters().nInData, 1);
}

BOOST_AUTO_TEST_CASE(PerUpstreamSuppression)
{
  /*
   *                          +---------+
   *                     +----|  nodeB  |----+
   *                     |    +---------+    |
   *                50ms |                   | 50ms
   *                     |                   |
   *                +---------+   50ms  +---------+
   *                |  nodeA  | <-----> |  nodeP  |
   *                +---------+         +---------+
   */

  const Name PRODUCER_PREFIX = "/suppress/me";
  TopologyTester topo;

  TopologyNode nodeA = topo.addForwarder("A"),
               nodeB = topo.addForwarder("B"),
               nodeP = topo.addForwarder("P");

  for (auto node : {nodeA, nodeB, nodeP}) {
    topo.setStrategy<AsfStrategy>(node);
  }

  auto linkAB = topo.addLink("AB", 50_ms, {nodeA, nodeB});
  auto linkAP = topo.addLink("AP", 50_ms, {nodeA, nodeP});
  auto linkBP = topo.addLink("BP", 50_ms, {nodeB, nodeP});

  auto consumer = topo.addAppFace("cons", nodeA),
       producer = topo.addAppFace("prod", nodeP, PRODUCER_PREFIX);

  topo.addEchoProducer(producer->getClientFace(), PRODUCER_PREFIX);

  topo.registerPrefix(nodeA, linkAP->getFace(nodeA), PRODUCER_PREFIX, 10);
  topo.registerPrefix(nodeA, linkAB->getFace(nodeA), PRODUCER_PREFIX, 1);
  topo.registerPrefix(nodeB, linkBP->getFace(nodeB), PRODUCER_PREFIX, 1);

  auto& faceA2B = linkAB->getFace(nodeA);
  auto& faceA2P = linkAP->getFace(nodeA);

  Name name(PRODUCER_PREFIX);
  name.appendTimestamp();
  // very short lifetime to make it expire within the initial retx suppression period (10ms)
  auto interest = makeInterest(name, false, 5_ms);

  // 1st interest should be sent to B
  consumer->getClientFace().expressInterest(*interest, nullptr, nullptr, nullptr);
  this->advanceClocks(1_ms);
  BOOST_TEST(faceA2B.getCounters().nOutInterests == 1);
  BOOST_TEST(faceA2P.getCounters().nOutInterests == 0);

  // 2nd interest should be sent to P and NOT suppressed
  interest->setInterestLifetime(100_ms);
  interest->refreshNonce();
  consumer->getClientFace().expressInterest(*interest, nullptr, nullptr, nullptr);
  this->advanceClocks(1_ms);
  BOOST_TEST(faceA2B.getCounters().nOutInterests == 1);
  BOOST_TEST(faceA2P.getCounters().nOutInterests == 1);

  this->advanceClocks(1_ms);

  // 3rd interest should be suppressed
  // without suppression, it would have been sent again to B as that's the earliest out-record
  interest->refreshNonce();
  consumer->getClientFace().expressInterest(*interest, nullptr, nullptr, nullptr);
  this->advanceClocks(1_ms);
  BOOST_TEST(faceA2B.getCounters().nOutInterests == 1);
  BOOST_TEST(faceA2P.getCounters().nOutInterests == 1);

  this->advanceClocks(2_ms); // 1st interest should expire now

  // 4th interest should be suppressed
  // without suppression, it would have been sent again to B because the out-record expired
  interest->refreshNonce();
  consumer->getClientFace().expressInterest(*interest, nullptr, nullptr, nullptr);
  this->advanceClocks(1_ms);
  BOOST_TEST(faceA2B.getCounters().nOutInterests == 1);
  BOOST_TEST(faceA2P.getCounters().nOutInterests == 1);

  this->advanceClocks(5_ms); // suppression window ends

  // 5th interest is sent to B and is outside the suppression window
  interest->refreshNonce();
  consumer->getClientFace().expressInterest(*interest, nullptr, nullptr, nullptr);
  this->advanceClocks(1_ms);
  BOOST_TEST(faceA2B.getCounters().nOutInterests == 2);
  BOOST_TEST(faceA2P.getCounters().nOutInterests == 1);

  this->advanceClocks(10_ms);

  // 6th interest is sent to P and is outside the suppression window
  interest->refreshNonce();
  consumer->getClientFace().expressInterest(*interest, nullptr, nullptr, nullptr);
  this->advanceClocks(1_ms);
  BOOST_TEST(faceA2B.getCounters().nOutInterests == 2);
  BOOST_TEST(faceA2P.getCounters().nOutInterests == 2);
}

BOOST_AUTO_TEST_CASE(NoPitOutRecordAndProbeInterestNewNonce)
{
  /*                  +---------+
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

  for (auto node : {nodeA, nodeB, nodeC, nodeD}) {
    topo.setStrategy<AsfStrategy>(node);
  }

  auto linkAB = topo.addLink("AB", 15_ms, {nodeA, nodeB}),
       linkAC = topo.addLink("AC", 14_ms, {nodeA, nodeC}),
       linkBC = topo.addLink("BC", 16_ms, {nodeB, nodeC}),
       linkBD = topo.addLink("BD", 80_ms, {nodeB, nodeD});

  auto ping = topo.addAppFace("c", nodeA);
  auto pingServer = topo.addAppFace("p", nodeD, PRODUCER_PREFIX);
  topo.addEchoProducer(pingServer->getClientFace());

  topo.registerPrefix(nodeA, linkAB->getFace(nodeA), PRODUCER_PREFIX, 15);
  topo.registerPrefix(nodeA, linkAC->getFace(nodeA), PRODUCER_PREFIX, 14);
  topo.registerPrefix(nodeC, linkBC->getFace(nodeC), PRODUCER_PREFIX, 16);
  topo.registerPrefix(nodeB, linkBD->getFace(nodeB), PRODUCER_PREFIX, 80);

  // Send 6 interest since probes can be scheduled b/w 0-5 seconds
  for (int i = 1; i < 7; i++) {
    // Send ping number i
    Name name(PRODUCER_PREFIX);
    name.appendTimestamp();
    auto interest = makeInterest(name);
    ping->getClientFace().expressInterest(*interest, nullptr, nullptr, nullptr);
    auto nonce = interest->getNonce();

    // Don't know when the probe will be triggered since it is random between 0-5 seconds
    // or whether it will be triggered for this interest
    for (int j = 1; j <= 1000 && linkAB->getFace(nodeA).getCounters().nOutInterests != 1; ++j) {
      this->advanceClocks(1_ms);
    }

    // Check if probe is sent to B else send another ping
    if (linkAB->getFace(nodeA).getCounters().nOutInterests == 1) {
      // Get pitEntry of node A
      auto pitEntry = topo.getForwarder(nodeA).getPit().find(*interest);
      // Get outRecord associated with face towards B
      auto outRecord = pitEntry->getOutRecord(linkAB->getFace(nodeA));
      BOOST_REQUIRE(outRecord != pitEntry->out_end());

      // Check that Nonce of interest is not equal to Nonce of Probe
      BOOST_CHECK_NE(nonce, outRecord->getLastNonce());

      // B should not have received the probe interest yet
      BOOST_CHECK_EQUAL(linkAB->getFace(nodeB).getCounters().nInInterests, 0);

      // i-1 interests through B when no probe
      BOOST_CHECK_EQUAL(linkBD->getFace(nodeB).getCounters().nOutInterests, i - 1);

      // After 15ms, B should get the probe interest
      this->advanceClocks(1_ms, 15_ms);
      BOOST_CHECK_EQUAL(linkAB->getFace(nodeB).getCounters().nInInterests, 1);
      BOOST_CHECK_EQUAL(linkBD->getFace(nodeB).getCounters().nOutInterests, i);

      pitEntry = topo.getForwarder(nodeB).getPit().find(*interest);

      // Get outRecord associated with face towards D.
      outRecord = pitEntry->getOutRecord(linkBD->getFace(nodeB));
      BOOST_CHECK(outRecord != pitEntry->out_end());

      // RTT between B and D
      this->advanceClocks(5_ms, 160_ms);
      outRecord = pitEntry->getOutRecord(linkBD->getFace(nodeB));

      BOOST_CHECK_EQUAL(linkBD->getFace(nodeB).getCounters().nInData, i);

      BOOST_CHECK(outRecord == pitEntry->out_end());

      // Data is returned for the ping after 15 ms - will result in false measurement
      // 14+16-15 = 15ms
      // Since outRecord == pitEntry->out_end()
      this->advanceClocks(1_ms, 15_ms);
      BOOST_CHECK_EQUAL(linkBD->getFace(nodeB).getCounters().nInData, i+1);

      break;
    }
  }
}

BOOST_FIXTURE_TEST_CASE(IgnoreTimeouts, AsfStrategyParametersGridFixture)
{
  // Both nodeB and nodeD have FIB entries to reach the producer
  topo.registerPrefix(nodeB, linkBC->getFace(nodeB), PRODUCER_PREFIX);
  topo.registerPrefix(nodeD, linkCD->getFace(nodeD), PRODUCER_PREFIX);

  // Send 15 interests let it change to use the 10 ms link
  runConsumer(15);

  uint64_t outInterestsBeforeFailure = linkAD->getFace(nodeA).getCounters().nOutInterests;

  // Bring down 10 ms link
  linkAB->fail();

  // Send 5 interests, after the last one it will record the timeout
  // ready to switch for the next interest
  runConsumer(5);

  // Check that link has not been switched to 100 ms because max-timeouts = 5
  BOOST_CHECK_EQUAL(linkAD->getFace(nodeA).getCounters().nOutInterests, outInterestsBeforeFailure);

  // Send 5 interests, check that 100 ms link is used
  runConsumer(5);

  BOOST_CHECK_EQUAL(linkAD->getFace(nodeA).getCounters().nOutInterests, outInterestsBeforeFailure + 5);
}

BOOST_FIXTURE_TEST_CASE(ProbingInterval, AsfStrategyParametersGridFixture)
{
  // Both nodeB and nodeD have FIB entries to reach the producer
  topo.registerPrefix(nodeB, linkBC->getFace(nodeB), PRODUCER_PREFIX);
  topo.registerPrefix(nodeD, linkCD->getFace(nodeD), PRODUCER_PREFIX);

  // Send 6 interests let it change to use the 10 ms link
  runConsumer(6);

  auto linkAC = topo.addLink("AC", 5_ms, {nodeA, nodeD});
  topo.registerPrefix(nodeA, linkAC->getFace(nodeA), PRODUCER_PREFIX, 1);

  BOOST_CHECK_EQUAL(linkAC->getFace(nodeA).getCounters().nOutInterests, 0);

  // After 30 seconds a probe would be sent that would switch make ASF switch
  runConsumer(30);

  BOOST_CHECK_EQUAL(linkAC->getFace(nodeA).getCounters().nOutInterests, 1);
}

BOOST_AUTO_TEST_CASE(InstantiationWithParameters)
{
  FaceTable faceTable;
  Forwarder forwarder{faceTable};

  auto checkValidity = [&] (std::string parameters, bool isCorrect) {
    Name strategyName(Name(AsfStrategy::getStrategyName()).append(std::move(parameters)));
    if (isCorrect) {
      BOOST_CHECK_NO_THROW(make_unique<AsfStrategy>(forwarder, strategyName));
    }
    else {
      BOOST_CHECK_THROW(make_unique<AsfStrategy>(forwarder, strategyName), std::invalid_argument);
    }
  };

  checkValidity("/probing-interval~30000/max-timeouts~5", true);
  checkValidity("/max-timeouts~5/probing-interval~30000", true);
  checkValidity("/probing-interval~30000", true);
  checkValidity("/max-timeouts~5", true);
  checkValidity("", true);

  checkValidity("/probing-interval~500", false); // At least 1 seconds
  checkValidity("/probing-interval~-5000", false);
  checkValidity("/max-timeouts~0", false);
  checkValidity("/max-timeouts~-5", false);
  checkValidity("/max-timeouts~-5/probing-interval~-30000", false);
  checkValidity("/max-timeouts", false);
  checkValidity("/probing-interval~", false);
  checkValidity("/~1000", false);
  checkValidity("/probing-interval~foo", false);
  checkValidity("/max-timeouts~1~2", false);
  checkValidity("/foo", false);
  checkValidity("/foo~42", false);
}

BOOST_AUTO_TEST_SUITE_END() // TestAsfStrategy
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace asf
} // namespace fw
} // namespace nfd
