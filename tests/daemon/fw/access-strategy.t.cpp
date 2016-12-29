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

#include "fw/access-strategy.hpp"

#include "tests/test-common.hpp"
#include "strategy-tester.hpp"
#include "topology-tester.hpp"

namespace nfd {
namespace fw {
namespace tests {

using namespace nfd::tests;

// The tester is unused in this file, but it's used in various templated test suites.
typedef StrategyTester<AccessStrategy> AccessStrategyTester;
NFD_REGISTER_STRATEGY(AccessStrategyTester);

// This test suite tests AccessStrategy's behavior as a black box,
// without accessing its internals.
//
// Many test assertions are qualitative rather than quantitative.
// They capture the design highlights of the strategy without requiring a definite value,
// so that the test suite is not fragile to minor changes in the strategy implementation.
//
// Topology graphes in this test suite are shown in ASCII art,
// in a style similar to ns-3 and ndnSIM examples.
// They are enclosed in multi-line comments, which is an intentional violation of
// code style rule 3.25. This is necessary because some lines ends with '\' which
// would cause "multi-line comment" compiler warning if '//' comments are used.

class TwoLaptopsFixture : public UnitTestTimeFixture
{
protected:
  TwoLaptopsFixture()
  {
    /*
     *                  +--------+
     *           +----->| router |<------+
     *           |      +--------+       |
     *      10ms |                       | 20ms
     *           v                       v
     *      +---------+             +---------+
     *      | laptopA |             | laptopB |
     *      +---------+             +---------+
     */

    router = topo.addForwarder("R");
    laptopA = topo.addForwarder("A");
    laptopB = topo.addForwarder("B");

    topo.setStrategy<fw::AccessStrategy>(router);

    linkA = topo.addLink("RA", time::milliseconds(10), {router, laptopA});
    linkB = topo.addLink("RB", time::milliseconds(20), {router, laptopB});
  }

protected:
  TopologyTester topo;

  TopologyNode router;
  TopologyNode laptopA;
  TopologyNode laptopB;
  shared_ptr<TopologyLink> linkA;
  shared_ptr<TopologyLink> linkB;
};

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_FIXTURE_TEST_SUITE(TestAccessStrategy, TwoLaptopsFixture)

BOOST_AUTO_TEST_CASE(OneProducer)
{
  /*
   *             /------------------\
   *             | intervalConsumer |
   *             \------------------/
   *                      ^ v
   *                      | v /laptops/A
   *                      |
   *                      v
   *      /laptops << +--------+ >> /laptops
   *           +----->| router |<------+
   *           |      +--------+       |
   *      10ms |                       | 20ms
   *           v                       v
   *      +---------+             +---------+
   *      | laptopA |             | laptopB |
   *      +---------+             +---------+
   *           ^  v
   *           |  v /laptops/A
   *           v
   *    /--------------\
   *    | echoProducer |
   *    \--------------/
   */

  // two laptops have same prefix in router FIB
  topo.registerPrefix(router, linkA->getFace(router), "ndn:/laptops");
  topo.registerPrefix(router, linkB->getFace(router), "ndn:/laptops");

  shared_ptr<TopologyAppLink> producer = topo.addAppFace("p", laptopA, "ndn:/laptops/A");
  topo.addEchoProducer(producer->getClientFace());

  shared_ptr<TopologyAppLink> consumer = topo.addAppFace("c", router);
  topo.addIntervalConsumer(consumer->getClientFace(), "ndn:/laptops/A",
                           time::milliseconds(100), 100);

  this->advanceClocks(time::milliseconds(5), time::seconds(12));

  // most Interests should be satisfied, and few Interests can go to wrong laptop
  BOOST_CHECK_GE(consumer->getForwarderFace().getCounters().nOutData, 97);
  BOOST_CHECK_GE(linkA->getFace(router).getCounters().nOutInterests, 97);
  BOOST_CHECK_LE(linkB->getFace(router).getCounters().nOutInterests, 5);
}

BOOST_AUTO_TEST_CASE(FastSlowProducer)
{
  /*
   *             /------------------\
   *             | intervalConsumer |
   *             \------------------/
   *                      ^ v
   *                      | v /laptops/BOTH
   *                      |
   *                      v
   *      /laptops << +--------+ >> /laptops
   *           +----->| router |<------+
   *           |      +--------+       |
   *      10ms |                       | 20ms
   *           v                       v
   *      +---------+             +---------+
   *      | laptopA |             | laptopB |
   *      +---------+             +---------+
   *           ^  v                    ^  v
   *           |  v /laptops/BOTH      |  v /laptops/BOTH
   *           v                       v
   *    /--------------\        /--------------\
   *    | echoProducer |        | echoProducer |
   *    \--------------/        \--------------/
   */

  // two laptops have same prefix in router FIB
  topo.registerPrefix(router, linkA->getFace(router), "ndn:/laptops");
  topo.registerPrefix(router, linkB->getFace(router), "ndn:/laptops");

  shared_ptr<TopologyAppLink> producerA = topo.addAppFace("pA", laptopA, "ndn:/laptops/BOTH");
  topo.addEchoProducer(producerA->getClientFace());
  shared_ptr<TopologyAppLink> producerB = topo.addAppFace("pB", laptopB, "ndn:/laptops/BOTH");
  topo.addEchoProducer(producerB->getClientFace());

  shared_ptr<TopologyAppLink> consumer = topo.addAppFace("c", router);
  topo.addIntervalConsumer(consumer->getClientFace(), "ndn:/laptops/BOTH",
                           time::milliseconds(100), 100);

  this->advanceClocks(time::milliseconds(5), time::seconds(12));

  // most Interests should be satisfied, and few Interests can go to slower laptopB
  BOOST_CHECK_GE(consumer->getForwarderFace().getCounters().nOutData, 97);
  BOOST_CHECK_GE(linkA->getFace(router).getCounters().nOutInterests, 90);
  BOOST_CHECK_LE(linkB->getFace(router).getCounters().nOutInterests, 15);
}

BOOST_AUTO_TEST_CASE(ProducerMobility)
{
  /*
   *           /------------------\                              /------------------\
   *           | intervalConsumer |                              | intervalConsumer |
   *           \------------------/              A               \------------------/
   *                    ^ v                      f                        ^ v
   *                    | v /laptops/M           t                        | v /laptops/M
   *                    |                        e                        |
   *                    v                        r                        v
   *    /laptops << +--------+ >> /laptops                /laptops << +--------+ >> /laptops
   *         +----->| router |<------+           6             +----->| router |<------+
   *         |      +--------+       |                         |      +--------+       |
   *    10ms |                       | 20ms  === s ==>    10ms |                       | 20ms
   *         v                       v           e             v                       v
   *    +---------+             +---------+      c        +---------+             +---------+
   *    | laptopA |             | laptopB |      o        | laptopA |             | laptopB |
   *    +---------+             +---------+      n        +---------+             +---------+
   *         ^  v                                d                                  v  ^
   *         |  v /laptops/M                     s                       /laptops/M v  |
   *         v                                                                         v
   *  /--------------\                                                          /--------------\
   *  | echoProducer |                                                          | echoProducer |
   *  \--------------/                                                          \--------------/
   */

  // two laptops have same prefix in router FIB
  topo.registerPrefix(router, linkA->getFace(router), "ndn:/laptops");
  topo.registerPrefix(router, linkB->getFace(router), "ndn:/laptops");

  shared_ptr<TopologyAppLink> producerA = topo.addAppFace("pA", laptopA, "ndn:/laptops/M");
  topo.addEchoProducer(producerA->getClientFace());
  shared_ptr<TopologyAppLink> producerB = topo.addAppFace("pB", laptopB, "ndn:/laptops/M");
  topo.addEchoProducer(producerB->getClientFace());

  shared_ptr<TopologyAppLink> consumer = topo.addAppFace("c", router);
  topo.addIntervalConsumer(consumer->getClientFace(), "ndn:/laptops/M",
                           time::milliseconds(100), 100);

  // producer is initially on laptopA
  producerB->fail();
  this->advanceClocks(time::milliseconds(5), time::seconds(6));

  // few Interests can go to laptopB
  BOOST_CHECK_LE(linkB->getFace(router).getCounters().nOutInterests, 5);

  // producer moves to laptopB
  producerA->fail();
  producerB->recover();
  PacketCounter::rep nInterestsToA_beforeMove = linkA->getFace(router).getCounters().nOutInterests;
  this->advanceClocks(time::milliseconds(5), time::seconds(6));

  // few additional Interests can go to laptopA
  BOOST_CHECK_LE(linkA->getFace(router).getCounters().nOutInterests - nInterestsToA_beforeMove, 5);

  // most Interests should be satisfied
  BOOST_CHECK_GE(consumer->getForwarderFace().getCounters().nOutData, 97);
}

BOOST_AUTO_TEST_CASE(Bidirectional)
{
  /*
   *                         /laptops << +--------+ >> /laptops
   *                              +----->| router |<------+
   *                              |      +--------+       |
   *                      ^  10ms |                       | 20ms  ^
   *                    / ^       v                       v       ^ /
   *                         +---------+             +---------+
   *                  +----->| laptopA |             | laptopB |<------------+
   *                  |      +---------+             +---------+             |
   *                  |           ^  v /laptops/A         ^  v /laptops/B    |
   *               ^  |           |  v                    |  v               |  ^
   *    /laptops/B ^  v           v                       v                  v  ^ /laptops/A
   *  /------------------\   /--------------\        /--------------\     /------------------\
   *  | intervalConsumer |   | echoProducer |        | echoProducer |     | intervalConsumer |
   *  \------------------/   \--------------/        \--------------/     \------------------/
   */

  // laptops have default routes toward the router
  topo.registerPrefix(laptopA, linkA->getFace(laptopA), "ndn:/");
  topo.registerPrefix(laptopB, linkB->getFace(laptopB), "ndn:/");

  // two laptops have same prefix in router FIB
  topo.registerPrefix(router, linkA->getFace(router), "ndn:/laptops");
  topo.registerPrefix(router, linkB->getFace(router), "ndn:/laptops");

  shared_ptr<TopologyAppLink> producerA = topo.addAppFace("pA", laptopA, "ndn:/laptops/A");
  topo.addEchoProducer(producerA->getClientFace());
  shared_ptr<TopologyAppLink> producerB = topo.addAppFace("pB", laptopB, "ndn:/laptops/B");
  topo.addEchoProducer(producerB->getClientFace());

  shared_ptr<TopologyAppLink> consumerAB = topo.addAppFace("cAB", laptopA);
  topo.addIntervalConsumer(consumerAB->getClientFace(), "ndn:/laptops/B",
                           time::milliseconds(100), 100);
  shared_ptr<TopologyAppLink> consumerBA = topo.addAppFace("cBA", laptopB);
  topo.addIntervalConsumer(consumerBA->getClientFace(), "ndn:/laptops/A",
                           time::milliseconds(100), 100);

  this->advanceClocks(time::milliseconds(5), time::seconds(12));

  // most Interests should be satisfied
  BOOST_CHECK_GE(consumerAB->getForwarderFace().getCounters().nOutData, 97);
  BOOST_CHECK_GE(consumerBA->getForwarderFace().getCounters().nOutData, 97);
}

BOOST_AUTO_TEST_CASE(PacketLoss)
{
  /*
   *   test case Interests
   *           |
   *           v
   *      +--------+
   *      | router |
   *      +--------+
   *           |  v
   *      10ms |  v /laptops
   *           v
   *      +---------+
   *      | laptopA |
   *      +---------+
   *           ^  v
   *           |  v /laptops/A
   *           v
   *    /--------------\
   *    | echoProducer |
   *    \--------------/
   */

  // laptopA has prefix in router FIB; laptopB is unused in this test case
  topo.registerPrefix(router, linkA->getFace(router), "ndn:/laptops");

  shared_ptr<TopologyAppLink> producerA = topo.addAppFace("pA", laptopA, "ndn:/laptops/A");
  topo.addEchoProducer(producerA->getClientFace());

  shared_ptr<TopologyAppLink> consumer = topo.addAppFace("c", router);

  // Interest 1 completes normally
  shared_ptr<Interest> interest1 = makeInterest("ndn:/laptops/A/1");
  bool hasData1 = false;
  consumer->getClientFace().expressInterest(*interest1,
                                            bind([&hasData1] { hasData1 = true; }),
                                            nullptr, nullptr);
  this->advanceClocks(time::milliseconds(5), time::seconds(1));
  BOOST_CHECK_EQUAL(hasData1, true);

  // Interest 2 experiences a packet loss on initial transmission
  shared_ptr<Interest> interest2a = makeInterest("ndn:/laptops/A/2");
  bool hasData2a = false, hasTimeout2a = false;
  consumer->getClientFace().expressInterest(*interest2a,
                                            bind([&hasData2a] { hasData2a = true; }),
                                            nullptr,
                                            bind([&hasTimeout2a] { hasTimeout2a = true; }));
  producerA->fail();
  this->advanceClocks(time::milliseconds(5), time::milliseconds(60));
  BOOST_CHECK_EQUAL(hasData2a, false);
  BOOST_CHECK_EQUAL(hasTimeout2a, false);

  // Interest 2 retransmission is suppressed
  shared_ptr<Interest> interest2b = makeInterest("ndn:/laptops/A/2");
  bool hasData2b = false;
  consumer->getClientFace().expressInterest(*interest2b,
                                            bind([&hasData2b] { hasData2b = true; }),
                                            nullptr, nullptr);
  producerA->recover();
  this->advanceClocks(time::milliseconds(5), time::seconds(1));
  BOOST_CHECK_EQUAL(hasData2b, false);

  // Interest 2 retransmission gets through, and is answered
  shared_ptr<Interest> interest2c = makeInterest("ndn:/laptops/A/2");
  bool hasData2c = false;
  consumer->getClientFace().expressInterest(*interest2c,
                                            bind([&hasData2c] { hasData2c = true; }),
                                            nullptr, nullptr);
  this->advanceClocks(time::milliseconds(5), time::seconds(1));
  BOOST_CHECK_EQUAL(hasData2c, true);
}

BOOST_AUTO_TEST_CASE(Bug2831)
{
  // make a two-node loop
  topo.registerPrefix(laptopA, linkA->getFace(laptopA), "ndn:/net");
  topo.registerPrefix(router, linkA->getFace(router), "ndn:/net");

  // send Interests from laptopA to router
  shared_ptr<TopologyAppLink> consumer = topo.addAppFace("c", laptopA);
  topo.addIntervalConsumer(consumer->getClientFace(), "ndn:/net",
                           time::milliseconds(100), 10);

  this->advanceClocks(time::milliseconds(5), time::seconds(2));

  // Interest shouldn't loop back from router
  BOOST_CHECK_EQUAL(linkA->getFace(router).getCounters().nOutInterests, 0);
}

BOOST_AUTO_TEST_SUITE_END() // TestAccessStrategy
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace fw
} // namespace nfd
