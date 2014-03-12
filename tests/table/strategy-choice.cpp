/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "table/strategy-choice.hpp"
#include "tests/fw/dummy-strategy.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TableStrategyChoice, BaseFixture)

using fw::Strategy;

BOOST_AUTO_TEST_CASE(Effective)
{
  Forwarder forwarder;
  Name nameP("ndn:/strategy/P");
  Name nameQ("ndn:/strategy/Q");
  Name nameZ("ndn:/strategy/Z");
  shared_ptr<Strategy> strategyP = make_shared<DummyStrategy>(boost::ref(forwarder), nameP);
  shared_ptr<Strategy> strategyQ = make_shared<DummyStrategy>(boost::ref(forwarder), nameQ);

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

class PStrategyInfo : public fw::StrategyInfo
{
};

BOOST_AUTO_TEST_CASE(ClearStrategyInfo)
{
  Forwarder forwarder;
  Name nameP("ndn:/strategy/P");
  Name nameQ("ndn:/strategy/Q");
  shared_ptr<Strategy> strategyP = make_shared<DummyStrategy>(boost::ref(forwarder), nameP);
  shared_ptr<Strategy> strategyQ = make_shared<DummyStrategy>(boost::ref(forwarder), nameQ);

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

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
