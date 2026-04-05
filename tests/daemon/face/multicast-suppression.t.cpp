/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2026,  Regents of the University of California,
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

#include "face/multicast-suppression.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/global-io-fixture.hpp"

namespace nfd::tests {

using namespace nfd::face::ams;

BOOST_AUTO_TEST_SUITE(Face)

BOOST_AUTO_TEST_SUITE(TestNameTree)

BOOST_AUTO_TEST_CASE(DefaultConstruction)
{
  NameTree tree;
  BOOST_CHECK_EQUAL(tree.isLeaf, false);
}

BOOST_AUTO_TEST_CASE(InsertAndExactLookup)
{
  NameTree tree;
  tree.insert("/a/b", 100.0);

  double val = tree.longestPrefixMatch("/a/b");
  BOOST_CHECK_CLOSE(val, 100.0, 0.001);
}

BOOST_AUTO_TEST_CASE(LongerNameMatchesPrefix)
{
  NameTree tree;
  tree.insert("/a/b", 100.0);

  double val = tree.longestPrefixMatch("/a/b/c");
  BOOST_CHECK_CLOSE(val, 100.0, 0.001);
}

BOOST_AUTO_TEST_CASE(MultiplePrefixes)
{
  NameTree tree;
  tree.insert("/a/b", 50.0);
  tree.insert("/a/b/c", 200.0);

  // longest match is /a/b/c
  double val = tree.longestPrefixMatch("/a/b/c/d");
  BOOST_CHECK_CLOSE(val, 200.0, 0.001);

  // longest match is /a/b
  val = tree.longestPrefixMatch("/a/b/d");
  BOOST_CHECK_CLOSE(val, 50.0, 0.001);
}

BOOST_AUTO_TEST_CASE(NoMatch)
{
  NameTree tree;
  tree.insert("/a/b", 100.0);

  double val = tree.longestPrefixMatch("/x/y");
  // should return UNSET sentinel (very large value)
  BOOST_CHECK(val > 1000000.0);
}

BOOST_AUTO_TEST_CASE(SpecialCharacters)
{
  NameTree tree;
  tree.insert("/ndn/test-123/ABC", 75.0);

  double val = tree.longestPrefixMatch("/ndn/test-123/ABC/data");
  BOOST_CHECK_CLOSE(val, 75.0, 0.001);
}

BOOST_AUTO_TEST_CASE(EmptyAndRootOnlyName)
{
  NameTree tree;
  tree.insert("/a", 50.0);

  // empty name returns UNSET
  double val = tree.longestPrefixMatch("");
  BOOST_CHECK(val > 1000000.0);

  // root-only "/" has no components, returns UNSET
  val = tree.longestPrefixMatch("/");
  BOOST_CHECK(val > 1000000.0);
}

BOOST_AUTO_TEST_CASE(SuppressionTimeOnCorrectNode)
{
  NameTree tree;
  tree.insert("/a/b", 100.0);

  // exact match should find value on the /a/b node
  double val = tree.longestPrefixMatch("/a/b");
  BOOST_CHECK_CLOSE(val, 100.0, 0.001);

  // partial match /a should NOT have a value (only /a/b was inserted)
  val = tree.longestPrefixMatch("/a");
  BOOST_CHECK(val > 1000000.0);
}

BOOST_AUTO_TEST_CASE(GetSuppressionTimer)
{
  NameTree tree;
  tree.insert("/a/b", 100.0);

  auto timer = tree.getSuppressionTimer("/a/b/c");
  BOOST_CHECK(timer >= 0_ms);
  BOOST_CHECK(timer < time::milliseconds(200));
}

BOOST_AUTO_TEST_SUITE_END() // TestNameTree

BOOST_AUTO_TEST_SUITE(TestEMAMeasurements)

BOOST_AUTO_TEST_CASE(DefaultConstruction)
{
  EMAMeasurements ema;
  BOOST_CHECK_CLOSE(ema.getEMACurrent(), 0.0, 0.001);
  BOOST_CHECK_CLOSE(ema.getEMAPrev(), 0.0, 0.001);
  BOOST_CHECK_CLOSE(ema.getCurrentSuppressionTime(), 1.0, 0.001);
}

BOOST_AUTO_TEST_CASE(CustomConstruction)
{
  EMAMeasurements ema(5.0, 2, 50.0);
  BOOST_CHECK_CLOSE(ema.getEMACurrent(), 5.0, 0.001);
  BOOST_CHECK_CLOSE(ema.getCurrentSuppressionTime(), 50.0, 0.001);
}

BOOST_AUTO_TEST_CASE(FirstUpdate)
{
  EMAMeasurements ema;
  // duplicateCount <= m_lastDuplicateCount (1) to avoid ignore counter
  ema.addUpdateEMA(1, false);

  // first update sets EMA = duplicateCount directly (since current == 0)
  BOOST_CHECK_CLOSE(ema.getEMACurrent(), 1.0, 0.001);
}

BOOST_AUTO_TEST_CASE(ExponentialSmoothing)
{
  // start with EMA = 4.0 via constructor
  EMAMeasurements ema(4.0, 1, 50.0);

  // duplicateCount=1 <= m_lastDuplicateCount(1), no ignore
  // EMA = round((0.125*1 + 0.875*4.0)*10)/10 = round(36.25)/10 = 3.6
  ema.addUpdateEMA(1, false);

  BOOST_CHECK_CLOSE(ema.getEMACurrent(), 3.6, 1.0);
}

BOOST_AUTO_TEST_CASE(SuppressionTimeHoldsAboveThreshold)
{
  // EMA=3.0, well above DUPLICATE_THRESHOLD (1.3), suppression at 50ms
  EMAMeasurements ema(3.0, 1, 50.0);
  double initial = ema.getCurrentSuppressionTime();

  // EMA drops toward 1 but stays above threshold
  // updateDelayTime: EMA decreasing but still > threshold → else branch, no change
  ema.addUpdateEMA(1, true);

  BOOST_CHECK_CLOSE(ema.getCurrentSuppressionTime(), initial, 0.001);
}

BOOST_AUTO_TEST_CASE(SuppressionTimeDecreases)
{
  // EMA=1.0, which is <= DUPLICATE_THRESHOLD (1.3)
  EMAMeasurements ema(1.0, 1, 100.0);

  // duplicateCount=1 <= m_lastDuplicateCount(1), no ignore
  // EMA stays at 1.0, which is <= threshold and <= prev
  // triggers additive decrease: 100 - 5 = 95
  ema.addUpdateEMA(1, false);

  BOOST_CHECK(ema.getCurrentSuppressionTime() < 100.0);
}

BOOST_AUTO_TEST_SUITE_END() // TestEMAMeasurements

BOOST_FIXTURE_TEST_SUITE(TestMulticastSuppressionClass, GlobalIoTimeFixture)

BOOST_AUTO_TEST_CASE(RecordFirstInterest)
{
  MulticastSuppression ms;
  auto interest = makeInterest("/test/interest");

  BOOST_CHECK_EQUAL(ms.recordInterest(*interest), 1);
}

BOOST_AUTO_TEST_CASE(RecordDuplicateInterest)
{
  MulticastSuppression ms;
  auto interest = makeInterest("/test/interest");

  ms.recordInterest(*interest);
  BOOST_CHECK_EQUAL(ms.recordInterest(*interest), 0);
}

BOOST_AUTO_TEST_CASE(RecordFirstData)
{
  MulticastSuppression ms;
  auto data = makeData("/test/data");

  BOOST_CHECK_EQUAL(ms.recordData(*data), 1);
}

BOOST_AUTO_TEST_CASE(RecordDuplicateData)
{
  MulticastSuppression ms;
  auto data = makeData("/test/data");

  ms.recordData(*data);
  BOOST_CHECK_EQUAL(ms.recordData(*data), 0);
}

BOOST_AUTO_TEST_CASE(DuplicateCount)
{
  MulticastSuppression ms;
  auto interest = makeInterest("/test/dup");

  ms.recordInterest(*interest);
  ms.recordInterest(*interest);
  ms.recordInterest(*interest);

  BOOST_CHECK_EQUAL(ms.getDuplicateCount(interest->getName(), 'i'), 3);
}

BOOST_AUTO_TEST_CASE(DuplicateCountNonExistent)
{
  MulticastSuppression ms;
  BOOST_CHECK_EQUAL(ms.getDuplicateCount(Name("/nonexistent"), 'i'), 0);
}

BOOST_AUTO_TEST_CASE(InterestInflight)
{
  MulticastSuppression ms;
  auto interest = makeInterest("/test/inflight");

  BOOST_CHECK(!ms.interestInflight(*interest));
  ms.recordInterest(*interest);
  BOOST_CHECK(ms.interestInflight(*interest));
}

BOOST_AUTO_TEST_CASE(DataInflight)
{
  MulticastSuppression ms;
  auto data = makeData("/test/inflight");

  BOOST_CHECK(!ms.dataInflight(*data));
  ms.recordData(*data);
  BOOST_CHECK(ms.dataInflight(*data));
}

BOOST_AUTO_TEST_CASE(ForwardedStatus)
{
  MulticastSuppression ms;
  auto interest = makeInterest("/test/forward");

  BOOST_CHECK(!ms.getForwardedStatus(interest->getName(), 'i'));

  ms.recordInterest(*interest, true);
  BOOST_CHECK(ms.getForwardedStatus(interest->getName(), 'i'));
}

BOOST_AUTO_TEST_CASE(EntryExpiration)
{
  MulticastSuppression ms;
  auto interest = makeInterest("/test/expire");

  ms.recordInterest(*interest);
  BOOST_CHECK(ms.interestInflight(*interest));

  // advance past DEFAULT_INSTANT_LIFETIME (30ms)
  advanceClocks(10_ms, 5);

  BOOST_CHECK(!ms.interestInflight(*interest));
}

BOOST_AUTO_TEST_CASE(GetDelayTimer)
{
  MulticastSuppression ms;
  Name name("/test/delay/item");

  auto timer = ms.getDelayTimer(name, 'i');
  BOOST_CHECK(timer >= 0_ms);
}

BOOST_AUTO_TEST_SUITE_END() // TestMulticastSuppressionClass

BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace nfd::tests
