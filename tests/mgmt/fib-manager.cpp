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

  shared_ptr<Face>
  getFace(FaceId id)
  {
    if (m_faces.size() < id)
      {
        BOOST_FAIL("Attempted to access invalid FaceId: " << id);
      }
    return m_faces[id-1];
  }

  void
  addFace(shared_ptr<Face> face)
  {
    m_faces.push_back(face);
  }

private:
  std::vector<shared_ptr<Face> > m_faces;
};


BOOST_AUTO_TEST_SUITE(MgmtFibManager)

void
validateControlResponse(const Data& response,
                        uint32_t expectedCode,
                        const std::string& expectedText)
{
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

BOOST_AUTO_TEST_CASE(MalformedCommmand)
{
  FibManagerFixture fixture;
  shared_ptr<InternalFace> face(new InternalFace);
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace,
                          &fixture, _1),
                          face);

  face->onReceiveData +=
    bind(&validateControlResponse, _1, 404, "MALFORMED");

  Interest command("/localhost/nfd/fib");
  manager.onFibRequest(command);
}

BOOST_AUTO_TEST_CASE(UnsupportedVerb)
{
  FibManagerFixture fixture;
  shared_ptr<InternalFace> face(new InternalFace);
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace,
                          &fixture, _1),
                          face);
  face->onReceiveData +=
    bind(&validateControlResponse, _1, 404, "UNSUPPORTED");

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
}

bool
foundNextHop(FaceId id, uint32_t cost, const fib::NextHop& next)
{
  return id == next.getFace()->getId() && next.getCost() == cost;
}

BOOST_AUTO_TEST_CASE(AddNextHopVerbInitialAdd)
{
  FibManagerFixture fixture;
  fixture.addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(new InternalFace);
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace,
                          &fixture, _1),
                          face);
  face->onReceiveData +=
    bind(&validateControlResponse, _1, 200, "OK");



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

  shared_ptr<fib::Entry> entry = fib.findLongestPrefixMatch("/hello");

  if (entry)
    {
      const fib::NextHopList& hops = entry->getNextHops();
      NFD_LOG_DEBUG("FaceId: " << hops[0].getFace()->getId());
      BOOST_REQUIRE(hops.size() == 1);
      BOOST_REQUIRE(std::find_if(hops.begin(),
                                 hops.end(),
                                 bind(&foundNextHop, -1, 101, _1)) != hops.end());
    }
  else
    {
      BOOST_FAIL("Failed to find expected fib entry");
    }
}

BOOST_AUTO_TEST_CASE(AddNextHopVerbAddToExisting)
{
  FibManagerFixture fixture;
  fixture.addFace(make_shared<DummyFace>());
  fixture.addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(new InternalFace);
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace,
                          &fixture, _1),
                          face);
  face->onReceiveData +=
    bind(&validateControlResponse, _1, 200, "OK");

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

      shared_ptr<fib::Entry> entry = fib.findLongestPrefixMatch("/hello");

      if (entry)
        {
          const fib::NextHopList& hops = entry->getNextHops();
          for (int j = 1; j <= i; j++)
            {
              BOOST_REQUIRE(hops.size() == i);
              // BOOST_REQUIRE(hops[j-1].getCost() == j);
              BOOST_REQUIRE(std::find_if(hops.begin(),
                                         hops.end(),
                                         bind(&foundNextHop, -1, 100 + j, _1)) != hops.end());
            }
        }
      else
        {
          BOOST_FAIL("Failed to find expected fib entry");
        }
    }
}

BOOST_AUTO_TEST_CASE(AddNextHopVerbUpdateFaceCost)
{
  FibManagerFixture fixture;
  fixture.addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(new InternalFace);
  Fib fib;
  FibManager manager(fib,
                     bind(&FibManagerFixture::getFace,
                          &fixture, _1),
                          face);
  face->onReceiveData +=
    bind(&validateControlResponse, _1, 200, "OK");

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
  }

  {
    options.setCost(102);

    Block encodedOptions(options.wireEncode());

    Name commandName("/localhost/nfd/fib");
    commandName.append("add-nexthop");
    commandName.append(encodedOptions);

    Interest command(commandName);
    manager.onFibRequest(command);
  }

  shared_ptr<fib::Entry> entry = fib.findLongestPrefixMatch("/hello");

  // Add faces with cost == FaceID for the name /hello
  // This test assumes:
  //   FaceIDs are -1 because we don't add them to a forwarder
  if (entry)
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
