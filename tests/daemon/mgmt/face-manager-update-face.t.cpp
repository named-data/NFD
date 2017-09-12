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

#include "mgmt/face-manager.hpp"
#include "face/generic-link-service.hpp"
#include "face-manager-command-fixture.hpp"
#include "nfd-manager-common-fixture.hpp"

#include <ndn-cxx/lp/tags.hpp>

#include <thread>

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
             bool enableLocalFields = false,
             bool enableReliability = false)
  {
    ControlParameters params;
    params.setUri(uri);
    params.setFacePersistency(persistency);
    params.setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, enableLocalFields);
    params.setFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED, enableReliability);

    createFace(params);
  }

  void
  createFace(const ControlParameters& createParams,
             bool isForOnDemandFace = false)
  {
    Interest req = makeControlCommandRequest("/localhost/nfd/faces/create", createParams);

    // if this creation if for on-demand face then create it on node2
    FaceManagerCommandNode& target = isForOnDemandFace ? this->node2 : this->node1;

    bool hasCallbackFired = false;
    signal::ScopedConnection connection = target.face.onSendData.connect(
      [&, req, isForOnDemandFace, this] (const Data& response) {
        if (!req.getName().isPrefixOf(response.getName())) {
          return;
        }

        ControlResponse create(response.getContent().blockFromValue());
        BOOST_REQUIRE_EQUAL(create.getCode(), 200);
        BOOST_REQUIRE(create.getBody().hasWire());

        ControlParameters faceParams(create.getBody());
        BOOST_REQUIRE(faceParams.hasFaceId());
        this->faceId = faceParams.getFaceId();

        hasCallbackFired = true;

        if (isForOnDemandFace) {
          auto face = target.faceTable.get(static_cast<FaceId>(this->faceId));

          // to force creation of on-demand face
          face->sendInterest(*make_shared<Interest>("/hello/world"));
        }
      });

    target.face.receive(req);
    this->advanceClocks(time::milliseconds(1), 5);

    if (isForOnDemandFace) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100)); // allow wallclock time for socket IO
      this->advanceClocks(time::milliseconds(1), 5); // let node1 accept Interest and create on-demand face
    }

    BOOST_REQUIRE(hasCallbackFired);
  }

  void
  updateFace(const ControlParameters& requestParams,
             bool isSelfUpdating,
             const function<void(const ControlResponse& resp)>& checkResp)
  {
    Interest req = makeControlCommandRequest("/localhost/nfd/faces/update", requestParams);
    if (isSelfUpdating) {
      // Attach IncomingFaceIdTag to interest
      req.setTag(make_shared<lp::IncomingFaceIdTag>(faceId));
    }

    bool hasCallbackFired = false;
    signal::ScopedConnection connection = this->node1.face.onSendData.connect(
      [req, &hasCallbackFired, &checkResp] (const Data& response) {
        if (!req.getName().isPrefixOf(response.getName())) {
          return;
        }

        ControlResponse actual(response.getContent().blockFromValue());
        checkResp(actual);

        hasCallbackFired = true;
      });

    this->node1.face.receive(req);
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

    Interest req = makeControlCommandRequest("/localhost/nfd/faces/destroy", params);

    bool hasCallbackFired = false;
    signal::ScopedConnection connection = this->node1.face.onSendData.connect(
      [this, req, &hasCallbackFired] (const Data& response) {
        if (!req.getName().isPrefixOf(response.getName())) {
          return;
        }

        ControlResponse destroy(response.getContent().blockFromValue());
        BOOST_CHECK_EQUAL(destroy.getCode(), 200);

        faceId = 0;
        hasCallbackFired = true;
      });

    this->node1.face.receive(req);
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

template<bool CAN_CHANGE_PERSISTENCY>
class UpdatePersistencyDummyTransport : public face::Transport
{
public:
  UpdatePersistencyDummyTransport()
  {
    this->setPersistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  }

protected:
  bool
  canChangePersistencyToImpl(ndn::nfd::FacePersistency newPersistency) const override
  {
    return CAN_CHANGE_PERSISTENCY;
  }

  void
  doClose() override
  {
  }

private:
  void
  doSend(face::Transport::Packet&& packet) override
  {
  }
};

namespace mpl = boost::mpl;

using UpdatePersistencyTests =
  mpl::vector<mpl::pair<UpdatePersistencyDummyTransport<true>, CommandSuccess>,
              mpl::pair<UpdatePersistencyDummyTransport<false>, CommandFailure<409>>>;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(UpdatePersistency, T, UpdatePersistencyTests, FaceManagerUpdateFixture)
{
  using TransportType = typename T::first;
  using ResultType = typename T::second;

  auto face = make_shared<face::Face>(make_unique<face::GenericLinkService>(),
                                      make_unique<TransportType>());
  this->node1.faceTable.add(face);

  auto parameters = ControlParameters()
    .setFaceId(face->getId())
    .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERMANENT);

  updateFace(parameters, false, [] (const ControlResponse& actual) {
      BOOST_TEST_MESSAGE(actual.getText());
      BOOST_CHECK_EQUAL(actual.getCode(), ResultType::getExpected().getCode());

      // the response for either 200 or 409 will have a content body
      BOOST_REQUIRE(actual.getBody().hasWire());

      ControlParameters resp;
      resp.wireDecode(actual.getBody());
      BOOST_CHECK_EQUAL(resp.getFacePersistency(), ndn::nfd::FACE_PERSISTENCY_PERMANENT);
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

BOOST_AUTO_TEST_CASE(UpdateReliabilityEnableDisable)
{
  createFace("udp4://127.0.0.1:26363");

  ControlParameters enableParams;
  enableParams.setFaceId(faceId);
  enableParams.setFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED, true);

  ControlParameters disableParams;
  disableParams.setFaceId(faceId);
  disableParams.setFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED, false);

  updateFace(enableParams, false, [] (const ControlResponse& actual) {
    ControlResponse expected(200, "OK");
    BOOST_CHECK_EQUAL(actual.getCode(), expected.getCode());
    BOOST_TEST_MESSAGE(actual.getText());

    if (actual.getBody().hasWire()) {
      ControlParameters actualParams(actual.getBody());

      BOOST_CHECK(actualParams.hasFaceId());
      BOOST_CHECK(actualParams.hasFacePersistency());
      BOOST_REQUIRE(actualParams.hasFlags());
      // Check if flags indicate reliability enabled
      BOOST_CHECK(actualParams.getFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED));
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
      // Check if flags indicate reliability disabled
      BOOST_CHECK(!actualParams.getFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED));
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
