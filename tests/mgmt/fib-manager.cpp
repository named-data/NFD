/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "mgmt/fib-manager.hpp"
#include "fw/forwarder.hpp"
#include "table/fib.hpp"
#include "table/fib-nexthop.hpp"
#include "face/face.hpp"
#include "mgmt/internal-face.hpp"
#include "../face/dummy-face.hpp"

#include <algorithm>

#include <ndn-cpp-dev/management/fib-management-options.hpp>
#include <ndn-cpp-dev/management/control-response.hpp>

#include <boost/test/unit_test.hpp>

namespace nfd {

NFD_LOG_INIT("FibManagerTest");

class FibManagerFixture
{
public:

  FibManagerFixture()
    : m_callbackFired(false)
  {

  }

  shared_ptr<Face>
  getFace(FaceId id)
  {
    if (id > 0 && id <= m_faces.size())
      {
        return m_faces[id-1];
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
  validateControlResponse(const Data& response,
                          uint32_t expectedCode,
                          const std::string& expectedText)
  {
    m_callbackFired = true;
    Block controlRaw = response.getContent().blockFromValue();

    ndn::ControlResponse control;
    control.wireDecode(controlRaw);

    NFD_LOG_DEBUG("received control response"
                  << " Name: " << response.getName()
                  << " code: " << control.getCode()
                  << " text: " << control.getText());

    BOOST_REQUIRE(control.getCode() == expectedCode);
    BOOST_REQUIRE(control.getText() == expectedText);
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

private:
  std::vector<shared_ptr<Face> > m_faces;
  bool m_callbackFired;
};


BOOST_AUTO_TEST_SUITE(MgmtFibManager)



bool
foundNextHop(FaceId id, uint32_t cost, const fib::NextHop& next)
{
  return id == next.getFace()->getId() && next.getCost() == cost;
}

bool
addedNextHopWithCost(const Fib& fib, const Name& prefix, size_t oldSize, uint32_t cost)
{
  shared_ptr<fib::Entry> entry = fib.findLongestPrefixMatch(prefix);

  if (static_cast<bool>(entry))
    {
      const fib::NextHopList& hops = entry->getNextHops();
      return hops.size() == oldSize + 1 &&
        std::find_if(hops.begin(), hops.end(), bind(&foundNextHop, -1, cost, _1)) != hops.end();
    }
  return false;
}

BOOST_AUTO_TEST_CASE(TestFireInterestFilter)
{
  FibManagerFixture fixture;
  shared_ptr<InternalFace> face(make_shared<InternalFace>());
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace,
                          &fixture, _1),
                          face);

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, &fixture,  _1, 400, "Malformed command");

  Interest command("/localhost/nfd/fib");
  face->sendInterest(command);
}

BOOST_AUTO_TEST_CASE(MalformedCommmand)
{
  FibManagerFixture fixture;
  shared_ptr<InternalFace> face(make_shared<InternalFace>());
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace,
                          &fixture, _1),
                          face);

  BOOST_REQUIRE(fixture.didCallbackFire() == false);

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, &fixture, _1, 400, "Malformed command");

  Interest command("/localhost/nfd/fib");
  manager.onFibRequest(command);

  BOOST_REQUIRE(fixture.didCallbackFire());
}

BOOST_FIXTURE_TEST_CASE(UnsupportedVerb, FibManagerFixture)
{
  shared_ptr<InternalFace> face(make_shared<InternalFace>());
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace,
                          this, _1),
                          face);

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1, 501, "Unsupported command");

  ndn::FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(1);
  options.setCost(1);

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("unsupported");
  commandName.append(encodedOptions);

  Interest command(commandName);
  manager.onFibRequest(command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_FIXTURE_TEST_CASE(UnsignedCommand, FibManagerFixture)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(make_shared<InternalFace>());
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace,
                          this, _1),
                     face);
  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1, 200, "OK");
  /// \todo enable once sig checking implement
    // bind(&FibManagerFixture::validateControlResponse, this, _1, 401, "SIGREG");

  ndn::FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(1);
  options.setCost(101);

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("add-nexthop");
  commandName.append(encodedOptions);

  Interest command(commandName);
  manager.onFibRequest(command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(addedNextHopWithCost(fib, "/hello", 0, 101));
}

BOOST_FIXTURE_TEST_CASE(UnauthorizedCommand, FibManagerFixture)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(make_shared<InternalFace>());
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace,
                          this, _1),
                     face);
  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1, 200, "OK");
  /// \todo enable once sig checking implement
    // bind(&FibManagerFixture::validateControlResponse, this, _1, 403, "Unauthorized command");

  ndn::FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(1);
  options.setCost(101);

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("add-nexthop");
  commandName.append(encodedOptions);

  Interest command(commandName);
  manager.onFibRequest(command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(addedNextHopWithCost(fib, "/hello", 0, 101));
}

BOOST_FIXTURE_TEST_CASE(BadOptionParse, FibManagerFixture)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(make_shared<InternalFace>());
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace,
                          this, _1),
                     face);

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1, 400, "Malformed command");

  Name commandName("/localhost/nfd/fib");
  commandName.append("add-nexthop");
  commandName.append("NotReallyOptions");

  Interest command(commandName);
  manager.onFibRequest(command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_FIXTURE_TEST_CASE(UnknownFaceId, FibManagerFixture)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(make_shared<InternalFace>());
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace,
                          this, _1),
                     face);

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1, 400, "Malformed command");

  ndn::FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(1000);
  options.setCost(101);

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("add-nexthop");
  commandName.append(encodedOptions);

  Interest command(commandName);
  manager.onFibRequest(command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(addedNextHopWithCost(fib, "/hello", 0, 101) == false);
}

BOOST_FIXTURE_TEST_CASE(AddNextHopVerbInitialAdd, FibManagerFixture)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(make_shared<InternalFace>());
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace,
                          this, _1),
                          face);
  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1, 200, "OK");

  ndn::FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(1);
  options.setCost(101);

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("add-nexthop");
  commandName.append(encodedOptions);

  Interest command(commandName);
  manager.onFibRequest(command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(addedNextHopWithCost(fib, "/hello", 0, 101));
}

BOOST_FIXTURE_TEST_CASE(AddNextHopVerbAddToExisting, FibManagerFixture)
{
  addFace(make_shared<DummyFace>());
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(make_shared<InternalFace>());
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace,
                          this, _1),
                          face);
  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1, 200, "OK");

  // Add faces with cost == FaceID for the name /hello
  // This test assumes:
  //   FaceIDs are -1 because we don't add them to a forwarder

  for (int i = 1; i <= 2; i++)
    {
      ndn::FibManagementOptions options;
      options.setName("/hello");
      options.setFaceId(i);
      options.setCost(100 + i);

      Block encodedOptions(options.wireEncode());

      Name commandName("/localhost/nfd/fib");
      commandName.append("add-nexthop");
      commandName.append(encodedOptions);

      Interest command(commandName);
      manager.onFibRequest(command);
      BOOST_REQUIRE(didCallbackFire());
      resetCallbackFired();

      shared_ptr<fib::Entry> entry = fib.findLongestPrefixMatch("/hello");

      if (static_cast<bool>(entry))
        {
          const fib::NextHopList& hops = entry->getNextHops();
          for (int j = 1; j <= i; j++)
            {
              BOOST_REQUIRE(addedNextHopWithCost(fib, "/hello", i - 1, 100 + j));
            }
        }
      else
        {
          BOOST_FAIL("Failed to find expected fib entry");
        }
    }
}

BOOST_FIXTURE_TEST_CASE(AddNextHopVerbUpdateFaceCost, FibManagerFixture)
{
  FibManagerFixture fixture;
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(make_shared<InternalFace>());
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace,
                          this, _1),
                          face);
  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1, 200, "OK");

  ndn::FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(1);

  {
    options.setCost(1);

    Block encodedOptions(options.wireEncode());

    Name commandName("/localhost/nfd/fib");
    commandName.append("add-nexthop");
    commandName.append(encodedOptions);

    Interest command(commandName);
    manager.onFibRequest(command);
    BOOST_REQUIRE(didCallbackFire());
    resetCallbackFired();
  }

  {
    options.setCost(102);

    Block encodedOptions(options.wireEncode());

    Name commandName("/localhost/nfd/fib");
    commandName.append("add-nexthop");
    commandName.append(encodedOptions);

    Interest command(commandName);
    manager.onFibRequest(command);
    BOOST_REQUIRE(didCallbackFire());
  }

  shared_ptr<fib::Entry> entry = fib.findLongestPrefixMatch("/hello");

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

BOOST_AUTO_TEST_SUITE_END()

} // namespace nfd
