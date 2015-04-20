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

#include "table/cs.hpp"
#include "table/cs-policy-priority-fifo.hpp"
#include <ndn-cxx/util/crypto.hpp>

#include "tests/test-common.hpp"

namespace nfd {
namespace cs {
namespace tests {

using namespace nfd::tests;
using ndn::nfd::LocalControlHeader;

BOOST_AUTO_TEST_SUITE(CsPriorityFifo)

BOOST_FIXTURE_TEST_CASE(EvictOne, UnitTestTimeFixture)
{
  Cs cs(3);
  cs.setPolicy(unique_ptr<Policy>(new PriorityFifoPolicy()));

  shared_ptr<Data> dataA = makeData("ndn:/A");
  dataA->setFreshnessPeriod(time::milliseconds(99999));
  dataA->wireEncode();
  cs.insert(*dataA);

  shared_ptr<Data> dataB = makeData("ndn:/B");
  dataB->setFreshnessPeriod(time::milliseconds(10));
  dataB->wireEncode();
  cs.insert(*dataB);

  shared_ptr<Data> dataC = makeData("ndn:/C");
  dataC->setFreshnessPeriod(time::milliseconds(99999));
  dataC->wireEncode();
  cs.insert(*dataC, true);

  this->advanceClocks(time::milliseconds(11));

  // evict unsolicited
  shared_ptr<Data> dataD = makeData("ndn:/D");
  dataD->setFreshnessPeriod(time::milliseconds(99999));
  dataD->wireEncode();
  cs.insert(*dataD);
  BOOST_CHECK_EQUAL(cs.size(), 3);
  cs.find(Interest("ndn:/C"),
          bind([] { BOOST_CHECK(false); }),
          bind([] { BOOST_CHECK(true); }));

  // evict stale
  shared_ptr<Data> dataE = makeData("ndn:/E");
  dataE->setFreshnessPeriod(time::milliseconds(99999));
  dataE->wireEncode();
  cs.insert(*dataE);
  BOOST_CHECK_EQUAL(cs.size(), 3);
  cs.find(Interest("ndn:/B"),
          bind([] { BOOST_CHECK(false); }),
          bind([] { BOOST_CHECK(true); }));

  // evict fifo
  shared_ptr<Data> dataF = makeData("ndn:/F");
  dataF->setFreshnessPeriod(time::milliseconds(99999));
  dataF->wireEncode();
  cs.insert(*dataF);
  BOOST_CHECK_EQUAL(cs.size(), 3);
  cs.find(Interest("ndn:/A"),
          bind([] { BOOST_CHECK(false); }),
          bind([] { BOOST_CHECK(true); }));
}

BOOST_FIXTURE_TEST_CASE(Refresh, UnitTestTimeFixture)
{
  Cs cs(3);
  cs.setPolicy(unique_ptr<Policy>(new PriorityFifoPolicy()));

  shared_ptr<Data> dataA = makeData("ndn:/A");
  dataA->setFreshnessPeriod(time::milliseconds(99999));
  dataA->wireEncode();
  cs.insert(*dataA);

  shared_ptr<Data> dataB = makeData("ndn:/B");
  dataB->setFreshnessPeriod(time::milliseconds(10));
  dataB->wireEncode();
  cs.insert(*dataB);

  shared_ptr<Data> dataC = makeData("ndn:/C");
  dataC->setFreshnessPeriod(time::milliseconds(10));
  dataC->wireEncode();
  cs.insert(*dataC);

  this->advanceClocks(time::milliseconds(11));

  // refresh dataB
  shared_ptr<Data> dataB2 = make_shared<Data>(*dataB);
  dataB2->wireEncode();
  cs.insert(*dataB2);
  BOOST_CHECK_EQUAL(cs.size(), 3);
  cs.find(Interest("ndn:/A"),
          bind([] { BOOST_CHECK(true); }),
          bind([] { BOOST_CHECK(false); }));

  cs.find(Interest("ndn:/B"),
          bind([] { BOOST_CHECK(true); }),
          bind([] { BOOST_CHECK(false); }));

  cs.find(Interest("ndn:/C"),
          bind([] { BOOST_CHECK(true); }),
          bind([] { BOOST_CHECK(false); }));

  // evict dataC stale
  shared_ptr<Data> dataD = makeData("ndn:/D");
  dataD->setFreshnessPeriod(time::milliseconds(99999));
  dataD->wireEncode();
  cs.insert(*dataD);
  BOOST_CHECK_EQUAL(cs.size(), 3);
  cs.find(Interest("ndn:/C"),
          bind([] { BOOST_CHECK(false); }),
          bind([] { BOOST_CHECK(true); }));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace cs
} // namespace nfd
