/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "table/measurements-accessor.hpp"
#include "fw/strategy.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TableMeasurementsAccessor, BaseFixture)

class MeasurementsAccessorTestStrategy : public fw::Strategy
{
public:
  explicit
  MeasurementsAccessorTestStrategy(Forwarder& forwarder)
    : Strategy(forwarder, "ndn:/MeasurementsAccessorTestStrategy")
  {
  }

  virtual
  ~MeasurementsAccessorTestStrategy()

  {
  }

  virtual void
  afterReceiveInterest(const Face& inFace,
                       const Interest& interest,
                       shared_ptr<fib::Entry> fibEntry,
                       shared_ptr<pit::Entry> pitEntry)
  {
    BOOST_ASSERT(false);
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

  shared_ptr<MeasurementsAccessorTestStrategy> strategy1 =
    make_shared<MeasurementsAccessorTestStrategy>(boost::ref(forwarder));
  shared_ptr<MeasurementsAccessorTestStrategy> strategy2 =
    make_shared<MeasurementsAccessorTestStrategy>(boost::ref(forwarder));

  Name nameRoot("ndn:/");
  Name nameA   ("ndn:/A");
  Name nameAB  ("ndn:/A/B");
  Name nameABC ("ndn:/A/B/C");
  Name nameAD  ("ndn:/A/D");

  Fib& fib = forwarder.getFib();
  fib.insert(nameRoot).first->setStrategy(strategy1);
  fib.insert(nameA   ).first->setStrategy(strategy2);
  fib.insert(nameAB  ).first->setStrategy(strategy1);

  MeasurementsAccessor& accessor1 = strategy1->getMeasurements_accessor();
  MeasurementsAccessor& accessor2 = strategy2->getMeasurements_accessor();

  BOOST_CHECK_EQUAL(static_cast<bool>(accessor1.get(nameRoot)), true);
  BOOST_CHECK_EQUAL(static_cast<bool>(accessor1.get(nameA   )), false);
  BOOST_CHECK_EQUAL(static_cast<bool>(accessor1.get(nameAB  )), true);
  BOOST_CHECK_EQUAL(static_cast<bool>(accessor1.get(nameABC )), true);
  BOOST_CHECK_EQUAL(static_cast<bool>(accessor1.get(nameAD  )), false);

  shared_ptr<measurements::Entry> entryRoot = forwarder.getMeasurements().get(nameRoot);
  BOOST_CHECK_NO_THROW(accessor1.getParent(entryRoot));

  BOOST_CHECK_EQUAL(static_cast<bool>(accessor2.get(nameRoot)), false);
  BOOST_CHECK_EQUAL(static_cast<bool>(accessor2.get(nameA   )), true);
  BOOST_CHECK_EQUAL(static_cast<bool>(accessor2.get(nameAB  )), false);
  BOOST_CHECK_EQUAL(static_cast<bool>(accessor2.get(nameABC )), false);
  BOOST_CHECK_EQUAL(static_cast<bool>(accessor2.get(nameAD  )), true);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
