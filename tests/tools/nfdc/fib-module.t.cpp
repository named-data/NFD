/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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
          <endpointId>1</endpointId>
          <cost>9</cost>
        </nextHop>
        <nextHop>
          <faceId>272</faceId>
          <endpointId>2</endpointId>
          <cost>50</cost>
        </nextHop>
        <nextHop>
          <faceId>274</faceId>
          <endpointId>3</endpointId>
          <cost>78</cost>
        </nextHop>
        <nextHop>
          <faceId>275</faceId>
          <endpointId>0</endpointId>
          <cost>80</cost>
        </nextHop>
      </nextHops>
    </fibEntry>
    <fibEntry>
      <prefix>/localhost/nfd</prefix>
      <nextHops>
        <nextHop>
          <faceId>1</faceId>
          <endpointId>0</endpointId>
          <cost>0</cost>
        </nextHop>
        <nextHop>
          <faceId>274</faceId>
          <endpointId>5</endpointId>
          <cost>0</cost>
        </nextHop>
      </nextHops>
    </fibEntry>
  </fib>
)XML");

const std::string STATUS_TEXT = std::string(R"TEXT(
FIB:
  / nexthops={face=262:1 (cost=9), face=272:2 (cost=50), face=274:3 (cost=78), face=275 (cost=80)}
  /localhost/nfd nexthops={face=1 (cost=0), face=274:5 (cost=0)}
)TEXT").substr(1);

BOOST_AUTO_TEST_CASE(Status)
{
  this->fetchStatus();
  FibEntry payload1;
  payload1.setPrefix("/")
          .addNextHopRecord(NextHopRecord().setFaceId(262).setEndpointId(1).setCost(9))
          .addNextHopRecord(NextHopRecord().setFaceId(272).setEndpointId(2).setCost(50))
          .addNextHopRecord(NextHopRecord().setFaceId(274).setEndpointId(3).setCost(78))
          .addNextHopRecord(NextHopRecord().setFaceId(275).setCost(80));
  FibEntry payload2;
  payload2.setPrefix("/localhost/nfd")
          .addNextHopRecord(NextHopRecord().setFaceId(1).setCost(0))
          .addNextHopRecord(NextHopRecord().setFaceId(274).setEndpointId(5).setCost(0));
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
