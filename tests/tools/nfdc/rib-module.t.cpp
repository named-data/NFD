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

#include "nfdc/rib-module.hpp"

#include "execute-command-fixture.hpp"
#include "status-fixture.hpp"

namespace nfd {
namespace tools {
namespace nfdc {
namespace tests {

BOOST_AUTO_TEST_SUITE(Nfdc)
BOOST_FIXTURE_TEST_SUITE(TestRibModule, StatusFixture<RibModule>)

BOOST_FIXTURE_TEST_SUITE(AddCommand, ExecuteCommandFixture)

BOOST_AUTO_TEST_CASE(NormalByFaceId)
{
  this->processInterest = [this] (const Interest& interest) {
    if (this->respondFaceQuery(interest)) {
      return;
    }

    ControlParameters req = MOCK_NFD_MGMT_REQUIRE_LAST_COMMAND_IS("/localhost/nfd/rib/register");
    ndn::nfd::RibRegisterCommand cmd;
    cmd.validateRequest(req);
    cmd.applyDefaultsToRequest(req);
    BOOST_CHECK_EQUAL(req.getName(), "/vxXoEaWeDB");
    BOOST_CHECK_EQUAL(req.getFaceId(), 10156);
    BOOST_CHECK_EQUAL(req.getOrigin(), ndn::nfd::ROUTE_ORIGIN_STATIC);
    BOOST_CHECK_EQUAL(req.getCost(), 0);
    BOOST_CHECK_EQUAL(req.getFlags(), ndn::nfd::ROUTE_FLAGS_NONE);
    BOOST_CHECK_EQUAL(req.hasExpirationPeriod(), false);

    this->succeedCommand(req);
  };

  this->execute("route add /vxXoEaWeDB 10156 no-inherit");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal("route-add-accepted prefix=/vxXoEaWeDB nexthop=10156 origin=255 "
                           "cost=0 flags=none expires=never\n"));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(NormalByFaceUri)
{
  this->processInterest = [this] (const Interest& interest) {
    if (this->respondFaceQuery(interest)) {
      return;
    }

    ControlParameters req = MOCK_NFD_MGMT_REQUIRE_LAST_COMMAND_IS("/localhost/nfd/rib/register");
    ndn::nfd::RibRegisterCommand cmd;
    cmd.validateRequest(req);
    cmd.applyDefaultsToRequest(req);
    BOOST_CHECK_EQUAL(req.getName(), "/FLQAsaYnYf");
    BOOST_CHECK_EQUAL(req.getFaceId(), 2249);
    BOOST_CHECK_EQUAL(req.getOrigin(), 17591);
    BOOST_CHECK_EQUAL(req.getCost(), 702);
    BOOST_CHECK_EQUAL(req.getFlags(), ndn::nfd::ROUTE_FLAG_CHILD_INHERIT |
                                      ndn::nfd::ROUTE_FLAG_CAPTURE);
    BOOST_REQUIRE_EQUAL(req.hasExpirationPeriod(), true);
    BOOST_REQUIRE_EQUAL(req.getExpirationPeriod(), time::milliseconds(727411987));

    ControlParameters resp = req;
    resp.setExpirationPeriod(time::milliseconds(727411154)); // server side may change expiration
    this->succeedCommand(resp);
  };

  this->execute("route add /FLQAsaYnYf tcp4://32.121.182.82:6363 "
                "origin 17591 cost 702 capture expires 727411987");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal("route-add-accepted prefix=/FLQAsaYnYf nexthop=2249 origin=17591 "
                           "cost=702 flags=child-inherit|capture expires=727411154ms\n"));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(FaceNotExist)
{
  this->processInterest = [this] (const Interest& interest) {
    BOOST_CHECK(this->respondFaceQuery(interest));
  };

  this->execute("route add /GJiKDus5i 23728");
  BOOST_CHECK_EQUAL(exitCode, 3);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Face not found\n"));
}

BOOST_AUTO_TEST_CASE(Ambiguous)
{
  this->processInterest = [this] (const Interest& interest) {
    BOOST_CHECK(this->respondFaceQuery(interest));
  };

  this->execute("route add /BQqjjnVsz udp4://225.131.75.231:56363");
  BOOST_CHECK_EQUAL(exitCode, 5);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Multiple faces match specified remote FaceUri. "
                           "Re-run the command with a FaceId: "
                           "6720 (local=udp4://202.83.168.28:56363), "
                           "31066 (local=udp4://25.90.26.32:56363)\n"));
}

BOOST_AUTO_TEST_CASE(ErrorCanonization)
{
  this->execute("route add /bxJfGsVtDt udp6://32.38.164.64:10445");
  BOOST_CHECK_EQUAL(exitCode, 4);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Error during remote FaceUri canonization: "
                           "No endpoints match the specified address selector\n"));
}

BOOST_AUTO_TEST_CASE(ErrorDataset)
{
  this->processInterest = nullptr; // no response to dataset or command

  this->execute("route add /q1Qf7go7 udp://159.242.33.78");
  BOOST_CHECK_EQUAL(exitCode, 1);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Error 10060 when querying face: Timeout\n"));
}

BOOST_AUTO_TEST_CASE(ErrorCommand)
{
  this->processInterest = [this] (const Interest& interest) {
    if (this->respondFaceQuery(interest)) {
      return;
    }

    MOCK_NFD_MGMT_REQUIRE_LAST_COMMAND_IS("/localhost/nfd/rib/register");
    // no response to command
  };

  this->execute("route add /bYiMbEuE 10156");
  BOOST_CHECK_EQUAL(exitCode, 1);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Error 10060 when adding route: request timed out\n"));
}

BOOST_AUTO_TEST_SUITE_END() // AddCommand

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
