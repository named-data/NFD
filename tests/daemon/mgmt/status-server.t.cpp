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

#include "mgmt/status-server.hpp"
#include "fw/forwarder.hpp"
#include "version.hpp"
#include "mgmt/internal-face.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/face/dummy-face.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(MgmtStatusServer, BaseFixture)

shared_ptr<const Data> g_response;

void
interceptResponse(const Data& data)
{
  g_response = data.shared_from_this();
}

BOOST_AUTO_TEST_CASE(Status)
{
  // initialize
  time::system_clock::TimePoint t1 = time::system_clock::now();
  Forwarder forwarder;
  shared_ptr<InternalFace> internalFace = make_shared<InternalFace>();
  internalFace->onReceiveData.connect(&interceptResponse);
  ndn::KeyChain keyChain;
  StatusServer statusServer(internalFace, ref(forwarder), keyChain);
  time::system_clock::TimePoint t2 = time::system_clock::now();

  // populate tables
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
  shared_ptr<Interest> request = makeInterest("ndn:/localhost/nfd/status");
  request->setMustBeFresh(true);
  request->setChildSelector(1);

  g_response.reset();
  time::system_clock::TimePoint t3 = time::system_clock::now();
  internalFace->sendInterest(*request);
  g_io.run_one();
  time::system_clock::TimePoint t4 = time::system_clock::now();
  BOOST_REQUIRE(static_cast<bool>(g_response));

  // verify
  ndn::nfd::ForwarderStatus status;
  BOOST_REQUIRE_NO_THROW(status.wireDecode(g_response->getContent()));

  BOOST_CHECK_EQUAL(status.getNfdVersion(), NFD_VERSION_BUILD_STRING);
  BOOST_CHECK_GE(time::toUnixTimestamp(status.getStartTimestamp()), time::toUnixTimestamp(t1));
  BOOST_CHECK_LE(time::toUnixTimestamp(status.getStartTimestamp()), time::toUnixTimestamp(t2));
  BOOST_CHECK_GE(time::toUnixTimestamp(status.getCurrentTimestamp()), time::toUnixTimestamp(t3));
  BOOST_CHECK_LE(time::toUnixTimestamp(status.getCurrentTimestamp()), time::toUnixTimestamp(t4));

  // StatusServer under test isn't added to Forwarder,
  // so request and response won't affect table size
  BOOST_CHECK_EQUAL(status.getNNameTreeEntries(), forwarder.getNameTree().size());
  BOOST_CHECK_EQUAL(status.getNFibEntries(), forwarder.getFib().size());
  BOOST_CHECK_EQUAL(status.getNPitEntries(), forwarder.getPit().size());
  BOOST_CHECK_EQUAL(status.getNMeasurementsEntries(), forwarder.getMeasurements().size());
  BOOST_CHECK_EQUAL(status.getNCsEntries(), forwarder.getCs().size());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
