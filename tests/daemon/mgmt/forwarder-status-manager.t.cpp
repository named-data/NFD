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

#include "mgmt/forwarder-status-manager.hpp"
#include "fw/forwarder.hpp"
#include "version.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/face/dummy-face.hpp"
#include <ndn-cxx/util/dummy-client-face.hpp>
#include <ndn-cxx/mgmt/dispatcher.hpp>

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(Mgmt, UnitTestTimeFixture)
BOOST_AUTO_TEST_SUITE(TestForwarderStatusManager)

BOOST_AUTO_TEST_CASE(Status)
{
  // initialize
  time::system_clock::TimePoint t1 = time::system_clock::now();
  Forwarder forwarder;
  auto face = ndn::util::makeDummyClientFace(g_io, {true, true});
  ndn::KeyChain keyChain;
  ndn::mgmt::Dispatcher dispatcher(*face, ref(keyChain));
  ForwarderStatusManager statusServer(ref(forwarder), ref(dispatcher));
  dispatcher.addTopPrefix("/localhost/nfd");
  advanceClocks(time::milliseconds(1));

  // populate tables
  time::system_clock::TimePoint t2 = time::system_clock::now();
  forwarder.getFib().insert("ndn:/fib1");
  forwarder.getPit().insert(*makeInterest("ndn:/pit1"));
  forwarder.getPit().insert(*makeInterest("ndn:/pit2"));
  forwarder.getPit().insert(*makeInterest("ndn:/pit3"));
  forwarder.getPit().insert(*makeInterest("ndn:/pit4"));
  forwarder.getMeasurements().get("ndn:/measurements1");
  forwarder.getMeasurements().get("ndn:/measurements2");
  forwarder.getMeasurements().get("ndn:/measurements3");
  BOOST_CHECK_GE(forwarder.getFib().size(), 1);
  BOOST_CHECK_GE(forwarder.getPit().size(), 4);
  BOOST_CHECK_GE(forwarder.getMeasurements().size(), 3);

  // request
  time::system_clock::TimePoint t3 = time::system_clock::now();
  auto request = makeInterest("ndn:/localhost/nfd/status");
  request->setMustBeFresh(true);
  request->setChildSelector(1);

  face->receive<Interest>(*request);
  advanceClocks(time::milliseconds(1));

  // verify
  time::system_clock::TimePoint t4 = time::system_clock::now();
  BOOST_REQUIRE_EQUAL(face->sentDatas.size(), 1);
  ndn::nfd::ForwarderStatus status;
  BOOST_REQUIRE_NO_THROW(status.wireDecode(face->sentDatas[0].getContent()));

  BOOST_CHECK_EQUAL(status.getNfdVersion(), NFD_VERSION_BUILD_STRING);
  BOOST_CHECK_GE(time::toUnixTimestamp(status.getStartTimestamp()), time::toUnixTimestamp(t1));
  BOOST_CHECK_LE(time::toUnixTimestamp(status.getStartTimestamp()), time::toUnixTimestamp(t2));
  BOOST_CHECK_GE(time::toUnixTimestamp(status.getCurrentTimestamp()), time::toUnixTimestamp(t3));
  BOOST_CHECK_LE(time::toUnixTimestamp(status.getCurrentTimestamp()), time::toUnixTimestamp(t4));

  // Interest/Data toward ForwarderStatusManager don't go through Forwarder,
  // so request and response won't affect table size
  BOOST_CHECK_EQUAL(status.getNNameTreeEntries(), forwarder.getNameTree().size());
  BOOST_CHECK_EQUAL(status.getNFibEntries(), forwarder.getFib().size());
  BOOST_CHECK_EQUAL(status.getNPitEntries(), forwarder.getPit().size());
  BOOST_CHECK_EQUAL(status.getNMeasurementsEntries(), forwarder.getMeasurements().size());
  BOOST_CHECK_EQUAL(status.getNCsEntries(), forwarder.getCs().size());
}

BOOST_AUTO_TEST_SUITE_END() // TestForwarderStatusManager
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace tests
} // namespace nfd
