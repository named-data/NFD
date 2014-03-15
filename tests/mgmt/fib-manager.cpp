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
#include "tests/face/dummy-face.hpp"

#include "validation-common.hpp"
#include "tests/test-common.hpp"

#include "fib-enumeration-publisher-common.hpp"

namespace nfd {
namespace tests {

NFD_LOG_INIT("FibManagerTest");

class FibManagerFixture : protected BaseFixture, public FibEnumerationPublisherFixture
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
    BOOST_REQUIRE(control.getBody().value_size() == expectedBody.value_size());

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
      : m_manager(boost::ref(m_fib),
                  bind(&FibManagerFixture::getFace, this, _1),
                  m_face)
    , m_callbackFired(false)
  {
  }

protected:
  FibManager m_manager;

  std::vector<shared_ptr<Face> > m_faces;
  bool m_callbackFired;
};

template <typename T> class AuthorizedCommandFixture:
    public CommandFixture<T>
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

BOOST_AUTO_TEST_CASE(TestFireInterestFilter)
{
  shared_ptr<InternalFace> face = getInternalFace();

  Interest command("/localhost/nfd/fib");

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this,  _1,
         command.getName(), 400, "Malformed command");

  face->sendInterest(command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_AUTO_TEST_CASE(MalformedCommmand)
{
  shared_ptr<InternalFace> face = getInternalFace();

  BOOST_REQUIRE(didCallbackFire() == false);

  Interest command("/localhost/nfd/fib");

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1,
         command.getName(), 400, "Malformed command");

  getFibManager().onFibRequest(command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_AUTO_TEST_CASE(UnsupportedVerb)
{
  shared_ptr<InternalFace> face = getInternalFace();

  FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(1);
  options.setCost(1);

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("unsupported");
  commandName.append(encodedOptions);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1,
         command->getName(), 501, "Unsupported command");

  getFibManager().onFibRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_AUTO_TEST_CASE(UnsignedCommand)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face = getInternalFace();

  FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(1);
  options.setCost(101);

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("add-nexthop");
  commandName.append(encodedOptions);

  Interest command(commandName);

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse,
         this, _1, command.getName(), 401, "Signature required");


  getFibManager().onFibRequest(command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(!addedNextHopWithCost(getFib(), "/hello", 0, 101));
}

BOOST_FIXTURE_TEST_CASE(UnauthorizedCommand, UnauthorizedCommandFixture<FibManagerFixture>)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face = getInternalFace();

  FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(1);
  options.setCost(101);

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("add-nexthop");
  commandName.append(encodedOptions);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse,
         this, _1, command->getName(), 403, "Unauthorized command");

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
  commandName.append("NotReallyOptions");

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1,
         command->getName(), 400, "Malformed command");

  getFibManager().onFibRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_AUTO_TEST_CASE(UnknownFaceId)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face = getInternalFace();

  FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(1000);
  options.setCost(101);

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("add-nexthop");
  commandName.append(encodedOptions);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1,
         command->getName(), 404, "Face not found");

  getFibManager().onFibRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(addedNextHopWithCost(getFib(), "/hello", 0, 101) == false);
}

BOOST_AUTO_TEST_CASE(TestImplicitFaceId)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face = getInternalFace();

  FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(0);
  options.setCost(101);

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("add-nexthop");
  commandName.append(encodedOptions);

  FibManagementOptions expectedOptions;
  expectedOptions.setName("/hello");
  expectedOptions.setFaceId(1);
  expectedOptions.setCost(101);

  Block encodedExpectedOptions(expectedOptions.wireEncode());

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  command->setIncomingFaceId(1);
  generateCommand(*command);

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1,
         command->getName(), 200, "Success", encodedExpectedOptions);

  getFibManager().onFibRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(addedNextHopWithFace(getFib(), "/hello", 0, 101, getFace(1)));
}

BOOST_AUTO_TEST_CASE(AddNextHopVerbInitialAdd)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face = getInternalFace();

  FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(1);
  options.setCost(101);

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("add-nexthop");
  commandName.append(encodedOptions);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1,
         command->getName(), 200, "Success", encodedOptions);

  getFibManager().onFibRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
  BOOST_REQUIRE(addedNextHopWithCost(getFib(), "/hello", 0, 101));
}

BOOST_AUTO_TEST_CASE(AddNextHopVerbAddToExisting)
{
  addFace(make_shared<DummyFace>());
  shared_ptr<InternalFace> face = getInternalFace();

  for (int i = 1; i <= 2; i++)
    {

      FibManagementOptions options;
      options.setName("/hello");
      options.setFaceId(1);
      options.setCost(100 + i);

      Block encodedOptions(options.wireEncode());

      Name commandName("/localhost/nfd/fib");
      commandName.append("add-nexthop");
      commandName.append(encodedOptions);

      shared_ptr<Interest> command(make_shared<Interest>(commandName));
      generateCommand(*command);

      face->onReceiveData +=
        bind(&FibManagerFixture::validateControlResponse, this, _1,
             command->getName(), 200, "Success", encodedOptions);

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

      face->onReceiveData.clear();
    }
}

BOOST_AUTO_TEST_CASE(AddNextHopVerbUpdateFaceCost)
{
  addFace(make_shared<DummyFace>());
  shared_ptr<InternalFace> face = getInternalFace();

  FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(1);

  {
    options.setCost(1);

    Block encodedOptions(options.wireEncode());

    Name commandName("/localhost/nfd/fib");
    commandName.append("add-nexthop");
    commandName.append(encodedOptions);

    shared_ptr<Interest> command(make_shared<Interest>(commandName));
    generateCommand(*command);

    face->onReceiveData +=
      bind(&FibManagerFixture::validateControlResponse, this, _1,
           command->getName(), 200, "Success", encodedOptions);

    getFibManager().onFibRequest(*command);

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

    shared_ptr<Interest> command(make_shared<Interest>(commandName));
    generateCommand(*command);

    face->onReceiveData +=
      bind(&FibManagerFixture::validateControlResponse, this, _1,
           command->getName(), 200, "Success", encodedOptions);

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

// BOOST_AUTO_TEST_CASE(Insert)
// {
//   addFace(make_shared<DummyFace>());
//   addFace(make_shared<DummyFace>());
//   shared_ptr<InternalFace> face = getInternalFace();

//   {
//     FibManagementOptions options;
//     options.setName("/hello");
//     options.setFaceId(1);
//     options.setCost(101);

//     Block encodedOptions(options.wireEncode());

//     Name commandName("/localhost/nfd/fib");
//     commandName.append("add-nexthop");
//     commandName.append(encodedOptions);

//     shared_ptr<Interest> command(make_shared<Interest>(commandName));
//     generateCommand(*command);

//     face->onReceiveData +=
//       bind(&FibManagerFixture::validateControlResponse, this, _1,
//            command->getName(), 200, "Success", encodedOptions);

//     getFibManager().onFibRequest(*command);
//   }

//   BOOST_REQUIRE(didCallbackFire());

//   shared_ptr<fib::Entry> entry = getFib().findExactMatch("/hello");
//   if (static_cast<bool>(entry))
//     {
//       const fib::NextHopList& hops = entry->getNextHops();
//       BOOST_CHECK_EQUAL(hops.size(), 1);
//     }

//   resetCallbackFired();
//   face->onReceiveData.clear();

//   {
//     FibManagementOptions options;
//     options.setName("/hello");
//     options.setFaceId(2);
//     options.setCost(102);

//     Block encodedOptions(options.wireEncode());

//     Name commandName("/localhost/nfd/fib");
//     commandName.append("add-nexthop");
//     commandName.append(encodedOptions);

//     shared_ptr<Interest> command(make_shared<Interest>(commandName));
//     generateCommand(*command);

//     face->onReceiveData +=
//       bind(&FibManagerFixture::validateControlResponse, this, _1,
//            command->getName(), 200, "Success", encodedOptions);

//     getFibManager().onFibRequest(*command);
//   }

//   BOOST_REQUIRE(didCallbackFire());

//   entry = getFib().findExactMatch("/hello");
//   if (static_cast<bool>(entry))
//     {
//       const fib::NextHopList& hops = entry->getNextHops();
//       BOOST_CHECK_EQUAL(hops.size(), 2);
//     }

// }

// void
// testRemove(CommandFixture<FibManagerFixture>* fixture,
//            FibManager& manager,
//            Fib& fib,
//            shared_ptr<Face> face,
//            const Name& target)
// {
//   FibManagementOptions options;
//   options.setName(target);

//   Block encodedOptions(options.wireEncode());

//   Name commandName("/localhost/nfd/fib");
//   commandName.append("delete");
//   commandName.append(encodedOptions);

//   shared_ptr<Interest> command(make_shared<Interest>(commandName));
//   fixture->generateCommand(*command);

//   face->onReceiveData +=
//     bind(&FibManagerFixture::validateControlResponse, fixture, _1,
//          command->getName(), 200, "Success", encodedOptions);

//   manager.onFibRequest(*command);

//   BOOST_REQUIRE(fixture->didCallbackFire());

//   if (static_cast<bool>(fib.findExactMatch(target)))
//     {
//       BOOST_FAIL("Found \"removed\" prefix");
//     }
//   face->onReceiveData.clear();
// }

// BOOST_AUTO_TEST_CASE(Delete)
// {
//   shared_ptr<InternalFace> face = getInternalFace();
//   FibManager& manager = getFibManager();
//   Fib& fib = getFib();

//   fib.insert("/a");
//   fib.insert("/a/b");
//   fib.insert("/a/b/c");

//   testRemove(this, manager, fib, face, "/");

//   if (!static_cast<bool>(fib.findExactMatch("/a")) ||
//       !static_cast<bool>(fib.findExactMatch("/a/b")) ||
//       !static_cast<bool>(fib.findExactMatch("/a/b/c")))
//     {
//       BOOST_FAIL("Removed incorrect entry");
//     }

//   testRemove(this, manager, fib, face, "/a/b");

//   if (!static_cast<bool>(fib.findExactMatch("/a")) ||
//       !static_cast<bool>(fib.findExactMatch("/a/b/c")))
//     {
//       BOOST_FAIL("Removed incorrect entry");
//     }

//   testRemove(this, manager, fib, face, "/a/b/c");

//   if (!static_cast<bool>(fib.findExactMatch("/a")))
//     {
//       BOOST_FAIL("Removed incorrect entry");
//     }

//   testRemove(this, manager, fib, face, "/a");

//   testRemove(this, manager, fib, face, "/does/not/exist");
// }

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
  FibManagementOptions options;
  options.setName(targetName);
  options.setFaceId(targetFace);

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("remove-nexthop");
  commandName.append(encodedOptions);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  fixture->generateCommand(*command);

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, fixture, _1,
         command->getName(), 200, "Success", encodedOptions);

  manager.onFibRequest(*command);

  BOOST_REQUIRE(fixture->didCallbackFire());

  fixture->resetCallbackFired();
  face->onReceiveData.clear();
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

BOOST_AUTO_TEST_CASE(RemoveNoFace)
{
  shared_ptr<InternalFace> face = getInternalFace();

  FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(1);

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("remove-nexthop");
  commandName.append(encodedOptions);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1,
         command->getName(), 404, "Face not found");

  getFibManager().onFibRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
}

BOOST_AUTO_TEST_CASE(RemoveNoPrefix)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face = getInternalFace();

  FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(1);

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("remove-nexthop");
  commandName.append(encodedOptions);

  shared_ptr<Interest> command(make_shared<Interest>(commandName));
  generateCommand(*command);

  face->onReceiveData +=
    bind(&FibManagerFixture::validateControlResponse, this, _1,
         command->getName(), 404, "Prefix not found");

  getFibManager().onFibRequest(*command);

  BOOST_REQUIRE(didCallbackFire());
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

  m_face->onReceiveData +=
    bind(&FibEnumerationPublisherFixture::decodeFibEntryBlock, this, _1);

  shared_ptr<Interest> command(make_shared<Interest>("/localhost/nfd/fib/list"));

  m_manager.onFibRequest(*command);
  BOOST_REQUIRE(m_finished);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
