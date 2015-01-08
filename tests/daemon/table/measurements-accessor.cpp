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

#include "table/measurements-accessor.hpp"
#include "fw/strategy.hpp"

#include "tests/test-common.hpp"
#include "../fw/dummy-strategy.hpp"

namespace nfd {
namespace tests {

using measurements::Entry;

class MeasurementsAccessorTestStrategy : public DummyStrategy
{
public:
  MeasurementsAccessorTestStrategy(Forwarder& forwarder, const Name& name)
    : DummyStrategy(forwarder, name)
  {
  }

  virtual
  ~MeasurementsAccessorTestStrategy()
  {
  }

public: // accessors
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
    : strategy1(make_shared<MeasurementsAccessorTestStrategy>(ref(forwarder), "ndn:/strategy1"))
    , strategy2(make_shared<MeasurementsAccessorTestStrategy>(ref(forwarder), "ndn:/strategy2"))
    , measurements(forwarder.getMeasurements())
    , accessor1(strategy1->getMeasurementsAccessor())
    , accessor2(strategy2->getMeasurementsAccessor())
  {
    StrategyChoice& strategyChoice = forwarder.getStrategyChoice();
    strategyChoice.install(strategy1);
    strategyChoice.install(strategy2);
    strategyChoice.insert("/"   , strategy1->getName());
    strategyChoice.insert("/A"  , strategy2->getName());
    strategyChoice.insert("/A/B", strategy1->getName());
  }

protected:
  Forwarder forwarder;
  shared_ptr<MeasurementsAccessorTestStrategy> strategy1;
  shared_ptr<MeasurementsAccessorTestStrategy> strategy2;
  Measurements& measurements;
  MeasurementsAccessor& accessor1;
  MeasurementsAccessor& accessor2;
};

BOOST_FIXTURE_TEST_SUITE(TableMeasurementsAccessor, MeasurementsAccessorFixture)

BOOST_AUTO_TEST_CASE(Get)
{
  BOOST_CHECK(accessor1.get("/"     ) != nullptr);
  BOOST_CHECK(accessor1.get("/A"    ) == nullptr);
  BOOST_CHECK(accessor1.get("/A/B"  ) != nullptr);
  BOOST_CHECK(accessor1.get("/A/B/C") != nullptr);
  BOOST_CHECK(accessor1.get("/A/D"  ) == nullptr);

  BOOST_CHECK(accessor2.get("/"     ) == nullptr);
  BOOST_CHECK(accessor2.get("/A"    ) != nullptr);
  BOOST_CHECK(accessor2.get("/A/B"  ) == nullptr);
  BOOST_CHECK(accessor2.get("/A/B/C") == nullptr);
  BOOST_CHECK(accessor2.get("/A/D"  ) != nullptr);
}

BOOST_AUTO_TEST_CASE(GetParent)
{
  shared_ptr<Entry> entryRoot = measurements.get("/");
  BOOST_CHECK(accessor1.getParent(*entryRoot) == nullptr);
  BOOST_CHECK(accessor2.getParent(*entryRoot) == nullptr);

  shared_ptr<Entry> entryABC = measurements.get("/A/B/C");
  BOOST_CHECK(accessor1.getParent(*entryABC) != nullptr);
  BOOST_CHECK(accessor2.getParent(*entryABC) == nullptr);

  shared_ptr<Entry> entryAB = measurements.get("/A/B");
  BOOST_CHECK(accessor1.getParent(*entryAB) == nullptr);
  // whether accessor2.getParent(*entryAB) can return an Entry is undefined,
  // because strategy2 shouldn't obtain entryAB in the first place
}

BOOST_AUTO_TEST_CASE(FindLongestPrefixMatch)
{
  shared_ptr<Interest> interest = makeInterest("/A/B/C");
  shared_ptr<pit::Entry> pitEntry = forwarder.getPit().insert(*interest).first;

  measurements.get("/");
  BOOST_CHECK(accessor1.findLongestPrefixMatch("/A/B") != nullptr);
  BOOST_CHECK(accessor1.findLongestPrefixMatch(*pitEntry) != nullptr);

  measurements.get("/A");
  BOOST_CHECK(accessor1.findLongestPrefixMatch("/A/B") == nullptr);
  BOOST_CHECK(accessor1.findLongestPrefixMatch(*pitEntry) == nullptr);
}

BOOST_AUTO_TEST_CASE(FindExactMatch)
{
  measurements.get("/");
  measurements.get("/A");
  measurements.get("/A/B");
  measurements.get("/A/B/C");
  measurements.get("/A/D");

  BOOST_CHECK(accessor1.findExactMatch("/"     ) != nullptr);
  BOOST_CHECK(accessor1.findExactMatch("/A"    ) == nullptr);
  BOOST_CHECK(accessor1.findExactMatch("/A/B"  ) != nullptr);
  BOOST_CHECK(accessor1.findExactMatch("/A/B/C") != nullptr);
  BOOST_CHECK(accessor1.findExactMatch("/A/D"  ) == nullptr);
  BOOST_CHECK(accessor1.findExactMatch("/A/E"  ) == nullptr);
  BOOST_CHECK(accessor1.findExactMatch("/F"    ) == nullptr);

  BOOST_CHECK(accessor2.findExactMatch("/"     ) == nullptr);
  BOOST_CHECK(accessor2.findExactMatch("/A"    ) != nullptr);
  BOOST_CHECK(accessor2.findExactMatch("/A/B"  ) == nullptr);
  BOOST_CHECK(accessor2.findExactMatch("/A/B/C") == nullptr);
  BOOST_CHECK(accessor2.findExactMatch("/A/D"  ) != nullptr);
  BOOST_CHECK(accessor2.findExactMatch("/A/E"  ) == nullptr);
  BOOST_CHECK(accessor2.findExactMatch("/F"    ) == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
