/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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
#include "nfd-manager-common-fixture.hpp"

namespace nfd {
namespace tests {

BOOST_AUTO_TEST_SUITE(Mgmt)
BOOST_AUTO_TEST_SUITE(TestFaceManager)

BOOST_FIXTURE_TEST_SUITE(CreateFace, BaseFixture)

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
      .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT)
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
      .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT)
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
      .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT)
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
      .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT)
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
      .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT)
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
      .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT)
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
      .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT)
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
      .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT)
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
      .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT)
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
      .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT)
      .setBaseCongestionMarkingInterval(50_ms)
      .setDefaultCongestionThreshold(1000)
      .setFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED, false);
  }
};

class FaceUriMalformed
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("tcp4://127.0.0.1:not-a-port");
  }
};

class FaceUriNonCanonical
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("udp://localhost");
  }
};

class FaceUriUnsupportedScheme
{
public:
  static ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("dev://eth0");
  }
};

namespace mpl = boost::mpl;

// pairs of CreateCommand and Success/Failure status
using TestCases = mpl::vector<
                    mpl::pair<TcpFaceOnDemand, CommandFailure<406>>,
                    mpl::pair<TcpFacePersistent, CommandSuccess>,
                    mpl::pair<TcpFacePermanent, CommandSuccess>,
                    mpl::pair<UdpFaceOnDemand, CommandFailure<406>>,
                    mpl::pair<UdpFacePersistent, CommandSuccess>,
                    mpl::pair<UdpFacePermanent, CommandSuccess>,
                    mpl::pair<LocalTcpFaceLocalFieldsEnabled, CommandSuccess>,
                    mpl::pair<LocalTcpFaceLocalFieldsDisabled, CommandSuccess>,
                    mpl::pair<NonLocalUdpFaceLocalFieldsEnabled, CommandFailure<406>>,
                    mpl::pair<NonLocalUdpFaceLocalFieldsDisabled, CommandSuccess>,
                    mpl::pair<TcpFaceLpReliabilityEnabled, CommandSuccess>,
                    mpl::pair<TcpFaceLpReliabilityDisabled, CommandSuccess>,
                    mpl::pair<UdpFaceLpReliabilityEnabled, CommandSuccess>,
                    mpl::pair<UdpFaceLpReliabilityDisabled, CommandSuccess>,
                    mpl::pair<TcpFaceCongestionMarkingEnabled, CommandSuccess>,
                    mpl::pair<TcpFaceCongestionMarkingDisabled, CommandSuccess>,
                    mpl::pair<FaceUriMalformed, CommandFailure<400>>,
                    mpl::pair<FaceUriNonCanonical, CommandFailure<400>>,
                    mpl::pair<FaceUriUnsupportedScheme, CommandFailure<406>>>;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(NewFace, T, TestCases, FaceManagerCommandFixture)
{
  using FaceType = typename T::first;
  using CreateResult = typename T::second;

  Interest req = makeControlCommandRequest("/localhost/nfd/faces/create", FaceType::getParameters());

  bool hasCallbackFired = false;
  this->node1.face.onSendData.connect([this, req, &hasCallbackFired] (const Data& response) {
    if (!req.getName().isPrefixOf(response.getName())) {
      return;
    }

    ControlResponse actual(response.getContent().blockFromValue());
    ControlResponse expected(CreateResult::getExpected());
    BOOST_TEST_MESSAGE(actual.getText());
    BOOST_CHECK_EQUAL(expected.getCode(), actual.getCode());

    if (actual.getBody().hasWire()) {
      ControlParameters expectedParams(FaceType::getParameters());
      ControlParameters actualParams(actual.getBody());

      BOOST_CHECK(actualParams.hasFaceId());
      BOOST_CHECK_EQUAL(expectedParams.getFacePersistency(), actualParams.getFacePersistency());

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
            BOOST_CHECK(actualParams.getFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED));
          }
        }
        else {
          // local fields and LpReliability are disabled by default, congestion marking is enabled
          // by default on TCP, UDP, and Unix stream
          BOOST_CHECK(!actualParams.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED));
          BOOST_CHECK(!actualParams.getFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED));
          BOOST_CHECK(actualParams.getFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED));
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
      }
      else {
        BOOST_CHECK_EQUAL(expectedParams.getUri(), actualParams.getUri());
      }
    }

    if (actual.getCode() != 200) {
      FaceUri uri;
      if (uri.parse(FaceType::getParameters().getUri())) {
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
  this->advanceClocks(1_ms, 5);

  BOOST_CHECK(hasCallbackFired);
}

BOOST_FIXTURE_TEST_CASE(ExistingFace, FaceManagerCommandFixture)
{
  using FaceType = UdpFacePersistent;

  {
    // create face
    Interest req = makeControlCommandRequest("/localhost/nfd/faces/create", FaceType::getParameters());
    this->node1.face.receive(req);
    this->advanceClocks(1_ms, 5);
  }

  // find the created face
  auto foundFace = this->node1.findFaceByUri(FaceType::getParameters().getUri());
  BOOST_REQUIRE(foundFace != nullptr);

  {
    // re-create face
    Interest req = makeControlCommandRequest("/localhost/nfd/faces/create", FaceType::getParameters());

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
    this->advanceClocks(1_ms, 5);

    BOOST_CHECK(hasCallbackFired);
  }
}

BOOST_AUTO_TEST_SUITE_END() // CreateFace
BOOST_AUTO_TEST_SUITE_END() // TestFaceManager
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace tests
} // namespace nfd
