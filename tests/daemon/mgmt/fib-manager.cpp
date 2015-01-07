/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
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

#include "mgmt/fib-manager.hpp"
#include "table/fib.hpp"
#include "table/fib-nexthop.hpp"
#include "face/face.hpp"
#include "mgmt/internal-face.hpp"
#include "tests/daemon/face/dummy-face.hpp"

#include "validation-common.hpp"
#include "tests/test-common.hpp"

#include "fib-enumeration-publisher-common.hpp"

namespace nfd {
namespace tests {

NFD_LOG_INIT("FibManagerTest");

class FibManagerFixture : public FibEnumerationPublisherFixture
{
public:

  virtual
  ~FibManagerFixture()
  {
  }

  shared_ptr<Face>
  getFace(FaceId id)
  {
    if (id > 0 && static_cast<size_t>(id) <= m_faces.size())
      {
        return m_faces[id - 1];
      }
    NFD_LOG_DEBUG("No face found returning NULL");
    return shared_ptr<DummyFace>();
  }

  void
  addFace(shared_ptr<Face> face)
  {
    m_faces.push_back(face);
  }

  void
  validateControlResponseCommon(const Data& response,
                                const Name& expectedName,
                                uint32_t expectedCode,
                                const std::string& expectedText,
                                ControlResponse& control)
  {
    m_callbackFired = true;
    Block controlRaw = response.getContent().blockFromValue();

    control.wireDecode(controlRaw);

    // NFD_LOG_DEBUG("received control response"
    //               << " Name: " << response.getName()
    //               << " code: " << control.getCode()
    //               << " text: " << control.getText());

    BOOST_CHECK_EQUAL(response.getName(), expectedName);
    BOOST_CHECK_EQUAL(control.getCode(), expectedCode);
    BOOST_CHECK_EQUAL(control.getText(), expectedText);
  }

  void
  validateControlResponse(const Data& response,
                          const Name& expectedName,
                          uint32_t expectedCode,
                          const std::string& expectedText)
  {
    ControlResponse control;
    validateControlResponseCommon(response, expectedName,
                                  expectedCode, expectedText, control);

    if (!control.getBody().empty())
      {
        BOOST_FAIL("found unexpected control response body");
      }
  }

  void
  validateControlResponse(const Data& response,
                          const Name& expectedName,
                          uint32_t expectedCode,
                          const std::string& expectedText,
                          const Block& expectedBody)
  {
    ControlResponse control;
    validateControlResponseCommon(response, expectedName,
                                  expectedCode, expectedText, control);

    BOOST_REQUIRE(!control.getBody().empty());
    BOOST_REQUIRE_EQUAL(control.getBody().value_size(), expectedBody.value_size());

    BOOST_CHECK(memcmp(control.getBody().value(), expectedBody.value(),
                       expectedBody.value_size()) == 0);

  }

  bool
  didCallbackFire()
  {
    return m_callbackFired;
  }

  void
  resetCallbackFired()
  {
    m_callbackFired = false;
  }

  shared_ptr<InternalFace>
  getInternalFace()
  {
    return m_face;
  }

  FibManager&
  getFibManager()
  {
    return m_manager;
  }

  Fib&
  getFib()
  {
    return m_fib;
  }

  void
  addInterestRule(const std::string& regex,
                  ndn::IdentityCertificate& certificate)
  {
    m_manager.addInterestRule(regex, certificate);
  }

protected:
  FibManagerFixture()
    : m_manager(ref(m_fib), bind(&FibManagerFixture::getFace, this, _1), m_face, m_keyChain)
    , m_callbackFired(false)
  {
  }

protected:
  FibManager m_manager;

  std::vector<shared_ptr<Face> > m_faces;
  bool m_callbackFired;
  ndn::KeyChain m_keyChain;
};

template <typename T>
class AuthorizedCommandFixture : public CommandFixture<T>
{
public:
  AuthorizedCommandFixture()
  {
    const std::string regex = "^<localhost><nfd><fib>";
    T::addInterestRule(regex, *CommandFixture<T>::m_certificate);
  }

  virtual
  ~AuthorizedCommandFixture()
  {
  }
};

BOOST_FIXTURE_TEST_SUITE(MgmtFibManager, AuthorizedCommandFixture<FibManagerFixture>)

bool
foundNextHop(FaceId id, uint32_t cost, const fib::NextHop& next)
{
  return id == next.getFace()->getId() && next.getCost() == cost;
}

bool
addedNextHopWithCost(const Fib& fib, const Name& prefix, size_t oldSize, uint32_t cost)
{
  shared_ptr<fib::Entry> entry = fib.findExactMatch(prefix);

  if (static_cast<bool>(entry))
    {
      const fib::NextHopList& hops = entry->getNextHops();
      return hops.size() == oldSize + 1 &&
        std::find_if(hops.begin(), hops.end(), bind(&foundNextHop, -1, cost, _1)) != hops.end();
    }
  return false;
}

bool
foundNextHopWithFace(FaceId id, uint32_t cost,
                     shared_ptr<Face> face, const fib::NextHop& next)
{
  return id == next.getFace()->getId() && next.getCost() == cost && face == next.getFace();
}

bool
addedNextHopWithFace(const Fib& fib, const Name& prefix, size_t oldSize,
                     uint32_t cost, shared_ptr<Face> face)
{
  shared_ptr<fib::Entry> entry = fib.findExactMatch(prefix);

  if (static_cast<bool>(entry))
    {
      const fib::NextHopList& hops = entry->getNextHops();
      return hops.size() == oldSize + 1 &&
        std::find_if(hops.begin(), hops.end(), bind(&foundNextHop, -1, cost, _1)) != hops.end();
    }
  return false;
}

BOOST_AUTO_TEST_CASE(ShortName)
{
  shared_ptr<InternalFace> face = getInternalFace();

  shared_ptr<Interest> command = makeInterest("/localhost/nfd/fib");

  face->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 400, "Malformed command");
  });

  face->sendInterest(*command);
  g_io.run_one();

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_AUTO_TEST_CASE(MalformedCommmand)
{
  shared_ptr<InternalFace> face = getInternalFace();

  BOOST_REQUIRE(didCallbackFire() == false);

  Interest command("/localhost/nfd/fib");

  face->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command.getName(), 400, "Malformed command");
  });

  getFibManager().onFibRequest(command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_AUTO_TEST_CASE(UnsupportedVerb)
{
  shared_ptr<InternalFace> face = getInternalFace();

  ControlParameters parameters;
  parameters.setName("/hello");
  parameters.setFaceId(1);
  parameters.setCost(1);

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("unsupported");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  face->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 501, "Unsupported command");
  });

  getFibManager().onFibRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_AUTO_TEST_CASE(UnsignedCommand)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face = getInternalFace();

  ControlParameters parameters;
  parameters.setName("/hello");
  parameters.setFaceId(1);
  parameters.setCost(101);

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("add-nexthop");
  commandName.append(encodedParameters);

  Interest command(commandName);

  face->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command.getName(), 401, "Signature required");
  });

  getFibManager().onFibRequest(command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(!addedNextHopWithCost(getFib(), "/hello", 0, 101));
}

BOOST_FIXTURE_TEST_CASE(UnauthorizedCommand, UnauthorizedCommandFixture<FibManagerFixture>)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face = getInternalFace();

  ControlParameters parameters;
  parameters.setName("/hello");
  parameters.setFaceId(1);
  parameters.setCost(101);

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("add-nexthop");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  face->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 403, "Unauthorized command");
  });

  getFibManager().onFibRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(!addedNextHopWithCost(getFib(), "/hello", 0, 101));
}

BOOST_AUTO_TEST_CASE(BadOptionParse)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face = getInternalFace();

  Name commandName("/localhost/nfd/fib");
  commandName.append("add-nexthop");
  commandName.append("NotReallyParameters");

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  face->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 400, "Malformed command");
  });

  getFibManager().onFibRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_AUTO_TEST_CASE(UnknownFaceId)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face = getInternalFace();

  ControlParameters parameters;
  parameters.setName("/hello");
  parameters.setFaceId(1000);
  parameters.setCost(101);

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("add-nexthop");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  face->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 410, "Face not found");
  });

  getFibManager().onFibRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(addedNextHopWithCost(getFib(), "/hello", 0, 101) == false);
}

BOOST_AUTO_TEST_CASE(AddNextHopVerbImplicitFaceId)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face = getInternalFace();

  std::vector<ControlParameters> testedParameters;
  testedParameters.push_back(ControlParameters().setName("/hello").setCost(101).setFaceId(0));
  testedParameters.push_back(ControlParameters().setName("/hello").setCost(101));

  for (std::vector<ControlParameters>::iterator parameters = testedParameters.begin();
       parameters != testedParameters.end(); ++parameters) {

    Block encodedParameters(parameters->wireEncode());

    Name commandName("/localhost/nfd/fib");
    commandName.append("add-nexthop");
    commandName.append(encodedParameters);

    ControlParameters expectedParameters;
    expectedParameters.setName("/hello");
    expectedParameters.setFaceId(1);
    expectedParameters.setCost(101);

    Block encodedExpectedParameters(expectedParameters.wireEncode());

    shared_ptr<Interest> command(make_shared<Interest>(commandName));
    command->setIncomingFaceId(1);
    generateCommand(*command);

    signal::Connection conn = face->onReceiveData.connect(
        [this, command, encodedExpectedParameters] (const Data& response) {
          this->validateControlResponse(response, command->getName(),
                                        200, "Success", encodedExpectedParameters);
        });

    getFibManager().onFibRequest(*command);

    BOOST_REQUIRE(didCallbackFire());
    BOOST_REQUIRE(addedNextHopWithFace(getFib(), "/hello", 0, 101, getFace(1)));

    conn.disconnect();
    getFib().erase("/hello");
    BOOST_REQUIRE_EQUAL(getFib().size(), 0);
  }
}

BOOST_AUTO_TEST_CASE(AddNextHopVerbInitialAdd)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face = getInternalFace();

  ControlParameters parameters;
  parameters.setName("/hello");
  parameters.setFaceId(1);
  parameters.setCost(101);

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("add-nexthop");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  face->onReceiveData.connect([this, command, encodedParameters] (const Data& response) {
    this->validateControlResponse(response, command->getName(),
                                  200, "Success", encodedParameters);
  });

  getFibManager().onFibRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(addedNextHopWithCost(getFib(), "/hello", 0, 101));
}

BOOST_AUTO_TEST_CASE(AddNextHopVerbImplicitCost)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face = getInternalFace();

  ControlParameters parameters;
  parameters.setName("/hello");
  parameters.setFaceId(1);

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("add-nexthop");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  ControlParameters resultParameters;
  resultParameters.setName("/hello");
  resultParameters.setFaceId(1);
  resultParameters.setCost(0);

  face->onReceiveData.connect([this, command, resultParameters] (const Data& response) {
    this->validateControlResponse(response, command->getName(),
                                  200, "Success", resultParameters.wireEncode());
  });

  getFibManager().onFibRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(addedNextHopWithCost(getFib(), "/hello", 0, 0));
}

BOOST_AUTO_TEST_CASE(AddNextHopVerbAddToExisting)
{
  addFace(make_shared<DummyFace>());
  shared_ptr<InternalFace> face = getInternalFace();

  for (int i = 1; i <= 2; i++)
    {

      ControlParameters parameters;
      parameters.setName("/hello");
      parameters.setFaceId(1);
      parameters.setCost(100 + i);

      Block encodedParameters(parameters.wireEncode());

      Name commandName("/localhost/nfd/fib");
      commandName.append("add-nexthop");
      commandName.append(encodedParameters);

      shared_ptr<Interest> command(make_shared<Interest>(commandName));
      generateCommand(*command);

      signal::Connection conn = face->onReceiveData.connect(
          [this, command, encodedParameters] (const Data& response) {
            this->validateControlResponse(response, command->getName(),
                                          200, "Success", encodedParameters);
          });

      getFibManager().onFibRequest(*command);
      BOOST_REQUIRE(didCallbackFire());
      resetCallbackFired();

      shared_ptr<fib::Entry> entry = getFib().findExactMatch("/hello");

      if (static_cast<bool>(entry))
        {
          const fib::NextHopList& hops = entry->getNextHops();
          BOOST_REQUIRE(hops.size() == 1);
          BOOST_REQUIRE(std::find_if(hops.begin(), hops.end(),
                                     bind(&foundNextHop, -1, 100 + i, _1)) != hops.end());

        }
      else
        {
          BOOST_FAIL("Failed to find expected fib entry");
        }

      conn.disconnect();
    }
}

BOOST_AUTO_TEST_CASE(AddNextHopVerbUpdateFaceCost)
{
  addFace(make_shared<DummyFace>());
  shared_ptr<InternalFace> face = getInternalFace();

  ControlParameters parameters;
  parameters.setName("/hello");
  parameters.setFaceId(1);

  {
    parameters.setCost(1);

    Block encodedParameters(parameters.wireEncode());

    Name commandName("/localhost/nfd/fib");
    commandName.append("add-nexthop");
    commandName.append(encodedParameters);

    shared_ptr<Interest> command(make_shared<Interest>(commandName));
    generateCommand(*command);

    signal::Connection conn = face->onReceiveData.connect(
        [this, command, encodedParameters] (const Data& response) {
          this->validateControlResponse(response, command->getName(),
                                        200, "Success", encodedParameters);
        });

    getFibManager().onFibRequest(*command);

    BOOST_REQUIRE(didCallbackFire());

    resetCallbackFired();
    conn.disconnect();
  }

  {
    parameters.setCost(102);

    Block encodedParameters(parameters.wireEncode());

    Name commandName("/localhost/nfd/fib");
    commandName.append("add-nexthop");
    commandName.append(encodedParameters);

    shared_ptr<Interest> command(make_shared<Interest>(commandName));
    generateCommand(*command);

    face->onReceiveData.connect([this, command, encodedParameters] (const Data& response) {
      this->validateControlResponse(response, command->getName(),
                                    200, "Success", encodedParameters);
    });

    getFibManager().onFibRequest(*command);

    BOOST_REQUIRE(didCallbackFire());
  }

  shared_ptr<fib::Entry> entry = getFib().findExactMatch("/hello");

  // Add faces with cost == FaceID for the name /hello
  // This test assumes:
  //   FaceIDs are -1 because we don't add them to a forwarder
  if (static_cast<bool>(entry))
    {
      const fib::NextHopList& hops = entry->getNextHops();
      BOOST_REQUIRE(hops.size() == 1);
      BOOST_REQUIRE(std::find_if(hops.begin(),
                                 hops.end(),
                                 bind(&foundNextHop, -1, 102, _1)) != hops.end());
    }
  else
    {
      BOOST_FAIL("Failed to find expected fib entry");
    }
}

BOOST_AUTO_TEST_CASE(AddNextHopVerbMissingPrefix)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face = getInternalFace();

  ControlParameters parameters;
  parameters.setFaceId(1);

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("add-nexthop");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  face->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 400, "Malformed command");
  });

  getFibManager().onFibRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
}

bool
removedNextHopWithCost(const Fib& fib, const Name& prefix, size_t oldSize, uint32_t cost)
{
  shared_ptr<fib::Entry> entry = fib.findExactMatch(prefix);

  if (static_cast<bool>(entry))
    {
      const fib::NextHopList& hops = entry->getNextHops();
      return hops.size() == oldSize - 1 &&
        std::find_if(hops.begin(), hops.end(), bind(&foundNextHop, -1, cost, _1)) == hops.end();
    }
  return false;
}

void
testRemoveNextHop(CommandFixture<FibManagerFixture>* fixture,
                  FibManager& manager,
                  Fib& fib,
                  shared_ptr<Face> face,
                  const Name& targetName,
                  FaceId targetFace)
{
  ControlParameters parameters;
  parameters.setName(targetName);
  parameters.setFaceId(targetFace);

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("remove-nexthop");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  fixture->generateCommand(*command);

  signal::Connection conn = face->onReceiveData.connect(
      [fixture, command, encodedParameters] (const Data& response) {
        fixture->validateControlResponse(response, command->getName(),
                                         200, "Success", encodedParameters);
      });

  manager.onFibRequest(*command);

  BOOST_REQUIRE(fixture->didCallbackFire());

  fixture->resetCallbackFired();
  conn.disconnect();
}

BOOST_AUTO_TEST_CASE(RemoveNextHop)
{
  shared_ptr<Face> face1 = make_shared<DummyFace>();
  shared_ptr<Face> face2 = make_shared<DummyFace>();
  shared_ptr<Face> face3 = make_shared<DummyFace>();

  addFace(face1);
  addFace(face2);
  addFace(face3);

  shared_ptr<InternalFace> face = getInternalFace();
  FibManager& manager = getFibManager();
  Fib& fib = getFib();

  shared_ptr<fib::Entry> entry = fib.insert("/hello").first;

  entry->addNextHop(face1, 101);
  entry->addNextHop(face2, 202);
  entry->addNextHop(face3, 303);

  testRemoveNextHop(this, manager, fib, face, "/hello", 2);
  BOOST_REQUIRE(removedNextHopWithCost(fib, "/hello", 3, 202));

  testRemoveNextHop(this, manager, fib, face, "/hello", 3);
  BOOST_REQUIRE(removedNextHopWithCost(fib, "/hello", 2, 303));

  testRemoveNextHop(this, manager, fib, face, "/hello", 1);
  // BOOST_REQUIRE(removedNextHopWithCost(fib, "/hello", 1, 101));

  BOOST_CHECK(!static_cast<bool>(getFib().findExactMatch("/hello")));
}

BOOST_AUTO_TEST_CASE(RemoveFaceNotFound)
{
  shared_ptr<InternalFace> face = getInternalFace();

  ControlParameters parameters;
  parameters.setName("/hello");
  parameters.setFaceId(1);

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("remove-nexthop");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  face->onReceiveData.connect([this, command, encodedParameters] (const Data& response) {
    this->validateControlResponse(response, command->getName(),
                                  200, "Success", encodedParameters);
  });

  getFibManager().onFibRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_AUTO_TEST_CASE(RemovePrefixNotFound)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face = getInternalFace();

  ControlParameters parameters;
  parameters.setName("/hello");
  parameters.setFaceId(1);

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("remove-nexthop");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  face->onReceiveData.connect([this, command, encodedParameters] (const Data& response) {
    this->validateControlResponse(response, command->getName(),
                                  200, "Success", encodedParameters);
  });

  getFibManager().onFibRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_AUTO_TEST_CASE(RemoveMissingPrefix)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face = getInternalFace();

  ControlParameters parameters;
  parameters.setFaceId(1);

  Block encodedParameters(parameters.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("remove-nexthop");
  commandName.append(encodedParameters);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  face->onReceiveData.connect([this, command] (const Data& response) {
    this->validateControlResponse(response, command->getName(), 400, "Malformed command");
  });

  getFibManager().onFibRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_AUTO_TEST_CASE(RemoveImplicitFaceId)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face = getInternalFace();

  std::vector<ControlParameters> testedParameters;
  testedParameters.push_back(ControlParameters().setName("/hello").setFaceId(0));
  testedParameters.push_back(ControlParameters().setName("/hello"));

  for (std::vector<ControlParameters>::iterator parameters = testedParameters.begin();
       parameters != testedParameters.end(); ++parameters) {
    Block encodedParameters(parameters->wireEncode());

    Name commandName("/localhost/nfd/fib");
    commandName.append("remove-nexthop");
    commandName.append(encodedParameters);

    shared_ptr<Interest> command(make_shared<Interest>(commandName));
    command->setIncomingFaceId(1);
    generateCommand(*command);

    ControlParameters resultParameters;
    resultParameters.setFaceId(1);
    resultParameters.setName("/hello");

    signal::Connection conn = face->onReceiveData.connect(
        [this, command, resultParameters] (const Data& response) {
          this->validateControlResponse(response, command->getName(),
                                        200, "Success", resultParameters.wireEncode());
        });

    getFibManager().onFibRequest(*command);

    BOOST_REQUIRE(didCallbackFire());

    conn.disconnect();
  }
}

BOOST_FIXTURE_TEST_CASE(TestFibEnumerationRequest, FibManagerFixture)
{
  for (int i = 0; i < 87; i++)
    {
      Name prefix("/test");
      prefix.appendSegment(i);

      shared_ptr<DummyFace> dummy1(make_shared<DummyFace>());
      shared_ptr<DummyFace> dummy2(make_shared<DummyFace>());

      shared_ptr<fib::Entry> entry = m_fib.insert(prefix).first;
      entry->addNextHop(dummy1, std::numeric_limits<uint64_t>::max() - 1);
      entry->addNextHop(dummy2, std::numeric_limits<uint64_t>::max() - 2);

      m_referenceEntries.insert(entry);
    }
  for (int i = 0; i < 2; i++)
    {
      Name prefix("/test2");
      prefix.appendSegment(i);

      shared_ptr<DummyFace> dummy1(make_shared<DummyFace>());
      shared_ptr<DummyFace> dummy2(make_shared<DummyFace>());

      shared_ptr<fib::Entry> entry = m_fib.insert(prefix).first;
      entry->addNextHop(dummy1, std::numeric_limits<uint8_t>::max() - 1);
      entry->addNextHop(dummy2, std::numeric_limits<uint8_t>::max() - 2);

      m_referenceEntries.insert(entry);
    }

  ndn::EncodingBuffer buffer;

  m_face->onReceiveData.connect(bind(&FibEnumerationPublisherFixture::decodeFibEntryBlock,
                                     this, _1));

  shared_ptr<Interest> command(make_shared<Interest>("/localhost/nfd/fib/list"));

  m_manager.onFibRequest(*command);
  BOOST_REQUIRE(m_finished);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
