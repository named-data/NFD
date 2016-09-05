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

#include <thread>

namespace nfd {
namespace tests {

BOOST_AUTO_TEST_SUITE(Mgmt)
BOOST_AUTO_TEST_SUITE(TestFaceManager)

BOOST_FIXTURE_TEST_SUITE(CreateFace, BaseFixture)

class TcpFaceOnDemand
{
public:
  ControlParameters
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
  ControlParameters
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
  ControlParameters
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
  ControlParameters
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
  ControlParameters
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
  ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("udp4://127.0.0.1:26363")
      .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERMANENT);
  }
};

class UdpFaceConnectToSelf // face that will cause afterCreateFaceFailure to be invoked
                           // fails because remote endpoint is prohibited
{
public:
  ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("udp4://0.0.0.0:16363"); // cannot connect to self
  }
};

class LocalTcpFaceLocalFieldsEnabled
{
public:
  ControlParameters
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
  ControlParameters
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
  ControlParameters
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
  ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("udp4://127.0.0.1:26363")
      .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT)
      .setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, false);
  }
};

namespace mpl = boost::mpl;

// pairs of CreateCommand and Success/Failure status
typedef mpl::vector<mpl::pair<TcpFaceOnDemand, CommandFailure<406>>,
                    mpl::pair<TcpFacePersistent, CommandSuccess>,
                    mpl::pair<TcpFacePermanent, CommandFailure<406>>,
                    mpl::pair<UdpFaceOnDemand, CommandFailure<406>>,
                    mpl::pair<UdpFacePersistent, CommandSuccess>,
                    mpl::pair<UdpFacePermanent, CommandSuccess>,
                    mpl::pair<UdpFaceConnectToSelf, CommandFailure<406>>,
                    mpl::pair<LocalTcpFaceLocalFieldsEnabled, CommandSuccess>,
                    mpl::pair<LocalTcpFaceLocalFieldsDisabled, CommandSuccess>,
                    mpl::pair<NonLocalUdpFaceLocalFieldsEnabled, CommandFailure<406>>,
                    mpl::pair<NonLocalUdpFaceLocalFieldsDisabled, CommandSuccess>> Faces;

BOOST_FIXTURE_TEST_CASE_TEMPLATE(NewFace, T, Faces, FaceManagerCommandFixture)
{
  typedef typename T::first FaceType;
  typedef typename T::second CreateResult;

  Name commandName("/localhost/nfd/faces");
  commandName.append("create");
  commandName.append(FaceType().getParameters().wireEncode());
  auto command = makeInterest(commandName);
  m_keyChain.sign(*command);

  bool hasCallbackFired = false;
  this->node1.face.onSendData.connect([this, command, &hasCallbackFired] (const Data& response) {
    if (!command->getName().isPrefixOf(response.getName())) {
      return;
    }

    ControlResponse actual(response.getContent().blockFromValue());
    ControlResponse expected(CreateResult().getExpected());
    BOOST_CHECK_EQUAL(expected.getCode(), actual.getCode());
    BOOST_TEST_MESSAGE(actual.getText());

    if (actual.getBody().hasWire()) {
      ControlParameters expectedParams(FaceType().getParameters());
      ControlParameters actualParams(actual.getBody());

      BOOST_CHECK(actualParams.hasFaceId());
      BOOST_CHECK_EQUAL(expectedParams.getFacePersistency(), actualParams.getFacePersistency());

      if (actual.getCode() == 200) {
        if (expectedParams.hasFlags()) {
          BOOST_CHECK_EQUAL(expectedParams.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED),
                            actualParams.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED));
        }
        else {
          // local fields are disabled by default
          BOOST_CHECK_EQUAL(actualParams.getFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED), false);
        }
      }
      else {
        BOOST_CHECK_EQUAL(expectedParams.getUri(), actualParams.getUri());
      }
    }

    if (actual.getCode() != 200) {
      // ensure face not created
      FaceUri uri(FaceType().getParameters().getUri());
      auto& faceTable = this->node1.manager.m_faceTable;
      BOOST_CHECK(std::none_of(faceTable.begin(), faceTable.end(), [uri] (Face& face) {
        return face.getRemoteUri() == uri;
      }));
    }

    hasCallbackFired = true;
  });

  this->node1.face.receive(*command);
  this->advanceClocks(time::milliseconds(1), 5);

  BOOST_CHECK(hasCallbackFired);
}


typedef mpl::vector<// mpl::pair<mpl::pair<TcpFacePersistent, TcpFacePermanent>, TcpFacePermanent>, // no need to check now
                    // mpl::pair<mpl::pair<TcpFacePermanent, TcpFacePersistent>, TcpFacePersistent>, // no need to check now
                    mpl::pair<mpl::pair<UdpFacePersistent, UdpFacePermanent>, UdpFacePermanent>,
                    mpl::pair<mpl::pair<UdpFacePermanent, UdpFacePersistent>, UdpFacePermanent>> FaceTransitions;


BOOST_FIXTURE_TEST_CASE_TEMPLATE(ExistingFace, T, FaceTransitions, FaceManagerCommandFixture)
{
  typedef typename T::first::first FaceType1;
  typedef typename T::first::second FaceType2;
  typedef typename T::second FinalFaceType;

  {
    // create face

    Name commandName("/localhost/nfd/faces");
    commandName.append("create");
    commandName.append(FaceType1().getParameters().wireEncode());
    auto command = makeInterest(commandName);
    m_keyChain.sign(*command);

    this->node1.face.receive(*command);
    this->advanceClocks(time::milliseconds(1), 5);
  }

  //
  {
    // re-create face (= change face persistency)

    Name commandName("/localhost/nfd/faces");
    commandName.append("create");
    commandName.append(FaceType2().getParameters().wireEncode());
    auto command = makeInterest(commandName);
    m_keyChain.sign(*command);

    bool hasCallbackFired = false;
    this->node1.face.onSendData.connect([this, command, &hasCallbackFired] (const Data& response) {
        if (!command->getName().isPrefixOf(response.getName())) {
          return;
        }

        ControlResponse actual(response.getContent().blockFromValue());
        BOOST_REQUIRE_EQUAL(actual.getCode(), 200);
        BOOST_TEST_MESSAGE(actual.getText());

        ControlParameters expectedParams(FinalFaceType().getParameters());
        ControlParameters actualParams(actual.getBody());
        BOOST_REQUIRE(!actualParams.hasUri()); // Uri parameter is only included on command failure
        BOOST_CHECK_EQUAL(expectedParams.getFacePersistency(), actualParams.getFacePersistency());

        hasCallbackFired = true;
      });

    this->node1.face.receive(*command);
    this->advanceClocks(time::milliseconds(1), 5);

    BOOST_CHECK(hasCallbackFired);
  }
}


class UdpFace
{
public:
  ControlParameters
  getParameters()
  {
    return ControlParameters()
      .setUri("udp4://127.0.0.1:16363")
      .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  }
};


// Note that the transitions from on-demand TcpFace are intentionally not tested.
// On-demand TcpFace has a remote endpoint with a randomized port number.  Normal face
// creation operations will not need to create a face toward a remote port not listened by
// a channel.

typedef mpl::vector<mpl::pair<UdpFace, UdpFacePersistent>,
                    mpl::pair<UdpFace, UdpFacePermanent>> OnDemandFaceTransitions;

// need a slightly different logic to test transitions from OnDemand state
BOOST_FIXTURE_TEST_CASE_TEMPLATE(ExistingFaceOnDemand, T, OnDemandFaceTransitions, FaceManagerCommandFixture)
{
  typedef typename T::first  OtherNodeFace;
  typedef typename T::second FaceType;

  {
    // create on-demand face

    Name commandName("/localhost/nfd/faces");
    commandName.append("create");
    commandName.append(OtherNodeFace().getParameters().wireEncode());
    auto command = makeInterest(commandName);
    m_keyChain.sign(*command);

    ndn::util::signal::ScopedConnection connection =
      this->node2.face.onSendData.connect([this, command] (const Data& response) {
        if (!command->getName().isPrefixOf(response.getName())) {
          return;
        }

        ControlResponse controlResponse(response.getContent().blockFromValue());
        BOOST_REQUIRE_EQUAL(controlResponse.getText(), "OK");
        BOOST_REQUIRE_EQUAL(controlResponse.getCode(), 200);
        BOOST_REQUIRE(!ControlParameters(controlResponse.getBody()).hasUri());
        uint64_t faceId = ControlParameters(controlResponse.getBody()).getFaceId();
        auto face = this->node2.faceTable.get(static_cast<FaceId>(faceId));

        // to force creation of on-demand face
        auto dummyInterest = make_shared<Interest>("/hello/world");
        face->sendInterest(*dummyInterest);
      });

    this->node2.face.receive(*command);
    this->advanceClocks(time::milliseconds(1), 5); // let node2 process command and send Interest
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // allow wallclock time for socket IO
    this->advanceClocks(time::milliseconds(1), 5); // let node1 accept Interest and create on-demand face
  }

  // make sure there is on-demand face
  FaceUri onDemandFaceUri(FaceType().getParameters().getUri());
  const Face* foundFace = nullptr;
  for (const Face& face : this->node1.faceTable) {
    if (face.getRemoteUri() == onDemandFaceUri) {
      foundFace = &face;
      break;
    }
  }
  BOOST_REQUIRE_MESSAGE(foundFace != nullptr, "on-demand face is not created");

  //
  {
    // re-create face (= change face persistency)

    Name commandName("/localhost/nfd/faces");
    commandName.append("create");
    commandName.append(FaceType().getParameters().wireEncode());
    auto command = makeInterest(commandName);
    m_keyChain.sign(*command);

    bool hasCallbackFired = false;
    this->node1.face.onSendData.connect(
      [this, command, &hasCallbackFired, foundFace] (const Data& response) {
        if (!command->getName().isPrefixOf(response.getName())) {
          return;
        }

        ControlResponse actual(response.getContent().blockFromValue());
        BOOST_REQUIRE_EQUAL(actual.getCode(), 200);

        ControlParameters expectedParams(FaceType().getParameters());
        ControlParameters actualParams(actual.getBody());
        BOOST_REQUIRE(!actualParams.hasUri());
        BOOST_CHECK_EQUAL(actualParams.getFacePersistency(), expectedParams.getFacePersistency());
        BOOST_CHECK_EQUAL(actualParams.getFaceId(), foundFace->getId());
        BOOST_CHECK_EQUAL(foundFace->getPersistency(), expectedParams.getFacePersistency());

        hasCallbackFired = true;
      });

    this->node1.face.receive(*command);
    this->advanceClocks(time::milliseconds(1), 5);

    BOOST_CHECK(hasCallbackFired);
  }
}

BOOST_AUTO_TEST_SUITE_END() // CreateFace
BOOST_AUTO_TEST_SUITE_END() // TestFaceManager
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace tests
} // namespace nfd
