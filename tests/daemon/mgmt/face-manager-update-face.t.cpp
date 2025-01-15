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
#include "tests/daemon/face/dummy-transport.hpp"

#include <ndn-cxx/lp/tags.hpp>

#include <boost/logic/tribool.hpp>
#include <boost/mp11/list.hpp>
#include <thread>

namespace nfd::tests {

BOOST_AUTO_TEST_SUITE(Mgmt)
BOOST_AUTO_TEST_SUITE(TestFaceManager)

class FaceManagerUpdateFixture : public FaceManagerCommandFixture
{
public:
  ~FaceManagerUpdateFixture() override
  {
    destroyFace();
  }

  void
  createFace(const std::string& uri = "tcp4://127.0.0.1:26363",
             ndn::nfd::FacePersistency persistency = ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
             std::optional<time::nanoseconds> baseCongestionMarkingInterval = std::nullopt,
             std::optional<uint64_t> defaultCongestionThreshold = std::nullopt,
             bool enableLocalFields = false,
             bool enableReliability = false,
             boost::logic::tribool enableCongestionMarking = boost::logic::indeterminate)
  {
    ControlParameters params;
    params.setUri(uri);
    params.setFacePersistency(persistency);

    if (baseCongestionMarkingInterval) {
      params.setBaseCongestionMarkingInterval(*baseCongestionMarkingInterval);
    }

    if (defaultCongestionThreshold) {
      params.setDefaultCongestionThreshold(*defaultCongestionThreshold);
    }

    params.setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, enableLocalFields);
    params.setFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED, enableReliability);

    if (!boost::logic::indeterminate(enableCongestionMarking)) {
      params.setFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED, bool(enableCongestionMarking));
    }

    createFace(params);
  }

  void
  createFace(const ControlParameters& createParams, bool isForOnDemandFace = false)
  {
    Interest req = makeControlCommandRequest(CREATE_REQUEST, createParams);

    // if this creation if for on-demand face then create it on node2
    FaceManagerCommandNode& target = isForOnDemandFace ? this->node2 : this->node1;

    bool hasCallbackFired = false;
    signal::ScopedConnection conn = target.face.onSendData.connect([&] (const Data& response) {
      if (!req.getName().isPrefixOf(response.getName())) {
        return;
      }

      ControlResponse create(response.getContent().blockFromValue());
      BOOST_TEST_REQUIRE(create.getCode() == 200);
      ControlParameters faceParams(create.getBody());
      BOOST_TEST_REQUIRE(faceParams.hasFaceId());
      this->faceId = faceParams.getFaceId();

      hasCallbackFired = true;

      if (isForOnDemandFace) {
        auto face = target.faceTable.get(this->faceId);
        // to force creation of on-demand face
        face->sendInterest(*makeInterest("/hello/world"));
      }
    });

    target.face.receive(req);
    advanceClocks(1_ms, 10);

    if (isForOnDemandFace) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100)); // allow wallclock time for socket IO
      advanceClocks(1_ms, 10); // let node1 accept Interest and create on-demand face
    }

    BOOST_TEST_REQUIRE(hasCallbackFired);
  }

  void
  updateFace(const ControlParameters& requestParams, bool isSelfUpdating,
             const std::function<void(const ControlResponse& resp)>& checkResp)
  {
    Interest req = makeControlCommandRequest(UPDATE_REQUEST, requestParams);
    if (isSelfUpdating) {
      // Attach IncomingFaceIdTag to interest
      req.setTag(make_shared<lp::IncomingFaceIdTag>(faceId));
    }

    bool hasCallbackFired = false;
    signal::ScopedConnection conn = node1.face.onSendData.connect([&] (const Data& response) {
      if (!req.getName().isPrefixOf(response.getName())) {
        return;
      }

      ControlResponse actual(response.getContent().blockFromValue());
      checkResp(actual);

      hasCallbackFired = true;
    });

    this->node1.face.receive(req);
    advanceClocks(1_ms, 10);
    BOOST_TEST_REQUIRE(hasCallbackFired);
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
    Interest req = makeControlCommandRequest(DESTROY_REQUEST, params);

    bool hasCallbackFired = false;
    signal::ScopedConnection conn = node1.face.onSendData.connect([&] (const Data& response) {
      if (!req.getName().isPrefixOf(response.getName())) {
        return;
      }

      ControlResponse destroy(response.getContent().blockFromValue());
      BOOST_CHECK_EQUAL(destroy.getCode(), 200);

      faceId = 0;
      hasCallbackFired = true;
    });

    this->node1.face.receive(req);
    advanceClocks(1_ms, 10);
    BOOST_TEST_REQUIRE(hasCallbackFired);
  }

protected:
  FaceId faceId = 0;
};

BOOST_FIXTURE_TEST_SUITE(UpdateFace, FaceManagerUpdateFixture)

BOOST_AUTO_TEST_CASE(FaceDoesNotExist)
{
  ControlParameters requestParams;
  requestParams.setFaceId(65535);

  updateFace(requestParams, false, [] (const ControlResponse& actual) {
    BOOST_TEST(actual.getCode() == 404);
    BOOST_TEST(actual.getText() == "Specified face does not exist");
  });
}

using UpdatePersistencyTests = boost::mp11::mp_list<
  boost::mp11::mp_list<DummyTransportBase<true>, CommandSuccess>,
  boost::mp11::mp_list<DummyTransportBase<false>, CommandFailure<409>>
>;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(UpdatePersistency, T, UpdatePersistencyTests, FaceManagerUpdateFixture)
{
  using TransportType = boost::mp11::mp_first<T>;
  using ResultType = boost::mp11::mp_second<T>;

  auto face = make_shared<face::Face>(make_unique<face::GenericLinkService>(),
                                      make_unique<TransportType>());
  this->node1.faceTable.add(face);

  auto parameters = ControlParameters()
    .setFaceId(face->getId())
    .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERMANENT);

  updateFace(parameters, false, [] (const ControlResponse& actual) {
    BOOST_TEST(actual.getCode() == ResultType::getExpected().getCode(), actual.getText());

    // the response for either 200 or 409 will have a content body
    ControlParameters resp(actual.getBody());
    BOOST_CHECK_EQUAL(resp.getFacePersistency(), ndn::nfd::FACE_PERSISTENCY_PERMANENT);
  });
}

BOOST_AUTO_TEST_CASE(UpdateMtu)
{
  createFace("udp4://127.0.0.1:26363");

  ControlParameters validParams;
  validParams.setFaceId(faceId);
  validParams.setMtu(4000);

  ControlParameters mtuTooLow;
  mtuTooLow.setFaceId(faceId);
  mtuTooLow.setMtu(63);

  updateFace(validParams, false, [] (const ControlResponse& actual) {
    BOOST_TEST(actual.getCode() == 200, actual.getText());

    ControlParameters actualParams(actual.getBody());
    BOOST_TEST(actualParams.hasFaceId());
    BOOST_TEST_REQUIRE(actualParams.hasMtu());
    // Check for changed MTU
    BOOST_CHECK_EQUAL(actualParams.getMtu(), 4000);
  });

  updateFace(mtuTooLow, false, [] (const ControlResponse& actual) {
    BOOST_TEST(actual.getCode() == 409);
    BOOST_TEST(actual.getText() == "Invalid properties specified");

    ControlParameters actualParams(actual.getBody());
    BOOST_TEST(!actualParams.hasFaceId());
    BOOST_TEST_REQUIRE(actualParams.hasMtu());
    // Check for returned invalid parameter
    BOOST_CHECK_EQUAL(actualParams.getMtu(), 63);
  });
}

BOOST_AUTO_TEST_CASE(UpdateMtuUnsupportedFace)
{
  createFace("tcp4://127.0.0.1:26363");

  ControlParameters updateParams;
  updateParams.setFaceId(faceId);
  updateParams.setMtu(4000);

  updateFace(updateParams, false, [] (const ControlResponse& actual) {
    BOOST_TEST(actual.getCode() == 409);
    BOOST_TEST(actual.getText() == "Invalid properties specified");

    ControlParameters actualParams(actual.getBody());
    BOOST_TEST(!actualParams.hasFaceId());
    BOOST_TEST_REQUIRE(actualParams.hasMtu());
    // Check for returned invalid parameter
    BOOST_CHECK_EQUAL(actualParams.getMtu(), 4000);
  });
}

template<bool EnableLocalFields>
struct TcpLocalFields
{
  static inline const std::string uri = "tcp4://127.0.0.1:26363";
  static constexpr bool localFieldsInit = !EnableLocalFields;
  static constexpr bool localFieldsEnabled = EnableLocalFields;
  static constexpr bool mask = true;
  static constexpr bool shouldHaveBody = false;
};

// UDP faces are non-local by definition.
struct UdpLocalFieldsEnable
{
  static inline const std::string uri = "udp4://127.0.0.1:26363";
  static constexpr bool localFieldsInit = false;
  static constexpr bool localFieldsEnabled = true;
  static constexpr bool mask = true;
  static constexpr bool shouldHaveBody = true;
};

// UDP faces are non-local by definition.
// In this test case, attempt to disable local fields on face with local fields already disabled.
struct UdpLocalFieldsDisable
{
  static inline const std::string uri = "udp4://127.0.0.1:26363";
  static constexpr bool localFieldsInit = false;
  static constexpr bool localFieldsEnabled = false;
  static constexpr bool mask = true;
  static constexpr bool shouldHaveBody = false;
};

// In this test case, set Flags to enable local fields on non-local face, but exclude local fields
// from Mask. This test case will pass as no action is taken due to the missing Mask bit.
struct UdpLocalFieldsEnableNoMaskBit
{
  static inline const std::string uri = "udp4://127.0.0.1:26363";
  static constexpr bool localFieldsInit = false;
  static constexpr bool localFieldsEnabled = true;
  static constexpr bool mask = false;
  static constexpr bool shouldHaveBody = false;
};

using LocalFieldFaces = boost::mp11::mp_list<
  boost::mp11::mp_list<TcpLocalFields<true>, CommandSuccess>,
  boost::mp11::mp_list<TcpLocalFields<false>, CommandSuccess>,
  boost::mp11::mp_list<UdpLocalFieldsEnable, CommandFailure<409>>,
  boost::mp11::mp_list<UdpLocalFieldsDisable, CommandSuccess>,
  boost::mp11::mp_list<UdpLocalFieldsEnableNoMaskBit, CommandSuccess>
>;

BOOST_AUTO_TEST_CASE_TEMPLATE(UpdateLocalFields, T, LocalFieldFaces)
{
  using TestType = boost::mp11::mp_first<T>;
  using ResultType = boost::mp11::mp_second<T>;

  createFace(TestType::uri, ndn::nfd::FACE_PERSISTENCY_PERSISTENT, {}, {}, TestType::localFieldsInit);

  ControlParameters requestParams;
  requestParams.setFaceId(faceId);
  requestParams.setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, TestType::localFieldsEnabled);
  if constexpr (!TestType::mask) {
    requestParams.unsetFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED);
  }

  updateFace(requestParams, false, [] (const ControlResponse& actual) {
    BOOST_TEST(actual.getCode() == ResultType::getExpected().getCode(), actual.getText());

    if constexpr (TestType::shouldHaveBody) {
      ControlParameters actualParams(actual.getBody());
      BOOST_TEST(!actualParams.hasFacePersistency());
      BOOST_TEST(actualParams.hasFlags());
      BOOST_TEST(actualParams.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED));
      BOOST_TEST(actualParams.hasFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED));
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
    BOOST_TEST(actual.getCode() == 200, actual.getText());

    ControlParameters actualParams(actual.getBody());
    BOOST_TEST(actualParams.hasFaceId());
    BOOST_TEST(actualParams.hasFacePersistency());
    BOOST_TEST_REQUIRE(actualParams.hasFlags());
    // Check if flags indicate local fields enabled
    BOOST_TEST(actualParams.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED));
  });

  updateFace(disableParams, false, [] (const ControlResponse& actual) {
    BOOST_TEST(actual.getCode() == 200, actual.getText());

    ControlParameters actualParams(actual.getBody());
    BOOST_TEST(actualParams.hasFaceId());
    BOOST_TEST(actualParams.hasFacePersistency());
    BOOST_TEST_REQUIRE(actualParams.hasFlags());
    // Check if flags indicate local fields disabled
    BOOST_TEST(!actualParams.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED));
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
    BOOST_TEST(actual.getCode() == 200, actual.getText());

    ControlParameters actualParams(actual.getBody());
    BOOST_TEST(actualParams.hasFaceId());
    BOOST_TEST(actualParams.hasFacePersistency());
    BOOST_TEST_REQUIRE(actualParams.hasFlags());
    // Check if flags indicate reliability enabled
    BOOST_TEST(actualParams.getFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED));
  });

  updateFace(disableParams, false, [] (const ControlResponse& actual) {
    BOOST_TEST(actual.getCode() == 200, actual.getText());

    ControlParameters actualParams(actual.getBody());
    BOOST_TEST(actualParams.hasFaceId());
    BOOST_TEST(actualParams.hasFacePersistency());
    BOOST_TEST_REQUIRE(actualParams.hasFlags());
    // Check if flags indicate reliability disabled
    BOOST_TEST(!actualParams.getFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED));
  });
}

BOOST_AUTO_TEST_CASE(UpdateCongestionMarkingEnableDisable)
{
  createFace("udp4://127.0.0.1:26363");

  ControlParameters enableParams;
  enableParams.setFaceId(faceId);
  enableParams.setBaseCongestionMarkingInterval(50_ms);
  enableParams.setDefaultCongestionThreshold(10000);
  enableParams.setFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED, true);

  ControlParameters disableParams;
  disableParams.setFaceId(faceId);
  disableParams.setBaseCongestionMarkingInterval(70_ms);
  disableParams.setDefaultCongestionThreshold(5000);
  disableParams.setFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED, false);

  updateFace(enableParams, false, [] (const ControlResponse& actual) {
    BOOST_TEST(actual.getCode() == 200, actual.getText());

    ControlParameters actualParams(actual.getBody());
    BOOST_TEST(actualParams.hasFaceId());
    BOOST_TEST(actualParams.hasFacePersistency());
    // Check that congestion marking parameters changed
    BOOST_TEST_REQUIRE(actualParams.hasBaseCongestionMarkingInterval());
    BOOST_CHECK_EQUAL(actualParams.getBaseCongestionMarkingInterval(), 50_ms);
    BOOST_TEST_REQUIRE(actualParams.hasDefaultCongestionThreshold());
    BOOST_CHECK_EQUAL(actualParams.getDefaultCongestionThreshold(), 10000);
    BOOST_TEST_REQUIRE(actualParams.hasFlags());
    // Check if flags indicate congestion marking enabled
    BOOST_TEST(actualParams.getFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED));
  });

  updateFace(disableParams, false, [] (const ControlResponse& actual) {
    BOOST_TEST(actual.getCode() == 200, actual.getText());

    ControlParameters actualParams(actual.getBody());
    BOOST_TEST(actualParams.hasFaceId());
    BOOST_TEST(actualParams.hasFacePersistency());
    // Check that congestion marking parameters changed, even though feature disabled
    BOOST_TEST_REQUIRE(actualParams.hasBaseCongestionMarkingInterval());
    BOOST_CHECK_EQUAL(actualParams.getBaseCongestionMarkingInterval(), 70_ms);
    BOOST_TEST_REQUIRE(actualParams.hasDefaultCongestionThreshold());
    BOOST_CHECK_EQUAL(actualParams.getDefaultCongestionThreshold(), 5000);
    BOOST_TEST_REQUIRE(actualParams.hasFlags());
    // Check if flags indicate marking disabled
    BOOST_TEST(!actualParams.getFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED));
  });
}

BOOST_AUTO_TEST_CASE(SelfUpdating)
{
  createFace();

  // Send a command that does nothing and does not contain a FaceId
  ControlParameters sentParams;

  // Success case: FaceId is obtained automatically from IncomingFaceIdTag
  updateFace(sentParams, true, [] (const ControlResponse& actual) {
    BOOST_TEST(actual.getCode() == 200, actual.getText());
  });

  // Error case: IncomingFaceIdTag is missing
  updateFace(sentParams, false, [] (const ControlResponse& actual) {
    BOOST_TEST(actual.getCode() == 404);
    BOOST_TEST(actual.getText() == "No FaceId specified and IncomingFaceId not available");
  });
}

BOOST_AUTO_TEST_SUITE_END() // UpdateFace
BOOST_AUTO_TEST_SUITE_END() // TestFaceManager
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace nfd::tests
