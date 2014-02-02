/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "mgmt/internal-face.hpp"
#include "mgmt/fib-manager.hpp"
#include "table/fib.hpp"
#include "../face/dummy-face.hpp"

#include <ndn-cpp-dev/management/fib-management-options.hpp>
#include <ndn-cpp-dev/management/control-response.hpp>
#include <ndn-cpp-dev/encoding/block.hpp>

#include <boost/test/unit_test.hpp>

namespace nfd {

NFD_LOG_INIT("InternalFaceTest");

class InternalFaceFixture
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

BOOST_AUTO_TEST_SUITE(MgmtInternalFace)

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

BOOST_AUTO_TEST_CASE(ValidAddNextHop)
{
  InternalFaceFixture fixture;
  fixture.addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(new InternalFace);
  Fib fib;
  FibManager manager(fib,
                     bind(&InternalFaceFixture::getFace,
                          &fixture, _1),
                          face);

  face->onReceiveData +=
    bind(&validateControlResponse, _1, 200, "OK");

  ndn::FibManagementOptions options;
  options.setName("/hello");
  options.setFaceId(1);
  options.setCost(1);

  Block encodedOptions(options.wireEncode());

  Name commandName("/localhost/nfd/fib");
  commandName.append("add-nexthop");
  commandName.append(encodedOptions);

  Interest command(commandName);
  face->sendInterest(command);

  shared_ptr<fib::Entry> entry = fib.findLongestPrefixMatch("/hello");

  if (entry)
    {
      const fib::NextHopList& hops = entry->getNextHops();
      BOOST_REQUIRE(hops.size() == 1);
      BOOST_CHECK(hops[0].getCost() == 1);
    }
  else
    {
      BOOST_FAIL("Failed to find expected fib entry");
    }
}

BOOST_AUTO_TEST_CASE(InvalidPrefixRegistration)
{
  InternalFaceFixture fixture;
  fixture.addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(new InternalFace);
  Fib fib;
  FibManager manager(fib,
                     bind(&InternalFaceFixture::getFace,
                          &fixture, _1),
                     face);

  face->onReceiveData +=
    bind(&validateControlResponse, _1, 404, "MALFORMED");

  Interest nonRegInterest("/hello");
  face->sendInterest(nonRegInterest);

  shared_ptr<fib::Entry> entry = fib.findLongestPrefixMatch("/hello");

  if (entry)
    {
      const fib::NextHopList& hops = entry->getNextHops();
      BOOST_REQUIRE(hops.size() == 0);
    }
}

void
validateOnInterestCallback(const Name& name, const Interest& interest)
{
  NFD_LOG_DEBUG("Reached correct callback");
}

void
validateNoOnInterestCallback(const Name& name, const Interest& interest)
{
  BOOST_FAIL("Reached wrong callback");
}

BOOST_AUTO_TEST_CASE(SendInterestHitEnd)
{
  InternalFaceFixture fixture;
  fixture.addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(new InternalFace);
  Fib fib;
  FibManager manager(fib,
                     bind(&InternalFaceFixture::getFace,
                          &fixture, _1),
                          face);

  face->setInterestFilter("/localhost/nfd/fib",
                          &validateOnInterestCallback);

  // generate command whose name is canonically
  // ordered after /localhost/nfd/fib so that
  // we hit the end of the std::map

  Name commandName("/localhost/nfd/fib/end");
  Interest command(commandName);
  face->sendInterest(command);
}



BOOST_AUTO_TEST_CASE(SendInterestHitBegin)
{
  InternalFaceFixture fixture;
  fixture.addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(new InternalFace);
  Fib fib;
  FibManager manager(fib,
                     bind(&InternalFaceFixture::getFace,
                          &fixture, _1),
                          face);

  face->setInterestFilter("/localhost/nfd/fib",
                          &validateNoOnInterestCallback);

  // generate command whose name is canonically
  // ordered before /localhost/nfd/fib so that
  // we hit the beginning of the std::map

  Name commandName("/localhost/nfd");
  Interest command(commandName);
  face->sendInterest(command);
}



BOOST_AUTO_TEST_CASE(SendInterestHitExact)
{
  InternalFaceFixture fixture;
  fixture.addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(new InternalFace);
  Fib fib;
  FibManager manager(fib,
                     bind(&InternalFaceFixture::getFace,
                          &fixture, _1),
                          face);

  face->setInterestFilter("/localhost/nfd/eib",
                          &validateNoOnInterestCallback);

  face->setInterestFilter("/localhost/nfd/fib",
                          &validateOnInterestCallback);

  face->setInterestFilter("/localhost/nfd/gib",
                          &validateNoOnInterestCallback);

  // generate command whose name exactly matches
  // /localhost/nfd/fib

  Name commandName("/localhost/nfd/fib");
  Interest command(commandName);
  face->sendInterest(command);
}



BOOST_AUTO_TEST_CASE(SendInterestHitPrevious)
{
  InternalFaceFixture fixture;
  fixture.addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(new InternalFace);
  Fib fib;
  FibManager manager(fib,
                     bind(&InternalFaceFixture::getFace,
                          &fixture, _1),
                          face);

  face->setInterestFilter("/localhost/nfd/fib",
                          &validateOnInterestCallback);

  face->setInterestFilter("/localhost/nfd/fib/zzzzzzzzzzzzz/",
                          &validateNoOnInterestCallback);

  // generate command whose name exactly matches
  // an Interest filter

  Name commandName("/localhost/nfd/fib/previous");
  Interest command(commandName);
  face->sendInterest(command);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace nfd
