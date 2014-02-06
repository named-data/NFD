/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "mgmt/fib-manager.hpp"
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
                          const Name& expectedName,
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

    BOOST_CHECK_EQUAL(response.getName(), expectedName);
    BOOST_CHECK_EQUAL(control.getCode(), expectedCode);
    BOOST_CHECK_EQUAL(control.getText(), expectedText);
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
  shared_ptr<fib::Entry> entry = fib.findExactMatch(prefix);

  if (static_cast<bool>(entry))
    {
      const fib::NextHopList& hops = entry->getNextHops();
      return hops.size() == oldSize + 1 &&
        std::find_if(hops.begin(), hops.end(), bind(&foundNextHop, -1, cost, _1)) != hops.end();
    }
  return false;
}

BOOST_FIXTURE_TEST_CASE(TestFireInterestFilter, FibManagerFixture)
{
  shared_ptr<InternalFace> face(make_shared<InternalFace>());
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace, this, _1),
                     face);

  Interest command("/localhost/nfd/fib");

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this,  _1,
         command.getName(), 400, "Malformed command");

  face->sendInterest(command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_FIXTURE_TEST_CASE(MalformedCommmand, FibManagerFixture)
{
  shared_ptr<InternalFace> face(make_shared<InternalFace>());
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace, this, _1),
                          face);

  BOOST_REQUIRE(didCallbackFire() == false);

  Interest command("/localhost/nfd/fib");

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1,
         command.getName(), 400, "Malformed command");

  manager.onFibRequest(command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_FIXTURE_TEST_CASE(UnsupportedVerb, FibManagerFixture)
{
  shared_ptr<InternalFace> face(make_shared<InternalFace>());
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace, this, _1),
                          face);

  ndn::FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(1);
  options.setCost(1);

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("unsupported");
  commandName.append(encodedOptions);

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1,
         commandName, 501, "Unsupported command");

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
                     bind(&FibManagerFixture::getFace, this, _1),
                     face);

  ndn::FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(1);
  options.setCost(101);

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("add-nexthop");
  commandName.append(encodedOptions);

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1,
         commandName, 404, "Prefix not found");
  /// \todo enable once sig checking implemented
    // bind(&FibManagerFixture::validateControlResponse, this, _1, 401, "Signature required");

  Interest command(commandName);
  manager.onFibRequest(command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(!addedNextHopWithCost(fib, "/hello", 0, 101));
}

BOOST_FIXTURE_TEST_CASE(UnauthorizedCommand, FibManagerFixture)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(make_shared<InternalFace>());
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace, this, _1),
                     face);

  ndn::FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(1);
  options.setCost(101);

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("add-nexthop");
  commandName.append(encodedOptions);

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1,
         commandName, 404, "Prefix not found");
  /// \todo enable once sig checking implemented
    // bind(&FibManagerFixture::validateControlResponse, this, _1, 403, "Unauthorized command");

  Interest command(commandName);
  manager.onFibRequest(command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(!addedNextHopWithCost(fib, "/hello", 0, 101));
}

BOOST_FIXTURE_TEST_CASE(BadOptionParse, FibManagerFixture)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(make_shared<InternalFace>());
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace, this, _1),
                     face);

  Name commandName("/localhost/nfd/fib");
  commandName.append("add-nexthop");
  commandName.append("NotReallyOptions");

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1,
         commandName, 400, "Malformed command");

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
                     bind(&FibManagerFixture::getFace, this, _1),
                     face);

  ndn::FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(1000);
  options.setCost(101);

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("add-nexthop");
  commandName.append(encodedOptions);

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1,
         commandName, 404, "Face not found");

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
                     bind(&FibManagerFixture::getFace, this, _1),
                          face);

  ndn::FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(1);
  options.setCost(101);

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("add-nexthop");
  commandName.append(encodedOptions);

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1,
         commandName, 200, "OK");

  fib.insert("/hello");

  Interest command(commandName);
  manager.onFibRequest(command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(addedNextHopWithCost(fib, "/hello", 0, 101));
}

BOOST_FIXTURE_TEST_CASE(AddNextHopVerbAddToExisting, FibManagerFixture)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(make_shared<InternalFace>());
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace, this, _1),
                          face);

  fib.insert("/hello");

  for (int i = 1; i <= 2; i++)
    {

      ndn::FibManagementOptions options;
      options.setName("/hello");
      options.setFaceId(1);
      options.setCost(100 + i);

      Block encodedOptions(options.wireEncode());

      Name commandName("/localhost/nfd/fib");
      commandName.append("add-nexthop");
      commandName.append(encodedOptions);

      face->onReceiveData +=
        bind(&FibManagerFixture::validateControlResponse, this, _1,
             commandName, 200, "OK");

      Interest command(commandName);
      manager.onFibRequest(command);
      BOOST_REQUIRE(didCallbackFire());
      resetCallbackFired();

      shared_ptr<fib::Entry> entry = fib.findExactMatch("/hello");

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

      face->onReceiveData.clear();
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

  fib.insert("/hello");

  ndn::FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(1);

  {
    options.setCost(1);

    Block encodedOptions(options.wireEncode());

    Name commandName("/localhost/nfd/fib");
    commandName.append("add-nexthop");
    commandName.append(encodedOptions);

    face->onReceiveData +=
      bind(&FibManagerFixture::validateControlResponse, this, _1,
           commandName, 200, "OK");

    Interest command(commandName);
    manager.onFibRequest(command);

    BOOST_REQUIRE(didCallbackFire());
  }

  resetCallbackFired();
  face->onReceiveData.clear();

  {
    options.setCost(102);

    Block encodedOptions(options.wireEncode());

    Name commandName("/localhost/nfd/fib");
    commandName.append("add-nexthop");
    commandName.append(encodedOptions);

    face->onReceiveData +=
      bind(&FibManagerFixture::validateControlResponse, this, _1,
           commandName, 200, "OK");

    Interest command(commandName);
    manager.onFibRequest(command);

    BOOST_REQUIRE(didCallbackFire());
  }

  shared_ptr<fib::Entry> entry = fib.findExactMatch("/hello");

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

BOOST_FIXTURE_TEST_CASE(Insert, FibManagerFixture)
{
  shared_ptr<InternalFace> face(make_shared<InternalFace>());
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace, this, _1),
                     face);

  {
    ndn::FibManagementOptions options;
    options.setName("/hello");

    Block encodedOptions(options.wireEncode());

    Name commandName("/localhost/nfd/fib");
    commandName.append("insert");
    commandName.append(encodedOptions);

    face->onReceiveData +=
      bind(&FibManagerFixture::validateControlResponse, this, _1,
           commandName, 200, "OK");

    Interest command(commandName);
    manager.onFibRequest(command);
  }

  BOOST_REQUIRE(didCallbackFire());

  shared_ptr<fib::Entry> entry = fib.findExactMatch("/hello");
  if (static_cast<bool>(entry))
    {
      const fib::NextHopList& hops = entry->getNextHops();
      BOOST_CHECK_EQUAL(hops.size(), 0);
    }

  resetCallbackFired();

  {
    ndn::FibManagementOptions options;
    options.setName("/hello");

    Block encodedOptions(options.wireEncode());

    Name commandName("/localhost/nfd/fib");
    commandName.append("insert");
    commandName.append(encodedOptions);

    face->onReceiveData +=
      bind(&FibManagerFixture::validateControlResponse, this, _1,
           commandName, 200, "OK");

    Interest command(commandName);
    manager.onFibRequest(command);
  }

  BOOST_REQUIRE(didCallbackFire());

  entry = fib.findExactMatch("/hello");
  if (static_cast<bool>(entry))
    {
      const fib::NextHopList& hops = entry->getNextHops();
      BOOST_CHECK_EQUAL(hops.size(), 0);
    }

}

void
testRemove(FibManagerFixture* fixture,
           FibManager& manager,
           Fib& fib,
           shared_ptr<Face> face,
           const Name& target)
{
  ndn::FibManagementOptions options;
  options.setName(target);

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("delete");
  commandName.append(encodedOptions);

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, fixture, _1,
         commandName, 200, "OK");

  Interest command(commandName);
  manager.onFibRequest(command);

  BOOST_REQUIRE(fixture->didCallbackFire());

  if (static_cast<bool>(fib.findExactMatch(target)))
    {
      BOOST_FAIL("Found \"removed\" prefix");
    }
  face->onReceiveData.clear();
}

BOOST_FIXTURE_TEST_CASE(Delete, FibManagerFixture)
{
  shared_ptr<InternalFace> face(make_shared<InternalFace>());
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace, this, _1),
                     face);

  fib.insert("/a");
  fib.insert("/a/b");
  fib.insert("/a/b/c");

  testRemove(this, manager, fib, face, "/");

  if (!static_cast<bool>(fib.findExactMatch("/a")) ||
      !static_cast<bool>(fib.findExactMatch("/a/b")) ||
      !static_cast<bool>(fib.findExactMatch("/a/b/c")))
    {
      BOOST_FAIL("Removed incorrect entry");
    }

  testRemove(this, manager, fib, face, "/a/b");

  if (!static_cast<bool>(fib.findExactMatch("/a")) ||
      !static_cast<bool>(fib.findExactMatch("/a/b/c")))
    {
      BOOST_FAIL("Removed incorrect entry");
    }

  testRemove(this, manager, fib, face, "/a/b/c");

  if (!static_cast<bool>(fib.findExactMatch("/a")))
    {
      BOOST_FAIL("Removed incorrect entry");
    }

  testRemove(this, manager, fib, face, "/a");

  testRemove(this, manager, fib, face, "/does/not/exist");
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
testRemoveNextHop(FibManagerFixture* fixture,
                  FibManager& manager,
                  Fib& fib,
                  shared_ptr<Face> face,
                  const Name& targetName,
                  FaceId targetFace)
{
  ndn::FibManagementOptions options;
  options.setName(targetName);
  options.setFaceId(targetFace);

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("remove-nexthop");
  commandName.append(encodedOptions);

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, fixture, _1,
         commandName, 200, "OK");

  Interest command(commandName);
  manager.onFibRequest(command);

  BOOST_REQUIRE(fixture->didCallbackFire());

  fixture->resetCallbackFired();
  face->onReceiveData.clear();
}

BOOST_FIXTURE_TEST_CASE(RemoveNextHop, FibManagerFixture)
{
  shared_ptr<Face> face1 = make_shared<DummyFace>();
  shared_ptr<Face> face2 = make_shared<DummyFace>();
  shared_ptr<Face> face3 = make_shared<DummyFace>();

  addFace(face1);
  addFace(face2);
  addFace(face3);

  shared_ptr<InternalFace> face(make_shared<InternalFace>());
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace, this, _1),
                          face);

  shared_ptr<fib::Entry> entry = fib.insert("/hello").first;

  entry->addNextHop(face1, 101);
  entry->addNextHop(face2, 202);
  entry->addNextHop(face3, 303);

  testRemoveNextHop(this, manager, fib, face, "/hello", 2);
  BOOST_REQUIRE(removedNextHopWithCost(fib, "/hello", 3, 202));

  testRemoveNextHop(this, manager, fib, face, "/hello", 3);
  BOOST_REQUIRE(removedNextHopWithCost(fib, "/hello", 2, 303));

  testRemoveNextHop(this, manager, fib, face, "/hello", 1);
  BOOST_REQUIRE(removedNextHopWithCost(fib, "/hello", 1, 101));

  if (!static_cast<bool>(fib.findExactMatch("/hello")))
    {
      BOOST_FAIL("removed entry after removing all next hops");
    }

}

BOOST_FIXTURE_TEST_CASE(RemoveNoFace, FibManagerFixture)
{
  shared_ptr<InternalFace> face(make_shared<InternalFace>());
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace, this, _1),
                          face);

  ndn::FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(1);

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("remove-nexthop");
  commandName.append(encodedOptions);

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1,
         commandName, 404, "Face not found");

  Interest command(commandName);
  manager.onFibRequest(command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_FIXTURE_TEST_CASE(RemoveNoPrefix, FibManagerFixture)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(make_shared<InternalFace>());
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace, this, _1),
                     face);

  ndn::FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(1);

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("remove-nexthop");
  commandName.append(encodedOptions);

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1,
         commandName, 404, "Prefix not found");

  Interest command(commandName);
  manager.onFibRequest(command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace nfd
