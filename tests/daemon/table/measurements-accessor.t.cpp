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

#include "table/measurements-accessor.hpp"
#include "fw/strategy.hpp"

#include "tests/test-common.hpp"
#include "../fw/dummy-strategy.hpp"
#include "../fw/choose-strategy.hpp"

namespace nfd {
namespace measurements {
namespace tests {

using namespace nfd::tests;

class MeasurementsAccessorTestStrategy : public DummyStrategy
{
public:
  static void
  registerAs(const Name& strategyName)
  {
    registerAsImpl<MeasurementsAccessorTestStrategy>(strategyName);
  }

  MeasurementsAccessorTestStrategy(Forwarder& forwarder, const Name& name)
    : DummyStrategy(forwarder, name)
  {
  }

  MeasurementsAccessor&
  getMeasurementsAccessor()
  {
    return this->getMeasurements();
  }
};

class MeasurementsAccessorFixture : public BaseFixture
{
protected:
  MeasurementsAccessorFixture()
    : measurements(forwarder.getMeasurements())
  {
    const Name strategyP("/measurements-accessor-test-strategy-P/%FD%01");
    const Name strategyQ("/measurements-accessor-test-strategy-Q/%FD%01");
    MeasurementsAccessorTestStrategy::registerAs(strategyP);
    MeasurementsAccessorTestStrategy::registerAs(strategyQ);

    accessor1 = &choose<MeasurementsAccessorTestStrategy>(forwarder, "/", strategyP)
                  .getMeasurementsAccessor();
    accessor2 = &choose<MeasurementsAccessorTestStrategy>(forwarder, "/A", strategyQ)
                  .getMeasurementsAccessor();
    accessor3 = &choose<MeasurementsAccessorTestStrategy>(forwarder, "/A/B", strategyP)
                  .getMeasurementsAccessor();

    // Despite accessor1 and accessor3 are associated with the same strategy name,
    // they are different strategy instances and thus cannot access each other's measurements.
  }

protected:
  Forwarder forwarder;
  Measurements& measurements;
  MeasurementsAccessor* accessor1;
  MeasurementsAccessor* accessor2;
  MeasurementsAccessor* accessor3;
};

BOOST_AUTO_TEST_SUITE(Table)
BOOST_FIXTURE_TEST_SUITE(TestMeasurementsAccessor, MeasurementsAccessorFixture)

BOOST_AUTO_TEST_CASE(Get)
{
  BOOST_CHECK(accessor1->get("/"     ) != nullptr);
  BOOST_CHECK(accessor1->get("/A"    ) == nullptr);
  BOOST_CHECK(accessor1->get("/A/B"  ) == nullptr);
  BOOST_CHECK(accessor1->get("/A/B/C") == nullptr);
  BOOST_CHECK(accessor1->get("/A/D"  ) == nullptr);

  BOOST_CHECK(accessor2->get("/"     ) == nullptr);
  BOOST_CHECK(accessor2->get("/A"    ) != nullptr);
  BOOST_CHECK(accessor2->get("/A/B"  ) == nullptr);
  BOOST_CHECK(accessor2->get("/A/B/C") == nullptr);
  BOOST_CHECK(accessor2->get("/A/D"  ) != nullptr);

  BOOST_CHECK(accessor3->get("/"     ) == nullptr);
  BOOST_CHECK(accessor3->get("/A"    ) == nullptr);
  BOOST_CHECK(accessor3->get("/A/B"  ) != nullptr);
  BOOST_CHECK(accessor3->get("/A/B/C") != nullptr);
  BOOST_CHECK(accessor3->get("/A/D"  ) == nullptr);
}

BOOST_AUTO_TEST_CASE(GetParent)
{
  Entry& entryRoot = measurements.get("/");
  BOOST_CHECK(accessor1->getParent(entryRoot) == nullptr);
  BOOST_CHECK(accessor2->getParent(entryRoot) == nullptr);

  Entry& entryA = measurements.get("/A");
  BOOST_CHECK(accessor2->getParent(entryA) == nullptr);
  Entry& entryAD = measurements.get("/A/D");
  BOOST_CHECK(accessor2->getParent(entryAD) != nullptr);
  // whether accessor1 and accessor3 can getParent(entryA) and getParent(entryAD) is undefined,
  // because they shouldn't have obtained those entries in the first place
}

BOOST_AUTO_TEST_CASE(FindLongestPrefixMatch)
{
  shared_ptr<Interest> interest = makeInterest("/A/B/C");
  shared_ptr<pit::Entry> pitEntry = forwarder.getPit().insert(*interest).first;

  measurements.get("/");
  BOOST_CHECK(accessor1->findLongestPrefixMatch("/A/B") != nullptr);
  BOOST_CHECK(accessor1->findLongestPrefixMatch(*pitEntry) != nullptr);

  measurements.get("/A");
  BOOST_CHECK(accessor1->findLongestPrefixMatch("/A/B") == nullptr);
  BOOST_CHECK(accessor1->findLongestPrefixMatch(*pitEntry) == nullptr);
}

BOOST_AUTO_TEST_CASE(FindExactMatch)
{
  measurements.get("/");
  measurements.get("/A");
  measurements.get("/A/B");
  measurements.get("/A/B/C");
  measurements.get("/A/D");

  BOOST_CHECK(accessor1->findExactMatch("/"     ) != nullptr);
  BOOST_CHECK(accessor1->findExactMatch("/A"    ) == nullptr);
  BOOST_CHECK(accessor1->findExactMatch("/A/B"  ) == nullptr);
  BOOST_CHECK(accessor1->findExactMatch("/A/B/C") == nullptr);
  BOOST_CHECK(accessor1->findExactMatch("/A/D"  ) == nullptr);
  BOOST_CHECK(accessor1->findExactMatch("/A/E"  ) == nullptr);
  BOOST_CHECK(accessor1->findExactMatch("/F"    ) == nullptr);

  BOOST_CHECK(accessor2->findExactMatch("/"     ) == nullptr);
  BOOST_CHECK(accessor2->findExactMatch("/A"    ) != nullptr);
  BOOST_CHECK(accessor2->findExactMatch("/A/B"  ) == nullptr);
  BOOST_CHECK(accessor2->findExactMatch("/A/B/C") == nullptr);
  BOOST_CHECK(accessor2->findExactMatch("/A/D"  ) != nullptr);
  BOOST_CHECK(accessor2->findExactMatch("/A/E"  ) == nullptr);
  BOOST_CHECK(accessor2->findExactMatch("/F"    ) == nullptr);

  BOOST_CHECK(accessor3->findExactMatch("/"     ) == nullptr);
  BOOST_CHECK(accessor3->findExactMatch("/A"    ) == nullptr);
  BOOST_CHECK(accessor3->findExactMatch("/A/B"  ) != nullptr);
  BOOST_CHECK(accessor3->findExactMatch("/A/B/C") != nullptr);
  BOOST_CHECK(accessor3->findExactMatch("/A/D"  ) == nullptr);
  BOOST_CHECK(accessor3->findExactMatch("/A/E"  ) == nullptr);
  BOOST_CHECK(accessor3->findExactMatch("/F"    ) == nullptr);
}

BOOST_AUTO_TEST_SUITE_END() // TestMeasurementsAccessor
BOOST_AUTO_TEST_SUITE_END() // Table

} // namespace tests
} // namespace measurements
} // namespace nfd
