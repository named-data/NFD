/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#include "nfdc/fib-module.hpp"

#include "status-fixture.hpp"

namespace nfd {
namespace tools {
namespace nfdc {
namespace tests {

BOOST_AUTO_TEST_SUITE(Nfdc)
BOOST_FIXTURE_TEST_SUITE(TestFibModule, StatusFixture<FibModule>)

const std::string STATUS_XML = stripXmlSpaces(R"XML(
  <fib>
    <fibEntry>
      <prefix>/</prefix>
      <nextHops>
        <nextHop>
          <faceId>262</faceId>
          <cost>9</cost>
        </nextHop>
        <nextHop>
          <faceId>272</faceId>
          <cost>50</cost>
        </nextHop>
        <nextHop>
          <faceId>274</faceId>
          <cost>78</cost>
        </nextHop>
      </nextHops>
    </fibEntry>
    <fibEntry>
      <prefix>/localhost/nfd</prefix>
      <nextHops>
        <nextHop>
          <faceId>1</faceId>
          <cost>0</cost>
        </nextHop>
        <nextHop>
          <faceId>274</faceId>
          <cost>0</cost>
        </nextHop>
      </nextHops>
    </fibEntry>
  </fib>
)XML");

const std::string STATUS_TEXT = std::string(R"TEXT(
FIB:
  / nexthops={faceid=262 (cost=9), faceid=272 (cost=50), faceid=274 (cost=78)}
  /localhost/nfd nexthops={faceid=1 (cost=0), faceid=274 (cost=0)}
)TEXT").substr(1);

BOOST_AUTO_TEST_CASE(Status)
{
  this->fetchStatus();
  FibEntry payload1;
  payload1.setPrefix("/")
          .addNextHopRecord(NextHopRecord().setFaceId(262).setCost(9))
          .addNextHopRecord(NextHopRecord().setFaceId(272).setCost(50))
          .addNextHopRecord(NextHopRecord().setFaceId(274).setCost(78));
  FibEntry payload2;
  payload2.setPrefix("/localhost/nfd")
          .addNextHopRecord(NextHopRecord().setFaceId(1).setCost(0))
          .addNextHopRecord(NextHopRecord().setFaceId(274).setCost(0));
  this->sendDataset("/localhost/nfd/fib/list", payload1, payload2);
  this->prepareStatusOutput();

  BOOST_CHECK(statusXml.is_equal(STATUS_XML));
  BOOST_CHECK(statusText.is_equal(STATUS_TEXT));
}

BOOST_AUTO_TEST_SUITE_END() // TestFibModule
BOOST_AUTO_TEST_SUITE_END() // Nfdc

} // namespace tests
} // namespace nfdc
} // namespace tools
} // namespace nfd
