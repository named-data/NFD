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

#include "mgmt/face-manager.hpp"
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
    BOOST_TEST_MESSAGE(actual.getText());
    BOOST_CHECK_EQUAL(expected.getCode(), actual.getCode());

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

BOOST_FIXTURE_TEST_CASE(ExistingFace, FaceManagerCommandFixture)
{
  using FaceType = UdpFacePersistent;

  {
    // create face

    Name commandName("/localhost/nfd/faces");
    commandName.append("create");
    commandName.append(FaceType().getParameters().wireEncode());
    auto command = makeInterest(commandName);
    m_keyChain.sign(*command);

    this->node1.face.receive(*command);
    this->advanceClocks(time::milliseconds(1), 5);
  }

  // find the created face
  auto foundFace = this->node1.findFaceByUri(FaceType().getParameters().getUri());
  BOOST_REQUIRE(foundFace != nullptr);

  {
    // re-create face

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
        BOOST_REQUIRE_EQUAL(actual.getCode(), 409);

        ControlParameters actualParams(actual.getBody());
        BOOST_CHECK_EQUAL(foundFace->getId(), actualParams.getFaceId());
        BOOST_CHECK_EQUAL(foundFace->getRemoteUri().toString(), actualParams.getUri());
        BOOST_CHECK_EQUAL(foundFace->getPersistency(), actualParams.getFacePersistency());

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
