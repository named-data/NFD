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

#include "fw/random-strategy.hpp"
#include "common/global.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/face/dummy-face.hpp"
#include "strategy-tester.hpp"

namespace nfd {
namespace fw {
namespace tests {

using RandomStrategyTester = StrategyTester<RandomStrategy>;
NFD_REGISTER_STRATEGY(RandomStrategyTester);

BOOST_AUTO_TEST_SUITE(Fw)

class RandomStrategyFixture : public GlobalIoTimeFixture
{
protected:
  RandomStrategyFixture()
    : face1(make_shared<DummyFace>())
    , face2(make_shared<DummyFace>())
    , face3(make_shared<DummyFace>())
    , face4(make_shared<DummyFace>())
  {
    faceTable.add(face1);
    faceTable.add(face2);
    faceTable.add(face3);
    faceTable.add(face4);
  }

protected:
  FaceTable faceTable;
  Forwarder forwarder{faceTable};
  RandomStrategyTester strategy{forwarder};
  Fib& fib{forwarder.getFib()};
  Pit& pit{forwarder.getPit()};

  shared_ptr<DummyFace> face1;
  shared_ptr<DummyFace> face2;
  shared_ptr<DummyFace> face3;
  shared_ptr<DummyFace> face4;
};

BOOST_FIXTURE_TEST_SUITE(TestRandomStrategy, RandomStrategyFixture)

BOOST_AUTO_TEST_CASE(Forward)
{
  fib::Entry& fibEntry = *fib.insert(Name()).first;
  fib.addOrUpdateNextHop(fibEntry, *face2, 10);
  fib.addOrUpdateNextHop(fibEntry, *face3, 20);
  fib.addOrUpdateNextHop(fibEntry, *face4, 30);

  // Send 1000 Interests
  for (int i = 0; i < 1000; ++i) {
    shared_ptr<Interest> interest = makeInterest("ndn:/BzgFBchqA" + std::to_string(i));
    shared_ptr<pit::Entry> pitEntry = pit.insert(*interest).first;

    pitEntry->insertOrUpdateInRecord(*face1, *interest);
    strategy.afterReceiveInterest(FaceEndpoint(*face1, 0), *interest, pitEntry);
  }

  // Map outFaceId -> SentInterests.
  std::unordered_map<int, int> faceInterestMap;
  for (const auto& i : strategy.sendInterestHistory) {
    faceInterestMap[i.outFaceId]++;
  }

  // Check that all faces received at least 10 Interest
  for (const auto& x : faceInterestMap) {
    BOOST_CHECK_GE(x.second, 10);
  }
}

BOOST_AUTO_TEST_SUITE_END() // TestRandomStrategy
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace fw
} // namespace nfd
