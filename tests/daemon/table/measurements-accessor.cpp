/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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

BOOST_FIXTURE_TEST_SUITE(TableMeasurementsAccessor, BaseFixture)

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
  getMeasurements_accessor()
  {
    return this->getMeasurements();
  }
};

BOOST_AUTO_TEST_CASE(Access)
{
  Forwarder forwarder;

  auto strategy1 = make_shared<MeasurementsAccessorTestStrategy>(ref(forwarder), "ndn:/strategy1");
  auto strategy2 = make_shared<MeasurementsAccessorTestStrategy>(ref(forwarder), "ndn:/strategy2");

  Name nameRoot("ndn:/");
  Name nameA   ("ndn:/A");
  Name nameAB  ("ndn:/A/B");
  Name nameABC ("ndn:/A/B/C");
  Name nameAD  ("ndn:/A/D");

  StrategyChoice& strategyChoice = forwarder.getStrategyChoice();
  strategyChoice.install(strategy1);
  strategyChoice.install(strategy2);
  strategyChoice.insert(nameRoot, strategy1->getName());
  strategyChoice.insert(nameA   , strategy2->getName());
  strategyChoice.insert(nameAB  , strategy1->getName());

  MeasurementsAccessor& accessor1 = strategy1->getMeasurements_accessor();
  MeasurementsAccessor& accessor2 = strategy2->getMeasurements_accessor();

  BOOST_CHECK_EQUAL(static_cast<bool>(accessor1.get(nameRoot)), true);
  BOOST_CHECK_EQUAL(static_cast<bool>(accessor1.get(nameA   )), false);
  BOOST_CHECK_EQUAL(static_cast<bool>(accessor1.get(nameAB  )), true);
  BOOST_CHECK_EQUAL(static_cast<bool>(accessor1.get(nameABC )), true);
  BOOST_CHECK_EQUAL(static_cast<bool>(accessor1.get(nameAD  )), false);

  shared_ptr<measurements::Entry> entryRoot = forwarder.getMeasurements().get(nameRoot);
  BOOST_CHECK_NO_THROW(accessor1.getParent(*entryRoot));

  BOOST_CHECK_EQUAL(static_cast<bool>(accessor2.get(nameRoot)), false);
  BOOST_CHECK_EQUAL(static_cast<bool>(accessor2.get(nameA   )), true);
  BOOST_CHECK_EQUAL(static_cast<bool>(accessor2.get(nameAB  )), false);
  BOOST_CHECK_EQUAL(static_cast<bool>(accessor2.get(nameABC )), false);
  BOOST_CHECK_EQUAL(static_cast<bool>(accessor2.get(nameAD  )), true);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
