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

namespace nfd {
namespace tests {

class StrategyChoiceFixture : public BaseFixture
{
protected:
  StrategyChoiceFixture()
    : sc(forwarder.getStrategyChoice())
    , strategyNameP("/strategy-choice-P/%FD%00")
    , strategyNameQ("/strategy-choice-Q/%FD%00")
  {
    DummyStrategy::registerAs(strategyNameP);
    DummyStrategy::registerAs(strategyNameQ);
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

  template<typename Q>
  Name
  findInstanceName(const Q& query)
  {
    return sc.findEffectiveStrategy(query).getInstanceName();
  }

protected:
  Forwarder forwarder;
  StrategyChoice& sc;

  const Name strategyNameP;
  const Name strategyNameQ;
};

BOOST_AUTO_TEST_SUITE(Table)
BOOST_FIXTURE_TEST_SUITE(TestStrategyChoice, StrategyChoiceFixture)

using fw::Strategy;

BOOST_AUTO_TEST_CASE(Parameters)
{
  // no parameters
  BOOST_CHECK_EQUAL(this->insertAndGet("/A", strategyNameP), strategyNameP);

  // one parameter
  Name oneParamName = Name(strategyNameP).append("param");
  BOOST_CHECK_EQUAL(this->insertAndGet("/B", oneParamName), oneParamName);

  // two parameters
  Name twoParamName = Name(strategyNameP).append("x").append("y");
  BOOST_CHECK_EQUAL(this->insertAndGet("/C", twoParamName), twoParamName);

  // parameter without version is disallowed
  Name oneParamUnversioned = strategyNameP.getPrefix(-1).append("param");
  BOOST_CHECK_EQUAL(sc.insert("/D", oneParamUnversioned), false);
}

BOOST_AUTO_TEST_CASE(Get)
{
  BOOST_CHECK(sc.insert("/", strategyNameP));
  // { '/'=>P }

  auto getRoot = sc.get("/");
  BOOST_CHECK_EQUAL(getRoot.first, true);
  BOOST_CHECK_EQUAL(getRoot.second, strategyNameP);

  auto getA = sc.get("/A");
  BOOST_CHECK_EQUAL(getA.first, false);
}

BOOST_AUTO_TEST_CASE(FindEffectiveStrategy)
{
  const Name strategyNameZ("/strategy-choice-Z/%FD%00"); // unregistered strategyName

  BOOST_CHECK(sc.insert("/", strategyNameP));
  // { '/'=>P }

  BOOST_CHECK_EQUAL(this->findInstanceName("/"), strategyNameP);
  BOOST_CHECK_EQUAL(this->findInstanceName("/A"), strategyNameP);
  BOOST_CHECK_EQUAL(this->findInstanceName("/A/B"), strategyNameP);

  BOOST_CHECK(sc.insert("/A/B", strategyNameP));
  // { '/'=>P, '/A/B'=>P }

  BOOST_CHECK_EQUAL(this->findInstanceName("/"), strategyNameP);
  BOOST_CHECK_EQUAL(this->findInstanceName("/A"), strategyNameP);
  BOOST_CHECK_EQUAL(this->findInstanceName("/A/B"), strategyNameP);
  // same entry, same instance
  BOOST_CHECK_EQUAL(&sc.findEffectiveStrategy("/"), &sc.findEffectiveStrategy("/A"));
  // different entries, distinct instances
  BOOST_CHECK_NE(&sc.findEffectiveStrategy("/"), &sc.findEffectiveStrategy("/A/B"));

  sc.erase("/A"); // no effect
  // { '/'=>P, '/A/B'=>P }

  BOOST_CHECK_EQUAL(this->findInstanceName("/"), strategyNameP);
  BOOST_CHECK_EQUAL(this->findInstanceName("/A"), strategyNameP);
  BOOST_CHECK_EQUAL(this->findInstanceName("/A/B"), strategyNameP);

  BOOST_CHECK(sc.insert("/A", strategyNameQ));
  // { '/'=>P, '/A/B'=>P, '/A'=>Q }

  BOOST_CHECK_EQUAL(this->findInstanceName("/"), strategyNameP);
  BOOST_CHECK_EQUAL(this->findInstanceName("/A"), strategyNameQ);
  BOOST_CHECK_EQUAL(this->findInstanceName("/A/B"), strategyNameP);

  sc.erase("/A/B");
  // { '/'=>P, '/A'=>Q }

  BOOST_CHECK_EQUAL(this->findInstanceName("/"), strategyNameP);
  BOOST_CHECK_EQUAL(this->findInstanceName("/A"), strategyNameQ);
  BOOST_CHECK_EQUAL(this->findInstanceName("/A/B"), strategyNameQ);

  BOOST_CHECK(!sc.insert("/", strategyNameZ)); // non existent strategy

  BOOST_CHECK(sc.insert("/", strategyNameQ));
  BOOST_CHECK(sc.insert("/A", strategyNameP));
  // { '/'=>Q, '/A'=>P }

  BOOST_CHECK_EQUAL(this->findInstanceName("/"), strategyNameQ);
  BOOST_CHECK_EQUAL(this->findInstanceName("/A"), strategyNameP);
  BOOST_CHECK_EQUAL(this->findInstanceName("/A/B"), strategyNameP);
  BOOST_CHECK_EQUAL(this->findInstanceName("/D"), strategyNameQ);
}

BOOST_AUTO_TEST_CASE(FindEffectiveStrategyWithPitEntry)
{
  shared_ptr<Data> dataABC = makeData("/A/B/C");
  Name fullName = dataABC->getFullName();

  BOOST_CHECK(sc.insert("/A", strategyNameP));
  BOOST_CHECK(sc.insert(fullName, strategyNameQ));

  Pit& pit = forwarder.getPit();
  shared_ptr<Interest> interestAB = makeInterest("/A/B");
  shared_ptr<pit::Entry> pitAB = pit.insert(*interestAB).first;
  shared_ptr<Interest> interestFull = makeInterest(fullName);
  shared_ptr<pit::Entry> pitFull = pit.insert(*interestFull).first;

  BOOST_CHECK_EQUAL(this->findInstanceName(*pitAB), strategyNameP);
  BOOST_CHECK_EQUAL(this->findInstanceName(*pitFull), strategyNameQ);
}

BOOST_AUTO_TEST_CASE(FindEffectiveStrategyWithMeasurementsEntry)
{
  BOOST_CHECK(sc.insert("/A", strategyNameP));
  BOOST_CHECK(sc.insert("/A/B/C", strategyNameQ));

  Measurements& measurements = forwarder.getMeasurements();
  measurements::Entry& mAB = measurements.get("/A/B");
  measurements::Entry& mABCD = measurements.get("/A/B/C/D");

  BOOST_CHECK_EQUAL(this->findInstanceName(mAB), strategyNameP);
  BOOST_CHECK_EQUAL(this->findInstanceName(mABCD), strategyNameQ);
}

BOOST_AUTO_TEST_CASE(Erase)
{
  NameTree& nameTree = forwarder.getNameTree();

  sc.insert("/", strategyNameP);

  size_t nNameTreeEntriesBefore = nameTree.size();

  sc.insert("/A/B", strategyNameQ);
  sc.erase("/A/B");
  BOOST_CHECK_EQUAL(nameTree.size(), nNameTreeEntriesBefore);
}

BOOST_AUTO_TEST_CASE(Enumerate)
{
  sc.insert("/",      strategyNameP);
  sc.insert("/A/B",   strategyNameQ);
  sc.insert("/A/B/C", strategyNameP);
  sc.insert("/D",     strategyNameP);
  sc.insert("/E",     strategyNameQ);

  BOOST_CHECK_EQUAL(sc.size(), 5);

  std::map<Name, Name> map; // namespace=>strategyName
  for (StrategyChoice::const_iterator it = sc.begin(); it != sc.end(); ++it) {
    map[it->getPrefix()] = it->getStrategyName();
  }

  BOOST_CHECK_EQUAL(map.at("/"),      strategyNameP);
  BOOST_CHECK_EQUAL(map.at("/A/B"),   strategyNameQ);
  BOOST_CHECK_EQUAL(map.at("/A/B/C"), strategyNameP);
  BOOST_CHECK_EQUAL(map.at("/D"),     strategyNameP);
  BOOST_CHECK_EQUAL(map.at("/E"),     strategyNameQ);
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

BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES(ClearStrategyInfo, 2)
BOOST_AUTO_TEST_CASE(ClearStrategyInfo)
{
  Measurements& measurements = forwarder.getMeasurements();

  BOOST_CHECK(sc.insert("/", strategyNameP));
  // { '/'=>P }
  measurements.get("/").insertStrategyInfo<PStrategyInfo>();
  measurements.get("/A").insertStrategyInfo<PStrategyInfo>();
  measurements.get("/A/B").insertStrategyInfo<PStrategyInfo>();
  measurements.get("/A/C").insertStrategyInfo<PStrategyInfo>();

  BOOST_CHECK(sc.insert("/A/B", strategyNameP));
  // { '/'=>P, '/A/B'=>P }
  BOOST_CHECK(measurements.get("/").getStrategyInfo<PStrategyInfo>() != nullptr);
  BOOST_CHECK(measurements.get("/A").getStrategyInfo<PStrategyInfo>() != nullptr);
  BOOST_CHECK(measurements.get("/A/B").getStrategyInfo<PStrategyInfo>() != nullptr); // expected failure
  BOOST_CHECK(measurements.get("/A/C").getStrategyInfo<PStrategyInfo>() != nullptr);

  BOOST_CHECK(sc.insert("/A", strategyNameQ));
  // { '/'=>P, '/A/B'=>P, '/A'=>Q }
  BOOST_CHECK(measurements.get("/").getStrategyInfo<PStrategyInfo>() != nullptr);
  BOOST_CHECK(measurements.get("/A").getStrategyInfo<PStrategyInfo>() == nullptr);
  BOOST_CHECK(measurements.get("/A/B").getStrategyInfo<PStrategyInfo>() != nullptr); // expected failure
  BOOST_CHECK(measurements.get("/A/C").getStrategyInfo<PStrategyInfo>() == nullptr);

  sc.erase("/A/B");
  // { '/'=>P, '/A'=>Q }
  BOOST_CHECK(measurements.get("/").getStrategyInfo<PStrategyInfo>() != nullptr);
  BOOST_CHECK(measurements.get("/A").getStrategyInfo<PStrategyInfo>() == nullptr);
  BOOST_CHECK(measurements.get("/A/B").getStrategyInfo<PStrategyInfo>() == nullptr);
  BOOST_CHECK(measurements.get("/A/C").getStrategyInfo<PStrategyInfo>() == nullptr);
}

BOOST_AUTO_TEST_SUITE_END() // TestStrategyChoice
BOOST_AUTO_TEST_SUITE_END() // Table

} // namespace tests
} // namespace nfd
