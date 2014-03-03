/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "table/strategy-choice.hpp"
#include "fw/strategy.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TableStrategyChoice, BaseFixture)

using fw::Strategy;

class StrategyChoiceTestDummyStrategy : public Strategy
{
public:
  StrategyChoiceTestDummyStrategy(Forwarder& forwarder, const Name& name)
    : Strategy(forwarder, name)
  {
  }

  virtual
  ~StrategyChoiceTestDummyStrategy()
  {
  }

  virtual void
  afterReceiveInterest(const Face& inFace,
                       const Interest& interest,
                       shared_ptr<fib::Entry> fibEntry,
                       shared_ptr<pit::Entry> pitEntry)
  {
  }
};

BOOST_AUTO_TEST_CASE(Effective)
{
  Forwarder forwarder;
  Name nameP("ndn:/strategy/P");
  Name nameQ("ndn:/strategy/Q");
  Name nameZ("ndn:/strategy/Z");
  shared_ptr<Strategy> strategyP = make_shared<StrategyChoiceTestDummyStrategy>(
                                   boost::ref(forwarder), nameP);
  shared_ptr<Strategy> strategyQ = make_shared<StrategyChoiceTestDummyStrategy>(
                                   boost::ref(forwarder), nameQ);

  StrategyChoice& table = forwarder.getStrategyChoice();

  // install
  BOOST_CHECK_EQUAL(table.install(strategyP), true);
  BOOST_CHECK_EQUAL(table.install(strategyQ), true);
  BOOST_CHECK_EQUAL(table.install(strategyQ), false);

  BOOST_CHECK(table.insert("ndn:/", "ndn:/strategy/P"));
  // { '/'=>P }

  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/")   .getName(), nameP);
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/A")  .getName(), nameP);
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/A/B").getName(), nameP);

  BOOST_CHECK(table.insert("ndn:/A/B", "ndn:/strategy/P"));
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

  BOOST_CHECK(table.insert("ndn:/A", "ndn:/strategy/Q"));
  // { '/'=>P, '/A/B'=>P, '/A'=>Q }

  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/")   .getName(), nameP);
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/A")  .getName(), nameQ);
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/A/B").getName(), nameP);

  table.erase("ndn:/A/B");
  // { '/'=>P, '/A'=>Q }

  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/")   .getName(), nameP);
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/A")  .getName(), nameQ);
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/A/B").getName(), nameQ);

  BOOST_CHECK(!table.insert("ndn:/", "ndn:/strategy/Z")); // non existent strategy

  BOOST_CHECK(table.insert("ndn:/", "ndn:/strategy/Q"));
  BOOST_CHECK(table.insert("ndn:/A", "ndn:/strategy/P"));
  // { '/'=>Q, '/A'=>P }

  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/")   .getName(), nameQ);
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/A")  .getName(), nameP);
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/A/B").getName(), nameP);
  BOOST_CHECK_EQUAL(table.findEffectiveStrategy("ndn:/D")  .getName(), nameQ);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
