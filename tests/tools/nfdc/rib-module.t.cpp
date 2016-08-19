/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#include "nfdc/rib-module.hpp"

#include "module-fixture.hpp"

namespace nfd {
namespace tools {
namespace nfdc {
namespace tests {

BOOST_AUTO_TEST_SUITE(Nfdc)
BOOST_FIXTURE_TEST_SUITE(TestRibModule, ModuleFixture<RibModule>)

const std::string STATUS_XML = stripXmlSpaces(R"XML(
  <rib>
    <ribEntry>
      <prefix>/</prefix>
      <routes>
        <route>
          <faceId>262</faceId>
          <origin>255</origin>
          <cost>9</cost>
          <flags>
            <ribCapture/>
          </flags>
        </route>
        <route>
          <faceId>272</faceId>
          <origin>255</origin>
          <cost>50</cost>
          <flags/>
        </route>
        <route>
          <faceId>274</faceId>
          <origin>255</origin>
          <cost>78</cost>
          <flags>
            <childInherit/>
            <ribCapture/>
          </flags>
        </route>
        <route>
          <faceId>276</faceId>
          <origin>255</origin>
          <cost>79</cost>
          <flags>
            <childInherit/>
          </flags>
          <expirationPeriod>PT47S</expirationPeriod>
        </route>
      </routes>
    </ribEntry>
    <ribEntry>
      <prefix>/localhost/nfd</prefix>
      <routes>
        <route>
          <faceId>258</faceId>
          <origin>0</origin>
          <cost>0</cost>
          <flags>
            <childInherit/>
          </flags>
        </route>
      </routes>
    </ribEntry>
  </rib>
)XML");

const std::string STATUS_TEXT =
  "RIB:\n"
  "  / route={faceid=262 (origin=255 cost=9 RibCapture), faceid=272 (origin=255 cost=50), "
    "faceid=274 (origin=255 cost=78 ChildInherit RibCapture), "
    "faceid=276 (origin=255 cost=79 expires=47s ChildInherit)}\n"
  "  /localhost/nfd route={faceid=258 (origin=0 cost=0 ChildInherit)}\n";

BOOST_AUTO_TEST_CASE(Status)
{
  this->fetchStatus();
  RibEntry payload1;
  payload1.setName("/")
          .addRoute(Route().setFaceId(262)
                           .setOrigin(ndn::nfd::ROUTE_ORIGIN_STATIC)
                           .setCost(9)
                           .setFlags(ndn::nfd::ROUTE_FLAG_CAPTURE))
          .addRoute(Route().setFaceId(272)
                           .setOrigin(ndn::nfd::ROUTE_ORIGIN_STATIC)
                           .setCost(50)
                           .setFlags(ndn::nfd::ROUTE_FLAGS_NONE))
          .addRoute(Route().setFaceId(274)
                           .setOrigin(ndn::nfd::ROUTE_ORIGIN_STATIC)
                           .setCost(78)
                           .setFlags(ndn::nfd::ROUTE_FLAG_CHILD_INHERIT | ndn::nfd::ROUTE_FLAG_CAPTURE))
          .addRoute(Route().setFaceId(276)
                           .setOrigin(ndn::nfd::ROUTE_ORIGIN_STATIC)
                           .setCost(79)
                           .setFlags(ndn::nfd::ROUTE_FLAG_CHILD_INHERIT)
                           .setExpirationPeriod(time::milliseconds(47292)));
  RibEntry payload2;
  payload2.setName("/localhost/nfd")
          .addRoute(Route().setFaceId(258)
                           .setOrigin(ndn::nfd::ROUTE_ORIGIN_APP)
                           .setCost(0)
                           .setFlags(ndn::nfd::ROUTE_FLAG_CHILD_INHERIT));
  this->sendDataset("/localhost/nfd/rib/list", payload1, payload2);
  this->prepareStatusOutput();

  BOOST_CHECK(statusXml.is_equal(STATUS_XML));
  BOOST_CHECK(statusText.is_equal(STATUS_TEXT));
}

BOOST_AUTO_TEST_SUITE_END() // TestRibModule
BOOST_AUTO_TEST_SUITE_END() // Nfdc

} // namespace tests
} // namespace nfdc
} // namespace tools
} // namespace nfd
