/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2025,  Regents of the University of California,
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

#include "mgmt/face-manager.hpp"
#include "face/generic-link-service.hpp"

#include "face-manager-command-fixture.hpp"

#include <boost/mp11/list.hpp>

namespace nfd::tests {

BOOST_AUTO_TEST_SUITE(Mgmt)
BOOST_AUTO_TEST_SUITE(TestFaceManager)

BOOST_FIXTURE_TEST_SUITE(CreateFace, GlobalIoFixture)

class TcpFaceOnDemand
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("tcp4://127.0.0.1:26363")
      .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  }
};

class TcpFacePersistent
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("tcp4://127.0.0.1:26363")
      .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  }
};

class TcpFacePermanent
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("tcp4://127.0.0.1:26363")
      .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERMANENT);
  }
};

class UdpFaceOnDemand
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("udp4://127.0.0.1:26363")
      .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  }
};

class UdpFacePersistent
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("udp4://127.0.0.1:26363")
      .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  }
};

class UdpFacePermanent
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("udp4://127.0.0.1:26363")
      .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERMANENT);
  }
};

class LocalTcpFaceLocalFieldsEnabled
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("tcp4://127.0.0.1:26363")
      .setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, true);
  }
};

class LocalTcpFaceLocalFieldsDisabled
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("tcp4://127.0.0.1:26363")
      .setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, false);
  }
};

class NonLocalUdpFaceLocalFieldsEnabled // won't work because non-local scope
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("udp4://127.0.0.1:26363")
      .setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, true);
  }
};

class NonLocalUdpFaceLocalFieldsDisabled
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("udp4://127.0.0.1:26363")
      .setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, false);
  }
};

class TcpFaceLpReliabilityEnabled
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("tcp4://127.0.0.1:26363")
      .setFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED, true);
  }
};

class TcpFaceLpReliabilityDisabled
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("tcp4://127.0.0.1:26363")
      .setFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED, false);
  }
};

class UdpFaceLpReliabilityEnabled
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("udp4://127.0.0.1:26363")
      .setFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED, true);
  }
};

class UdpFaceLpReliabilityDisabled
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("udp4://127.0.0.1:26363")
      .setFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED, false);
  }
};

class TcpFaceCongestionMarkingEnabled
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("tcp4://127.0.0.1:26363")
      .setBaseCongestionMarkingInterval(50_ms)
      .setDefaultCongestionThreshold(1000)
      .setFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED, true);
  }
};

class TcpFaceCongestionMarkingDisabled
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("tcp4://127.0.0.1:26363")
      .setBaseCongestionMarkingInterval(50_ms)
      .setDefaultCongestionThreshold(1000)
      .setFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED, false);
  }
};

class UdpFaceCongestionMarkingEnabled
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("udp4://127.0.0.1:26363")
      .setBaseCongestionMarkingInterval(50_ms)
      .setDefaultCongestionThreshold(1000)
      .setFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED, true);
  }
};

class UdpFaceCongestionMarkingDisabled
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("udp4://127.0.0.1:26363")
      .setBaseCongestionMarkingInterval(50_ms)
      .setDefaultCongestionThreshold(1000)
      .setFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED, false);
  }
};

class TcpFaceMtuOverride
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("tcp4://127.0.0.1:26363")
      .setMtu(1000);
  }
};

class UdpFaceMtuOverride
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("udp4://127.0.0.1:26363")
      .setMtu(1000);
  }
};

class RemoteUriMalformed
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("tcp4://127.0.0.1:not-a-port");
  }
};

class RemoteUriNonCanonical
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("udp://localhost");
  }
};

class LocalUriMalformed
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("udp4://127.0.0.1:26363")
      .setLocalUri("udp4://127.0.0.1:not-a-port");
  }
};

class LocalUriNonCanonical
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("udp4://127.0.0.1:26363")
      .setLocalUri("udp://localhost");
  }
};

class LocalUriUnsupported
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("udp4://127.0.0.1:26363")
      .setLocalUri("udp4://127.0.0.1:36363");
  }
};

class UnsupportedProtocol
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("dev://eth0");
  }
};

// Pairs of FaceCreateCommand request and success/failure response
using TestCases = boost::mp11::mp_list<
  boost::mp11::mp_list<TcpFaceOnDemand, CommandFailure<406>>,
  boost::mp11::mp_list<TcpFacePersistent, CommandSuccess>,
  boost::mp11::mp_list<TcpFacePermanent, CommandSuccess>,
  boost::mp11::mp_list<UdpFaceOnDemand, CommandFailure<406>>,
  boost::mp11::mp_list<UdpFacePersistent, CommandSuccess>,
  boost::mp11::mp_list<UdpFacePermanent, CommandSuccess>,
  boost::mp11::mp_list<LocalTcpFaceLocalFieldsEnabled, CommandSuccess>,
  boost::mp11::mp_list<LocalTcpFaceLocalFieldsDisabled, CommandSuccess>,
  boost::mp11::mp_list<NonLocalUdpFaceLocalFieldsEnabled, CommandFailure<406>>,
  boost::mp11::mp_list<NonLocalUdpFaceLocalFieldsDisabled, CommandSuccess>,
  boost::mp11::mp_list<TcpFaceLpReliabilityEnabled, CommandSuccess>,
  boost::mp11::mp_list<TcpFaceLpReliabilityDisabled, CommandSuccess>,
  boost::mp11::mp_list<UdpFaceLpReliabilityEnabled, CommandSuccess>,
  boost::mp11::mp_list<UdpFaceLpReliabilityDisabled, CommandSuccess>,
  boost::mp11::mp_list<TcpFaceCongestionMarkingEnabled, CommandSuccess>,
  boost::mp11::mp_list<TcpFaceCongestionMarkingDisabled, CommandSuccess>,
  boost::mp11::mp_list<UdpFaceCongestionMarkingEnabled, CommandSuccess>,
  boost::mp11::mp_list<UdpFaceCongestionMarkingDisabled, CommandSuccess>,
  boost::mp11::mp_list<TcpFaceMtuOverride, CommandFailure<406>>,
  boost::mp11::mp_list<UdpFaceMtuOverride, CommandSuccess>,
  boost::mp11::mp_list<RemoteUriMalformed, CommandFailure<400>>,
  boost::mp11::mp_list<RemoteUriNonCanonical, CommandFailure<400>>,
  boost::mp11::mp_list<LocalUriMalformed, CommandFailure<400>>,
  boost::mp11::mp_list<LocalUriNonCanonical, CommandFailure<400>>,
  boost::mp11::mp_list<LocalUriUnsupported, CommandFailure<406>>,
  boost::mp11::mp_list<UnsupportedProtocol, CommandFailure<406>>
>;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(NewFace, T, TestCases, FaceManagerCommandFixture)
{
  using CreateRequest = boost::mp11::mp_first<T>;
  using CreateResult = boost::mp11::mp_second<T>;

  Interest req = makeControlCommandRequest(this->CREATE_REQUEST, CreateRequest::getParameters());

  bool hasCallbackFired = false;
  this->node1.face.onSendData.connect([this, req, &hasCallbackFired] (const Data& response) {
    if (!req.getName().isPrefixOf(response.getName())) {
      return;
    }

    ControlResponse actual(response.getContent().blockFromValue());
    ControlResponse expected(CreateResult::getExpected());
    BOOST_TEST(expected.getCode() == actual.getCode(), actual.getText());

    if (actual.getBody().hasWire()) {
      ControlParameters expectedParams(CreateRequest::getParameters());
      ndn::nfd::FaceCreateCommand::applyDefaultsToRequest(expectedParams);
      ControlParameters actualParams(actual.getBody());

      BOOST_TEST(actualParams.hasFaceId());
      BOOST_TEST(expectedParams.getUri() == actualParams.getUri());
      BOOST_TEST(actualParams.hasLocalUri());
      BOOST_TEST(actualParams.hasFlags());
      BOOST_TEST(expectedParams.getFacePersistency() == actualParams.getFacePersistency());

      if (actual.getCode() == 200) {
        if (expectedParams.hasFlags()) {
          BOOST_CHECK_EQUAL(expectedParams.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED),
                            actualParams.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED));
          BOOST_CHECK_EQUAL(expectedParams.getFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED),
                            actualParams.getFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED));
          if (expectedParams.hasFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED)) {
            BOOST_CHECK_EQUAL(expectedParams.getFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED),
                              actualParams.getFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED));
          }
          else {
            // congestion marking is enabled by default
            BOOST_TEST(actualParams.getFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED));
          }
        }
        else {
          // local fields and reliability are disabled by default
          // congestion marking is enabled by default on TCP, UDP, and Unix stream
          BOOST_TEST(!actualParams.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED));
          BOOST_TEST(!actualParams.getFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED));
          BOOST_TEST(actualParams.getFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED));
        }

        if (expectedParams.hasBaseCongestionMarkingInterval()) {
          BOOST_CHECK_EQUAL(expectedParams.getBaseCongestionMarkingInterval(),
                            actualParams.getBaseCongestionMarkingInterval());
        }
        else {
          BOOST_CHECK_EQUAL(actualParams.getBaseCongestionMarkingInterval(), 100_ms);
        }

        if (expectedParams.hasDefaultCongestionThreshold()) {
          BOOST_CHECK_EQUAL(expectedParams.getDefaultCongestionThreshold(),
                            actualParams.getDefaultCongestionThreshold());
        }
        else {
          BOOST_CHECK_EQUAL(actualParams.getDefaultCongestionThreshold(), 65536);
        }

        if (expectedParams.hasMtu()) {
          BOOST_CHECK_EQUAL(expectedParams.getMtu(), actualParams.getMtu());
        }
      }
    }

    if (actual.getCode() != 200) {
      FaceUri uri;
      if (uri.parse(CreateRequest::getParameters().getUri())) {
        // ensure face not created
        const auto& faceTable = this->node1.faceTable;
        BOOST_CHECK(std::none_of(faceTable.begin(), faceTable.end(), [uri] (Face& face) {
          return face.getRemoteUri() == uri;
        }));
      }
      else {
        // do not check malformed FaceUri
      }
    }

    hasCallbackFired = true;
  });

  this->node1.face.receive(req);
  this->advanceClocks(1_ms, 10);

  BOOST_TEST(hasCallbackFired);
}

BOOST_FIXTURE_TEST_CASE(ExistingFace, FaceManagerCommandFixture)
{
  using FaceType = UdpFacePersistent;

  {
    // create face
    Interest req = makeControlCommandRequest(CREATE_REQUEST, FaceType::getParameters());
    this->node1.face.receive(req);
    this->advanceClocks(1_ms, 10);
  }

  // find the created face
  auto foundFace = this->node1.findFaceByUri(FaceType::getParameters().getUri());
  BOOST_TEST_REQUIRE(foundFace != nullptr);

  {
    // re-create face
    Interest req = makeControlCommandRequest(CREATE_REQUEST, FaceType::getParameters());

    bool hasCallbackFired = false;
    this->node1.face.onSendData.connect(
      [req, foundFace, &hasCallbackFired] (const Data& response) {
        if (!req.getName().isPrefixOf(response.getName())) {
          return;
        }

        ControlResponse actual(response.getContent().blockFromValue());
        BOOST_REQUIRE_EQUAL(actual.getCode(), 409);

        ControlParameters actualParams(actual.getBody());
        BOOST_CHECK_EQUAL(foundFace->getId(), actualParams.getFaceId());
        BOOST_CHECK_EQUAL(foundFace->getRemoteUri().toString(), actualParams.getUri());
        BOOST_CHECK_EQUAL(foundFace->getPersistency(), actualParams.getFacePersistency());
        auto linkService = dynamic_cast<face::GenericLinkService*>(foundFace->getLinkService());
        BOOST_CHECK_EQUAL(linkService->getOptions().baseCongestionMarkingInterval,
                          actualParams.getBaseCongestionMarkingInterval());
        BOOST_CHECK_EQUAL(linkService->getOptions().defaultCongestionThreshold,
                          actualParams.getDefaultCongestionThreshold());

        hasCallbackFired = true;
      });

    this->node1.face.receive(req);
    this->advanceClocks(1_ms, 10);

    BOOST_TEST(hasCallbackFired);
  }
}

BOOST_AUTO_TEST_SUITE_END() // CreateFace
BOOST_AUTO_TEST_SUITE_END() // TestFaceManager
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace nfd::tests
