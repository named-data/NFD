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

#include "table/strategy-choice.hpp"

#include "tests/test-common.hpp"
#include "../fw/dummy-strategy.hpp"
#include "../fw/install-strategy.hpp"

namespace nfd {
namespace tests {

class StrategyChoiceFixture : public BaseFixture
{
protected:
  StrategyChoiceFixture()
    : sc(forwarder.getStrategyChoice())
  {
  }

  Name
  insertAndGet(const Name& prefix, const Name& strategyName)
  {
    BOOST_REQUIRE(sc.insert(prefix, strategyName));
    bool isFound;
    Name foundName;
    std::tie(isFound, foundName) = sc.get(prefix);
    BOOST_REQUIRE(isFound);
    return foundName;
  }

protected:
  Forwarder forwarder;
  StrategyChoice& sc;
};

BOOST_AUTO_TEST_SUITE(Table)
BOOST_FIXTURE_TEST_SUITE(TestStrategyChoice, StrategyChoiceFixture)

using fw::Strategy;

BOOST_AUTO_TEST_CASE(Parameters)
{
  const Name strategyName("/strategy-choice-test-parameters/%FD%01");
  DummyStrategy::registerAs(strategyName);

  // no parameters
  BOOST_CHECK_EQUAL(this->insertAndGet("/A", strategyName), strategyName);

  // one parameter
  Name oneParamName = Name(strategyName).append("param");
  BOOST_CHECK_EQUAL(this->insertAndGet("/B", oneParamName), oneParamName);

  // two parameters
  Name twoParamName = Name(strategyName).append("x").append("y");
  BOOST_CHECK_EQUAL(this->insertAndGet("/C", twoParamName), twoParamName);

  // parameter without version is disallowed
  Name oneParamUnversioned = strategyName.getPrefix(-1).append("param");
  BOOST_CHECK_EQUAL(sc.insert("/D", oneParamUnversioned), false);
}

BOOST_AUTO_TEST_CASE(Get)
{
  Name nameP("ndn:/strategy/P");
  install<DummyStrategy>(forwarder, nameP);

  BOOST_CHECK(sc.insert("ndn:/", nameP));
  // { '/'=>P }

  auto getRoot = sc.get("ndn:/");
  BOOST_CHECK_EQUAL(getRoot.first, true);
  BOOST_CHECK_EQUAL(getRoot.second, nameP);

  auto getA = sc.get("ndn:/A");
  BOOST_CHECK_EQUAL(getA.first, false);
}

BOOST_AUTO_TEST_CASE(FindEffectiveStrategy)
{
  Name nameP("ndn:/strategy/P");
  Name nameQ("ndn:/strategy/Q");
  Name nameZ("ndn:/strategy/Z");
  install<DummyStrategy>(forwarder, nameP);
  install<DummyStrategy>(forwarder, nameQ);

  BOOST_CHECK(sc.insert("ndn:/", nameP));
  // { '/'=>P }

  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/")   .getName(), nameP);
  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/A")  .getName(), nameP);
  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/A/B").getName(), nameP);

  BOOST_CHECK(sc.insert("ndn:/A/B", nameP));
  // { '/'=>P, '/A/B'=>P }

  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/")   .getName(), nameP);
  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/A")  .getName(), nameP);
  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/A/B").getName(), nameP);
  // same instance
  BOOST_CHECK_EQUAL(&sc.findEffectiveStrategy("ndn:/"),
                    &sc.findEffectiveStrategy("ndn:/A/B"));

  sc.erase("ndn:/A"); // no effect
  // { '/'=>P, '/A/B'=>P }

  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/")   .getName(), nameP);
  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/A")  .getName(), nameP);
  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/A/B").getName(), nameP);

  BOOST_CHECK(sc.insert("ndn:/A", nameQ));
  // { '/'=>P, '/A/B'=>P, '/A'=>Q }

  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/")   .getName(), nameP);
  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/A")  .getName(), nameQ);
  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/A/B").getName(), nameP);

  sc.erase("ndn:/A/B");
  // { '/'=>P, '/A'=>Q }

  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/")   .getName(), nameP);
  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/A")  .getName(), nameQ);
  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/A/B").getName(), nameQ);

  BOOST_CHECK(!sc.insert("ndn:/", nameZ)); // non existent strategy

  BOOST_CHECK(sc.insert("ndn:/", nameQ));
  BOOST_CHECK(sc.insert("ndn:/A", nameP));
  // { '/'=>Q, '/A'=>P }

  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/")   .getName(), nameQ);
  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/A")  .getName(), nameP);
  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/A/B").getName(), nameP);
  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/D")  .getName(), nameQ);
}

BOOST_AUTO_TEST_CASE(FindEffectiveStrategyWithPitEntry)
{
  Name nameP("ndn:/strategy/P");
  Name nameQ("ndn:/strategy/Q");
  install<DummyStrategy>(forwarder, nameP);
  install<DummyStrategy>(forwarder, nameQ);

  shared_ptr<Data> dataABC = makeData("/A/B/C");
  Name fullName = dataABC->getFullName();

  BOOST_CHECK(sc.insert("/A", nameP));
  BOOST_CHECK(sc.insert(fullName, nameQ));

  Pit& pit = forwarder.getPit();
  shared_ptr<Interest> interestAB = makeInterest("/A/B");
  shared_ptr<pit::Entry> pitAB = pit.insert(*interestAB).first;
  shared_ptr<Interest> interestFull = makeInterest(fullName);
  shared_ptr<pit::Entry> pitFull = pit.insert(*interestFull).first;

  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy(*pitAB).getName(), nameP);
  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy(*pitFull).getName(), nameQ);
}

BOOST_AUTO_TEST_CASE(FindEffectiveStrategyWithMeasurementsEntry)
{
  Name nameP("ndn:/strategy/P");
  Name nameQ("ndn:/strategy/Q");
  install<DummyStrategy>(forwarder, nameP);
  install<DummyStrategy>(forwarder, nameQ);

  BOOST_CHECK(sc.insert("/A", nameP));
  BOOST_CHECK(sc.insert("/A/B/C", nameQ));

  Measurements& measurements = forwarder.getMeasurements();
  measurements::Entry& mAB = measurements.get("/A/B");
  measurements::Entry& mABCD = measurements.get("/A/B/C/D");

  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy(mAB).getName(), nameP);
  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy(mABCD).getName(), nameQ);
}

//XXX BOOST_CONCEPT_ASSERT((ForwardIterator<std::vector<int>::iterator>))
//    is also failing. There might be a problem with ForwardIterator concept checking.
//BOOST_CONCEPT_ASSERT((ForwardIterator<StrategyChoice::const_iterator>));

BOOST_AUTO_TEST_CASE(Enumerate)
{
  Name nameP("ndn:/strategy/P");
  Name nameQ("ndn:/strategy/Q");
  install<DummyStrategy>(forwarder, nameP);
  install<DummyStrategy>(forwarder, nameQ);

  sc.insert("ndn:/",      nameP);
  sc.insert("ndn:/A/B",   nameQ);
  sc.insert("ndn:/A/B/C", nameP);
  sc.insert("ndn:/D",     nameP);
  sc.insert("ndn:/E",     nameQ);

  BOOST_CHECK_EQUAL(sc.size(), 5);

  std::map<Name, Name> map; // namespace=>strategyName
  for (StrategyChoice::const_iterator it = sc.begin(); it != sc.end(); ++it) {
    map[it->getPrefix()] = it->getStrategyName();
  }
  BOOST_CHECK_EQUAL(map.size(), 5);
  BOOST_CHECK_EQUAL(map["ndn:/"],      nameP);
  BOOST_CHECK_EQUAL(map["ndn:/A/B"],   nameQ);
  BOOST_CHECK_EQUAL(map["ndn:/A/B/C"], nameP);
  BOOST_CHECK_EQUAL(map["ndn:/D"],     nameP);
  BOOST_CHECK_EQUAL(map["ndn:/E"],     nameQ);
  BOOST_CHECK_EQUAL(map.size(), 5);
}

class PStrategyInfo : public fw::StrategyInfo
{
public:
  static constexpr int
  getTypeId()
  {
    return 10;
  }
};

BOOST_AUTO_TEST_CASE(ClearStrategyInfo)
{
  Name nameP("ndn:/strategy/P");
  Name nameQ("ndn:/strategy/Q");
  install<DummyStrategy>(forwarder, nameP);
  install<DummyStrategy>(forwarder, nameQ);

  Measurements& measurements = forwarder.getMeasurements();

  BOOST_CHECK(sc.insert("ndn:/", nameP));
  // { '/'=>P }
  measurements.get("ndn:/").insertStrategyInfo<PStrategyInfo>();
  measurements.get("ndn:/A").insertStrategyInfo<PStrategyInfo>();
  measurements.get("ndn:/A/B").insertStrategyInfo<PStrategyInfo>();
  measurements.get("ndn:/A/C").insertStrategyInfo<PStrategyInfo>();

  BOOST_CHECK(sc.insert("ndn:/A/B", nameP));
  // { '/'=>P, '/A/B'=>P }
  BOOST_CHECK(measurements.get("ndn:/").getStrategyInfo<PStrategyInfo>() != nullptr);
  BOOST_CHECK(measurements.get("ndn:/A").getStrategyInfo<PStrategyInfo>() != nullptr);
  BOOST_CHECK(measurements.get("ndn:/A/B").getStrategyInfo<PStrategyInfo>() != nullptr);
  BOOST_CHECK(measurements.get("ndn:/A/C").getStrategyInfo<PStrategyInfo>() != nullptr);

  BOOST_CHECK(sc.insert("ndn:/A", nameQ));
  // { '/'=>P, '/A/B'=>P, '/A'=>Q }
  BOOST_CHECK(measurements.get("ndn:/").getStrategyInfo<PStrategyInfo>() != nullptr);
  BOOST_CHECK(measurements.get("ndn:/A").getStrategyInfo<PStrategyInfo>() == nullptr);
  BOOST_CHECK(measurements.get("ndn:/A/B").getStrategyInfo<PStrategyInfo>() != nullptr);
  BOOST_CHECK(measurements.get("ndn:/A/C").getStrategyInfo<PStrategyInfo>() == nullptr);

  sc.erase("ndn:/A/B");
  // { '/'=>P, '/A'=>Q }
  BOOST_CHECK(measurements.get("ndn:/").getStrategyInfo<PStrategyInfo>() != nullptr);
  BOOST_CHECK(measurements.get("ndn:/A").getStrategyInfo<PStrategyInfo>() == nullptr);
  BOOST_CHECK(measurements.get("ndn:/A/B").getStrategyInfo<PStrategyInfo>() == nullptr);
  BOOST_CHECK(measurements.get("ndn:/A/C").getStrategyInfo<PStrategyInfo>() == nullptr);
}

BOOST_AUTO_TEST_CASE(EraseNameTreeEntry)
{
  Name nameP("ndn:/strategy/P");
  Name nameQ("ndn:/strategy/Q");
  install<DummyStrategy>(forwarder, nameP);
  install<DummyStrategy>(forwarder, nameQ);

  NameTree& nameTree = forwarder.getNameTree();

  sc.insert("ndn:/", nameP);

  size_t nNameTreeEntriesBefore = nameTree.size();

  sc.insert("ndn:/A/B", nameQ);
  sc.erase("ndn:/A/B");
  BOOST_CHECK_EQUAL(nameTree.size(), nNameTreeEntriesBefore);
}

BOOST_AUTO_TEST_CASE(Versioning)
{
  Forwarder forwarder;
  Name nameP("ndn:/strategy/P");
  Name nameP1("ndn:/strategy/P/%FD%01");
  Name nameP2("ndn:/strategy/P/%FD%02");
  Name name3("ndn:/%FD%03");
  Name name4("ndn:/%FD%04");
  Name nameQ("ndn:/strategy/Q");
  Name nameQ5("ndn:/strategy/Q/%FD%05");

  // install
  auto strategyP1 = make_unique<DummyStrategy>(ref(forwarder), nameP1);
  Strategy* instanceP1 = strategyP1.get();
  auto strategyP1b = make_unique<DummyStrategy>(ref(forwarder), nameP1);
  auto strategyP2 = make_unique<DummyStrategy>(ref(forwarder), nameP2);
  auto strategy3 = make_unique<DummyStrategy>(ref(forwarder), name3);
  auto strategy4 = make_unique<DummyStrategy>(ref(forwarder), name4);
  auto strategyQ = make_unique<DummyStrategy>(ref(forwarder), nameQ);
  auto strategyQ5 = make_unique<DummyStrategy>(ref(forwarder), nameQ5);

  bool isInstalled = false;
  Strategy* installed = nullptr;

  std::tie(isInstalled, installed) = sc.install(std::move(strategyP1));
  BOOST_CHECK_EQUAL(isInstalled, true);
  BOOST_CHECK_EQUAL(installed, instanceP1);
  std::tie(isInstalled, installed) = sc.install(std::move(strategyP1b));
  BOOST_CHECK_EQUAL(isInstalled, false);
  BOOST_CHECK_EQUAL(installed, instanceP1);

  BOOST_CHECK_EQUAL(sc.hasStrategy(nameP,  false), true);
  BOOST_CHECK_EQUAL(sc.hasStrategy(nameP,  true),  false);
  BOOST_CHECK_EQUAL(sc.hasStrategy(nameP1, true),  true);

  BOOST_CHECK_EQUAL(sc.install(std::move(strategyP2)).first, true);
  BOOST_CHECK_EQUAL(sc.install(std::move(strategy3)).first, true);
  BOOST_CHECK_EQUAL(sc.install(std::move(strategy4)).first, true);
  BOOST_CHECK_EQUAL(sc.install(std::move(strategyQ)).first, true);
  BOOST_CHECK_EQUAL(sc.install(std::move(strategyQ5)).first, true);

  BOOST_CHECK(sc.insert("ndn:/", nameQ));
  // exact match, { '/'=>Q }
  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/").getName(), nameQ);

  BOOST_CHECK(sc.insert("ndn:/", nameQ));
  BOOST_CHECK(sc.insert("ndn:/", nameP));
  // { '/'=>P2 }
  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/").getName(), nameP2);

  BOOST_CHECK(sc.insert("ndn:/", nameQ));
  BOOST_CHECK(sc.insert("ndn:/", nameP1));
  // { '/'=>P1 }
  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/").getName(), nameP1);

  BOOST_CHECK(sc.insert("ndn:/", nameQ));
  BOOST_CHECK(sc.insert("ndn:/", nameP2));
  // { '/'=>P2 }
  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/").getName(), nameP2);

  BOOST_CHECK(sc.insert("ndn:/", nameQ));
  BOOST_CHECK(! sc.insert("ndn:/", "ndn:/strategy/A"));
  // not installed
  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/").getName(), nameQ);

  BOOST_CHECK(sc.insert("ndn:/", nameQ));
  BOOST_CHECK(! sc.insert("ndn:/", "ndn:/strategy/Z"));
  // not installed
  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/").getName(), nameQ);

  BOOST_CHECK(sc.insert("ndn:/", nameP1));
  BOOST_CHECK(sc.insert("ndn:/", "ndn:/"));
  // match one component longer only, { '/'=>4 }
  BOOST_CHECK_EQUAL(sc.findEffectiveStrategy("ndn:/").getName(), name4);
}

BOOST_AUTO_TEST_SUITE_END() // TestStrategyChoice
BOOST_AUTO_TEST_SUITE_END() // Table

} // namespace tests
} // namespace nfd
