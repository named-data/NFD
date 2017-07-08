/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

#include "ndn-autoconfig/multicast-discovery.hpp"

#include "../mock-nfd-mgmt-fixture.hpp"
#include <ndn-cxx/encoding/block-helpers.hpp>
#include <ndn-cxx/encoding/tlv-nfd.hpp>

namespace ndn {
namespace tools {
namespace autoconfig {
namespace tests {

using namespace ::nfd::tools::tests;
using nfd::ControlParameters;

BOOST_AUTO_TEST_SUITE(NdnAutoconfig)
BOOST_FIXTURE_TEST_SUITE(TestMulticastDiscovery, MockNfdMgmtFixture)

BOOST_AUTO_TEST_CASE(Normal)
{
  this->processInterest = [this] (const Interest& interest) {
    if (Name("/localhost/nfd/faces/query").isPrefixOf(interest.getName())) {
      nfd::FaceStatus payload1, payload2;
      payload1.setFaceId(860)
              .setRemoteUri("ether://[ff:ff:ff:ff:ff:ff]")
              .setLocalUri("dev://eth0")
              .setFaceScope(nfd::FACE_SCOPE_NON_LOCAL)
              .setFacePersistency(nfd::FACE_PERSISTENCY_PERMANENT)
              .setLinkType(nfd::LINK_TYPE_MULTI_ACCESS);
      payload2.setFaceId(861)
              .setRemoteUri("ether://[ff:ff:ff:ff:ff:ff]")
              .setLocalUri("dev://eth1")
              .setFaceScope(nfd::FACE_SCOPE_NON_LOCAL)
              .setFacePersistency(nfd::FACE_PERSISTENCY_PERMANENT)
              .setLinkType(nfd::LINK_TYPE_MULTI_ACCESS);
      this->sendDataset(interest.getName(), payload1, payload2);
      return;
    }

    optional<ControlParameters> req = parseCommand(interest, "/localhost/nfd/rib/register");
    if (req) {
      BOOST_REQUIRE(req->hasName());
      BOOST_CHECK_EQUAL(req->getName(), "/localhop/ndn-autoconf/hub");
      BOOST_REQUIRE(req->hasFaceId());

      if (req->getFaceId() == 860) {
        ControlParameters resp;
        resp.setName("/localhop/ndn-autoconf/hub")
            .setFaceId(860)
            .setOrigin(nfd::ROUTE_ORIGIN_APP)
            .setCost(1)
            .setFlags(0);
        this->succeedCommand(interest, resp);
      }
      else if (req->getFaceId() == 861) {
        // no reply, but stage should continue when there is at least one successful registration
      }
      else {
        BOOST_ERROR("unexpected rib/register command");
      }
      return;
    }

    req = parseCommand(interest, "/localhost/nfd/strategy-choice/set");
    if (req) {
      BOOST_REQUIRE(req->hasName());
      BOOST_CHECK_EQUAL(req->getName(), "/localhop/ndn-autoconf/hub");
      BOOST_REQUIRE(req->hasStrategy());
      BOOST_CHECK_EQUAL(req->getStrategy(), "/localhost/nfd/strategy/multicast");

      this->succeedCommand(interest, *req);
      return;
    }

    if (interest.getName() == "/localhop/ndn-autoconf/hub") {
      const char FACEURI[] = "udp://router.example.net";
      auto data = makeData(Name("/localhop/ndn-autoconf/hub").appendVersion());
      data->setContent(makeBinaryBlock(tlv::nfd::Uri, FACEURI, ::strlen(FACEURI)));
      face.receive(*data);
      return;
    }

    BOOST_ERROR("unexpected Interest " << interest);
  };

  nfd::Controller controller(face, m_keyChain);
  MulticastDiscovery stage(face, controller);

  bool hasSuccess = false;
  stage.onSuccess.connect([&hasSuccess] (const FaceUri& u) {
    BOOST_CHECK(!hasSuccess);
    hasSuccess = true;

    BOOST_CHECK(u == FaceUri("udp://router.example.net"));
  });

  stage.onFailure.connect([] (const std::string& reason) {
    BOOST_ERROR("unexpected failure: " << reason);
  });

  stage.start();
  face.processEvents(time::seconds(20));
  BOOST_CHECK(hasSuccess);
}

BOOST_AUTO_TEST_SUITE_END() // TestMulticastDiscovery
BOOST_AUTO_TEST_SUITE_END() // NdnAutoconfig

} // namespace tests
} // namespace autoconfig
} // namespace tools
} // namespace ndn
