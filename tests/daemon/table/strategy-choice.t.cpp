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

#include "table/strategy-choice.hpp"
#include "tests/daemon/fw/dummy-strategy.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TableStrategyChoice, BaseFixture)

using fw::Strategy;

BOOST_AUTO_TEST_CASE(Get)
{
  Forwarder forwarder;
  Name nameP("ndn:/strategy/P");
  shared_ptr<Strategy> strategyP = make_shared<DummyStrategy>(ref(forwarder), nameP);

  StrategyChoice& table = forwarder.getStrategyChoice();

  // install
  BOOST_CHECK_EQUAL(table.install(strategyP), true);

  BOOST_CHECK(table.insert("ndn:/", nameP));
  // { '/'=>P }

  auto getRoot = table.get("ndn:/");
  BOOST_CHECK_EQUAL(getRoot.first, true);
  BOOST_CHECK_EQUAL(getRoot.second, nameP);

  auto getA = table.get("ndn:/A");
  BOOST_CHECK_EQUAL(getA.first, false);
}

BOOST_AUTO_TEST_CASE(Effective)
{
  Forwarder forwarder;
  Name nameP("ndn:/strategy/P");
  Name nameQ("ndn:/strategy/Q");
  Name nameZ("ndn:/strategy/Z");
  shared_ptr<Strategy> strategyP = make_shared<DummyStrategy>(ref(forwarder), nameP);
  shared_ptr<Strategy> strategyQ = make_shared<DummyStrategy>(ref(forwarder), nameQ);

  StrategyChoice& table = forwarder.getStrategyChoice();

  // install
  BOOST_CHECK_EQUAL(table.install(strategyP), true);
  BOOST_CHECK_EQUAL(table.install(strategyQ), true);
  BOOST_CHECK_EQUAL(table.install(strategyQ), false);

  BOOST_CHECK(table.insert("ndn:/", nameP));
  // { '/'=>P }

  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/")   .getName(), nameP);
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/A")  .getName(), nameP);
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/A/B").getName(), nameP);

  BOOST_CHECK(table.insert("ndn:/A/B", nameP));
  // { '/'=>P, '/A/B'=>P }

  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/")   .getName(), nameP);
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/A")  .getName(), nameP);
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/A/B").getName(), nameP);
  // same instance
  BOOST_CHECK_EQUAL(&table.findEffectiveStrategy("ndn:/"),    strategyP.get());
  BOOST_CHECK_EQUAL(&table.findEffectiveStrategy("ndn:/A"),   strategyP.get());
  BOOST_CHECK_EQUAL(&table.findEffectiveStrategy("ndn:/A/B"), strategyP.get());

  table.erase("ndn:/A"); // no effect
  // { '/'=>P, '/A/B'=>P }

  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/")   .getName(), nameP);
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/A")  .getName(), nameP);
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/A/B").getName(), nameP);

  BOOST_CHECK(table.insert("ndn:/A", nameQ));
  // { '/'=>P, '/A/B'=>P, '/A'=>Q }

  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/")   .getName(), nameP);
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/A")  .getName(), nameQ);
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/A/B").getName(), nameP);

  table.erase("ndn:/A/B");
  // { '/'=>P, '/A'=>Q }

  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/")   .getName(), nameP);
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/A")  .getName(), nameQ);
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/A/B").getName(), nameQ);

  BOOST_CHECK(!table.insert("ndn:/", nameZ)); // non existent strategy

  BOOST_CHECK(table.insert("ndn:/", nameQ));
  BOOST_CHECK(table.insert("ndn:/A", nameP));
  // { '/'=>Q, '/A'=>P }

  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/")   .getName(), nameQ);
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/A")  .getName(), nameP);
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/A/B").getName(), nameP);
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/D")  .getName(), nameQ);
}

//XXX BOOST_CONCEPT_ASSERT((ForwardIterator<std::vector<int>::iterator>))
//    is also failing. There might be a problem with ForwardIterator concept checking.
//BOOST_CONCEPT_ASSERT((ForwardIterator<StrategyChoice::const_iterator>));

BOOST_AUTO_TEST_CASE(Enumerate)
{

  Forwarder forwarder;
  Name nameP("ndn:/strategy/P");
  Name nameQ("ndn:/strategy/Q");
  shared_ptr<Strategy> strategyP = make_shared<DummyStrategy>(ref(forwarder), nameP);
  shared_ptr<Strategy> strategyQ = make_shared<DummyStrategy>(ref(forwarder), nameQ);

  StrategyChoice& table = forwarder.getStrategyChoice();
  table.install(strategyP);
  table.install(strategyQ);

  table.insert("ndn:/",      nameP);
  table.insert("ndn:/A/B",   nameQ);
  table.insert("ndn:/A/B/C", nameP);
  table.insert("ndn:/D",     nameP);
  table.insert("ndn:/E",     nameQ);

  BOOST_CHECK_EQUAL(table.size(), 5);

  std::map<Name, Name> map; // namespace=>strategyName
  for (StrategyChoice::const_iterator it = table.begin(); it != table.end(); ++it) {
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
  Forwarder forwarder;
  Name nameP("ndn:/strategy/P");
  Name nameQ("ndn:/strategy/Q");
  shared_ptr<Strategy> strategyP = make_shared<DummyStrategy>(ref(forwarder), nameP);
  shared_ptr<Strategy> strategyQ = make_shared<DummyStrategy>(ref(forwarder), nameQ);

  StrategyChoice& table = forwarder.getStrategyChoice();
  Measurements& measurements = forwarder.getMeasurements();

  // install
  table.install(strategyP);
  table.install(strategyQ);

  BOOST_CHECK(table.insert("ndn:/", nameP));
  // { '/'=>P }
  measurements.get("ndn:/")     ->getOrCreateStrategyInfo<PStrategyInfo>();
  measurements.get("ndn:/A")    ->getOrCreateStrategyInfo<PStrategyInfo>();
  measurements.get("ndn:/A/B")  ->getOrCreateStrategyInfo<PStrategyInfo>();
  measurements.get("ndn:/A/C")  ->getOrCreateStrategyInfo<PStrategyInfo>();

  BOOST_CHECK(table.insert("ndn:/A/B", nameP));
  // { '/'=>P, '/A/B'=>P }
  BOOST_CHECK( static_cast<bool>(measurements.get("ndn:/")   ->getStrategyInfo<PStrategyInfo>()));
  BOOST_CHECK( static_cast<bool>(measurements.get("ndn:/A")  ->getStrategyInfo<PStrategyInfo>()));
  BOOST_CHECK( static_cast<bool>(measurements.get("ndn:/A/B")->getStrategyInfo<PStrategyInfo>()));
  BOOST_CHECK( static_cast<bool>(measurements.get("ndn:/A/C")->getStrategyInfo<PStrategyInfo>()));

  BOOST_CHECK(table.insert("ndn:/A", nameQ));
  // { '/'=>P, '/A/B'=>P, '/A'=>Q }
  BOOST_CHECK( static_cast<bool>(measurements.get("ndn:/")   ->getStrategyInfo<PStrategyInfo>()));
  BOOST_CHECK(!static_cast<bool>(measurements.get("ndn:/A")  ->getStrategyInfo<PStrategyInfo>()));
  BOOST_CHECK( static_cast<bool>(measurements.get("ndn:/A/B")->getStrategyInfo<PStrategyInfo>()));
  BOOST_CHECK(!static_cast<bool>(measurements.get("ndn:/A/C")->getStrategyInfo<PStrategyInfo>()));

  table.erase("ndn:/A/B");
  // { '/'=>P, '/A'=>Q }
  BOOST_CHECK( static_cast<bool>(measurements.get("ndn:/")   ->getStrategyInfo<PStrategyInfo>()));
  BOOST_CHECK(!static_cast<bool>(measurements.get("ndn:/A")  ->getStrategyInfo<PStrategyInfo>()));
  BOOST_CHECK(!static_cast<bool>(measurements.get("ndn:/A/B")->getStrategyInfo<PStrategyInfo>()));
  BOOST_CHECK(!static_cast<bool>(measurements.get("ndn:/A/C")->getStrategyInfo<PStrategyInfo>()));
}

BOOST_AUTO_TEST_CASE(EraseNameTreeEntry)
{
  Forwarder forwarder;
  NameTree& nameTree = forwarder.getNameTree();
  StrategyChoice& table = forwarder.getStrategyChoice();

  Name nameP("ndn:/strategy/P");
  Name nameQ("ndn:/strategy/Q");
  shared_ptr<Strategy> strategyP = make_shared<DummyStrategy>(ref(forwarder), nameP);
  shared_ptr<Strategy> strategyQ = make_shared<DummyStrategy>(ref(forwarder), nameQ);
  table.install(strategyP);
  table.install(strategyQ);

  table.insert("ndn:/", nameP);

  size_t nNameTreeEntriesBefore = nameTree.size();

  table.insert("ndn:/A/B", nameQ);
  table.erase("ndn:/A/B");
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
  shared_ptr<Strategy> strategyP1 = make_shared<DummyStrategy>(ref(forwarder), nameP1);
  shared_ptr<Strategy> strategyP2 = make_shared<DummyStrategy>(ref(forwarder), nameP2);
  shared_ptr<Strategy> strategy3  = make_shared<DummyStrategy>(ref(forwarder), name3);
  shared_ptr<Strategy> strategy4  = make_shared<DummyStrategy>(ref(forwarder), name4);
  shared_ptr<Strategy> strategyQ  = make_shared<DummyStrategy>(ref(forwarder), nameQ);
  shared_ptr<Strategy> strategyQ5 = make_shared<DummyStrategy>(ref(forwarder), nameQ5);

  StrategyChoice& table = forwarder.getStrategyChoice();

  // install
  BOOST_CHECK_EQUAL(table.install(strategyP1), true);
  BOOST_CHECK_EQUAL(table.install(strategyP1), false);
  BOOST_CHECK_EQUAL(table.hasStrategy(nameP,  false), true);
  BOOST_CHECK_EQUAL(table.hasStrategy(nameP,  true),  false);
  BOOST_CHECK_EQUAL(table.hasStrategy(nameP1, true),  true);

  BOOST_CHECK_EQUAL(table.install(strategyP2), true);
  BOOST_CHECK_EQUAL(table.install(strategy3),  true);
  BOOST_CHECK_EQUAL(table.install(strategy4),  true);
  BOOST_CHECK_EQUAL(table.install(strategyQ),  true);
  BOOST_CHECK_EQUAL(table.install(strategyQ5), true);

  BOOST_CHECK(table.insert("ndn:/", nameQ));
  // exact match, { '/'=>Q }
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/").getName(), nameQ);

  BOOST_CHECK(table.insert("ndn:/", nameQ));
  BOOST_CHECK(table.insert("ndn:/", nameP));
  // { '/'=>P2 }
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/").getName(), nameP2);

  BOOST_CHECK(table.insert("ndn:/", nameQ));
  BOOST_CHECK(table.insert("ndn:/", nameP1));
  // { '/'=>P1 }
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/").getName(), nameP1);

  BOOST_CHECK(table.insert("ndn:/", nameQ));
  BOOST_CHECK(table.insert("ndn:/", nameP2));
  // { '/'=>P2 }
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/").getName(), nameP2);

  BOOST_CHECK(table.insert("ndn:/", nameQ));
  BOOST_CHECK(! table.insert("ndn:/", "ndn:/strategy/A"));
  // not installed
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/").getName(), nameQ);

  BOOST_CHECK(table.insert("ndn:/", nameQ));
  BOOST_CHECK(! table.insert("ndn:/", "ndn:/strategy/Z"));
  // not installed
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/").getName(), nameQ);

  BOOST_CHECK(table.insert("ndn:/", nameP1));
  BOOST_CHECK(table.insert("ndn:/", "ndn:/"));
  // match one component longer only, { '/'=>4 }
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/").getName(), name4);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
