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

#include "face-manager-command-fixture.hpp"
#include "nfd-manager-common-fixture.hpp"
#include <ndn-cxx/lp/tags.hpp>

namespace nfd {
namespace tests {

BOOST_AUTO_TEST_SUITE(Mgmt)
BOOST_AUTO_TEST_SUITE(TestFaceManager)

class FaceManagerUpdateFixture : public FaceManagerCommandFixture
{
public:
  FaceManagerUpdateFixture()
    : faceId(0)
  {
  }

  ~FaceManagerUpdateFixture()
  {
    destroyFace();
  }

  void
  createFace(const std::string& uri = "tcp4://127.0.0.1:26363",
             ndn::nfd::FacePersistency persistency = ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
             bool enableLocalFields = false)
  {
    ControlParameters params;
    params.setUri(uri);
    params.setFacePersistency(persistency);
    params.setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, enableLocalFields);

    Name commandName("/localhost/nfd/faces/create");
    commandName.append(params.wireEncode());
    auto command = makeInterest(commandName);
    m_keyChain.sign(*command);

    bool hasCallbackFired = false;
    signal::ScopedConnection connection = this->node1.face.onSendData.connect(
      [this, command, &hasCallbackFired] (const Data& response) {
        if (!command->getName().isPrefixOf(response.getName())) {
          return;
        }

        ControlResponse create(response.getContent().blockFromValue());
        BOOST_REQUIRE_EQUAL(create.getCode(), 200);

        if (create.getBody().hasWire()) {
          ControlParameters faceParams(create.getBody());
          BOOST_REQUIRE(faceParams.hasFaceId());
          this->faceId = faceParams.getFaceId();
        }
        else {
          BOOST_FAIL("Face creation failed");
        }

        hasCallbackFired = true;
      });

    this->node1.face.receive(*command);
    this->advanceClocks(time::milliseconds(1), 5);

    BOOST_REQUIRE(hasCallbackFired);
  }

  void
  updateFace(const ControlParameters& requestParams,
             bool isSelfUpdating,
             const function<void(const ControlResponse& resp)>& checkResp)
  {
    Name commandName("/localhost/nfd/faces/update");
    commandName.append(requestParams.wireEncode());
    auto command = makeInterest(commandName);
    if (isSelfUpdating) {
      // Attach IncomingFaceIdTag to interest
      command->setTag(make_shared<lp::IncomingFaceIdTag>(faceId));
    }
    m_keyChain.sign(*command);

    bool hasCallbackFired = false;
    signal::ScopedConnection connection = this->node1.face.onSendData.connect(
      [this, command, &hasCallbackFired, &checkResp] (const Data& response) {
        if (!command->getName().isPrefixOf(response.getName())) {
          return;
        }

        ControlResponse actual(response.getContent().blockFromValue());
        checkResp(actual);

        hasCallbackFired = true;
      });

    this->node1.face.receive(*command);
    this->advanceClocks(time::milliseconds(1), 5);

    BOOST_CHECK(hasCallbackFired);
  }

private:
  void
  destroyFace()
  {
    if (faceId == 0) {
      return;
    }

    ControlParameters params;
    params.setFaceId(faceId);

    Name commandName("/localhost/nfd/faces/destroy");
    commandName.append(params.wireEncode());
    auto command = makeInterest(commandName);
    m_keyChain.sign(*command);

    bool hasCallbackFired = false;
    signal::ScopedConnection connection = this->node1.face.onSendData.connect(
      [this, command, &hasCallbackFired] (const Data& response) {
        if (!command->getName().isPrefixOf(response.getName())) {
          return;
        }

        ControlResponse destroy(response.getContent().blockFromValue());
        BOOST_CHECK_EQUAL(destroy.getCode(), 200);

        faceId = 0;
        hasCallbackFired = true;
      });

    this->node1.face.receive(*command);
    this->advanceClocks(time::milliseconds(1), 5);

    BOOST_CHECK(hasCallbackFired);
  }

public:
  FaceId faceId;
};

BOOST_FIXTURE_TEST_SUITE(UpdateFace, FaceManagerUpdateFixture)

BOOST_AUTO_TEST_CASE(FaceDoesNotExist)
{
  ControlParameters requestParams;
  requestParams.setFaceId(65535);

  updateFace(requestParams, false, [] (const ControlResponse& actual) {
    ControlResponse expected(404, "Specified face does not exist");
    BOOST_CHECK_EQUAL(actual.getCode(), expected.getCode());
    BOOST_TEST_MESSAGE(actual.getText());
  });
}

// TODO #3232: Expected failure until FacePersistency updating implemented
BOOST_AUTO_TEST_CASE(UpdatePersistency)
{
  createFace();

  ControlParameters requestParams;
  requestParams.setFaceId(faceId);
  requestParams.setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERMANENT);

  updateFace(requestParams, false, [] (const ControlResponse& actual) {
    ControlResponse expected(409, "Invalid fields specified");
    BOOST_CHECK_EQUAL(actual.getCode(), expected.getCode());
    BOOST_TEST_MESSAGE(actual.getText());

    if (actual.getBody().hasWire()) {
      ControlParameters actualParams(actual.getBody());

      BOOST_REQUIRE(actualParams.hasFacePersistency());
      BOOST_CHECK_EQUAL(actualParams.getFacePersistency(), ndn::nfd::FACE_PERSISTENCY_PERMANENT);
    }
    else {
      BOOST_ERROR("Response does not contain ControlParameters");
    }
  });
}

class TcpLocalFieldsEnable
{
public:
  std::string
  getUri()
  {
    return "tcp4://127.0.0.1:26363";
  }

  boost::asio::ip::address_v4
  getIpAddress()
  {
    return boost::asio::ip::address_v4::from_string("127.0.0.1");
  }

  ndn::nfd::FacePersistency
  getPersistency()
  {
    return ndn::nfd::FACE_PERSISTENCY_PERSISTENT;
  }

  bool
  getInitLocalFieldsEnabled()
  {
    return false;
  }

  bool
  getLocalFieldsEnabled()
  {
    return true;
  }

  bool
  getLocalFieldsEnabledMask()
  {
    return true;
  }

  bool
  shouldHaveWire()
  {
    return false;
  }
};

class TcpLocalFieldsDisable
{
public:
  std::string
  getUri()
  {
    return "tcp4://127.0.0.1:26363";
  }

  ndn::nfd::FacePersistency
  getPersistency()
  {
    return ndn::nfd::FACE_PERSISTENCY_PERSISTENT;
  }

  bool
  getInitLocalFieldsEnabled()
  {
    return true;
  }

  bool
  getLocalFieldsEnabled()
  {
    return false;
  }

  bool
  getLocalFieldsEnabledMask()
  {
    return true;
  }

  bool
  shouldHaveWire()
  {
    return false;
  }
};

// UDP faces are non-local by definition
class UdpLocalFieldsEnable
{
public:
  std::string
  getUri()
  {
    return "udp4://127.0.0.1:26363";
  }

  ndn::nfd::FacePersistency
  getPersistency()
  {
    return ndn::nfd::FACE_PERSISTENCY_PERSISTENT;
  }

  bool
  getInitLocalFieldsEnabled()
  {
    return false;
  }

  bool
  getLocalFieldsEnabled()
  {
    return true;
  }

  bool
  getLocalFieldsEnabledMask()
  {
    return true;
  }

  bool
  shouldHaveWire()
  {
    return true;
  }
};

// UDP faces are non-local by definition
// In this test case, attempt to disable local fields on face with local fields already disabled
class UdpLocalFieldsDisable
{
public:
  std::string
  getUri()
  {
    return "udp4://127.0.0.1:26363";
  }

  ndn::nfd::FacePersistency
  getPersistency()
  {
    return ndn::nfd::FACE_PERSISTENCY_PERSISTENT;
  }

  bool
  getInitLocalFieldsEnabled()
  {
    return false;
  }

  bool
  getLocalFieldsEnabled()
  {
    return false;
  }

  bool
  getLocalFieldsEnabledMask()
  {
    return true;
  }

  bool
  shouldHaveWire()
  {
    return false;
  }
};

// In this test case, set Flags to enable local fields on non-local face, but exclude local fields
// from Mask. This test case will pass as no action is taken due to the missing Mask bit.
class UdpLocalFieldsEnableNoMaskBit
{
public:
  std::string
  getUri()
  {
    return "udp4://127.0.0.1:26363";
  }

  ndn::nfd::FacePersistency
  getPersistency()
  {
    return ndn::nfd::FACE_PERSISTENCY_PERSISTENT;
  }

  bool
  getInitLocalFieldsEnabled()
  {
    return false;
  }

  bool
  getLocalFieldsEnabled()
  {
    return true;
  }

  bool
  getLocalFieldsEnabledMask()
  {
    return false;
  }

  bool
  shouldHaveWire()
  {
    return false;
  }
};

namespace mpl = boost::mpl;

typedef mpl::vector<mpl::pair<TcpLocalFieldsEnable, CommandSuccess>,
                    mpl::pair<TcpLocalFieldsDisable, CommandSuccess>,
                    mpl::pair<UdpLocalFieldsEnable, CommandFailure<409>>,
                    mpl::pair<UdpLocalFieldsDisable, CommandSuccess>,
                    mpl::pair<UdpLocalFieldsEnableNoMaskBit, CommandSuccess>> LocalFieldFaces;

BOOST_AUTO_TEST_CASE_TEMPLATE(UpdateLocalFields, T, LocalFieldFaces)
{
  typedef typename T::first TestType;
  typedef typename T::second ResultType;

  createFace(TestType().getUri(), TestType().getPersistency(), TestType().getInitLocalFieldsEnabled());

  ControlParameters requestParams;
  requestParams.setFaceId(faceId);
  requestParams.setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, TestType().getLocalFieldsEnabled());
  if (!TestType().getLocalFieldsEnabledMask()) {
    requestParams.unsetFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED);
  }

  updateFace(requestParams, false, [] (const ControlResponse& actual) {
    ControlResponse expected(ResultType().getExpected().getCode(), "");
    BOOST_CHECK_EQUAL(actual.getCode(), expected.getCode());
    BOOST_TEST_MESSAGE(actual.getText());

    if (TestType().shouldHaveWire() && actual.getBody().hasWire()) {
      ControlParameters actualParams(actual.getBody());

      BOOST_CHECK(!actualParams.hasFacePersistency());
      BOOST_CHECK(actualParams.hasFlags());
      BOOST_CHECK(actualParams.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED));
      BOOST_CHECK(actualParams.hasFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED));
    }
  });
}

BOOST_AUTO_TEST_CASE(UpdateLocalFieldsEnableDisable)
{
  createFace();

  ControlParameters enableParams;
  enableParams.setFaceId(faceId);
  enableParams.setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, true);

  ControlParameters disableParams;
  disableParams.setFaceId(faceId);
  disableParams.setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, false);

  updateFace(enableParams, false, [] (const ControlResponse& actual) {
    ControlResponse expected(200, "OK");
    BOOST_CHECK_EQUAL(actual.getCode(), expected.getCode());
    BOOST_TEST_MESSAGE(actual.getText());

    if (actual.getBody().hasWire()) {
      ControlParameters actualParams(actual.getBody());

      BOOST_CHECK(actualParams.hasFaceId());
      BOOST_CHECK(actualParams.hasFacePersistency());
      BOOST_REQUIRE(actualParams.hasFlags());
      // Check if flags indicate local fields enabled
      BOOST_CHECK(actualParams.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED));
    }
    else {
      BOOST_ERROR("Enable: Response does not contain ControlParameters");
    }
  });

  updateFace(disableParams, false, [] (const ControlResponse& actual) {
    ControlResponse expected(200, "OK");
    BOOST_CHECK_EQUAL(actual.getCode(), expected.getCode());
    BOOST_TEST_MESSAGE(actual.getText());

    if (actual.getBody().hasWire()) {
      ControlParameters actualParams(actual.getBody());

      BOOST_CHECK(actualParams.hasFaceId());
      BOOST_CHECK(actualParams.hasFacePersistency());
      BOOST_REQUIRE(actualParams.hasFlags());
      // Check if flags indicate local fields disabled
      BOOST_CHECK(!actualParams.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED));
    }
    else {
      BOOST_ERROR("Disable: Response does not contain ControlParameters");
    }
  });
}

BOOST_AUTO_TEST_CASE(SelfUpdating)
{
  createFace();

  // Send a command that does nothing (will return 200) and does not contain a FaceId
  ControlParameters sentParams;

  updateFace(sentParams, true, [] (const ControlResponse& actual) {
    ControlResponse expected(200, "OK");
    BOOST_REQUIRE_EQUAL(actual.getCode(), expected.getCode());
    BOOST_TEST_MESSAGE(actual.getText());
  });
}

BOOST_AUTO_TEST_SUITE_END() // UpdateFace
BOOST_AUTO_TEST_SUITE_END() // TestFaceManager
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace tests
} // namespace nfd
