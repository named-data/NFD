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

  InternalFaceFixture()
    : m_onInterestFired(false),
      m_noOnInterestFired(false)
  {

  }

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
  validateOnInterestCallback(const Name& name, const Interest& interest)
  {
    m_onInterestFired = true;
  }

  void
  validateNoOnInterestCallback(const Name& name, const Interest& interest)
  {
    m_noOnInterestFired = true;
  }

  void
  addFace(shared_ptr<Face> face)
  {
    m_faces.push_back(face);
  }

  bool
  didOnInterestFire()
  {
    return m_onInterestFired;
  }

  bool
  didNoOnInterestFire()
  {
    return m_noOnInterestFired;
  }

  void
  resetOnInterestFired()
  {
    m_onInterestFired = false;
  }

  void
  resetNoOnInterestFired()
  {
    m_noOnInterestFired = false;
  }

private:
  std::vector<shared_ptr<Face> > m_faces;
  bool m_onInterestFired;
  bool m_noOnInterestFired;
};

BOOST_AUTO_TEST_SUITE(MgmtInternalFace)

void
validatePutData(bool& called, const Name& expectedName, const Data& data)
{
  called = true;
  BOOST_CHECK_EQUAL(expectedName, data.getName());
}

BOOST_FIXTURE_TEST_CASE(PutData, InternalFaceFixture)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(new InternalFace);
  Fib fib;
  FibManager manager(fib,
                     bind(&InternalFaceFixture::getFace,
                          this, _1),
                          face);

  bool didPutData = false;
  Name dataName("/hello");
  face->onReceiveData += bind(&validatePutData, boost::ref(didPutData), dataName, _1);

  Data testData(dataName);
  face->sign(testData);
  face->put(testData);

  BOOST_REQUIRE(didPutData);
}

BOOST_FIXTURE_TEST_CASE(SendInterestHitEnd, InternalFaceFixture)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(new InternalFace);
  Fib fib;
  FibManager manager(fib,
                     bind(&InternalFaceFixture::getFace,
                          this, _1),
                          face);

  face->setInterestFilter("/localhost/nfd/fib",
                          bind(&InternalFaceFixture::validateOnInterestCallback,
                               this, _1, _2));

  // generate command whose name is canonically
  // ordered after /localhost/nfd/fib so that
  // we hit the end of the std::map

  Name commandName("/localhost/nfd/fib/end");
  Interest command(commandName);
  face->sendInterest(command);

  BOOST_REQUIRE(didOnInterestFire());
  BOOST_REQUIRE(didNoOnInterestFire() == false);
}



BOOST_FIXTURE_TEST_CASE(SendInterestHitBegin, InternalFaceFixture)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(new InternalFace);
  Fib fib;
  FibManager manager(fib,
                     bind(&InternalFaceFixture::getFace,
                          this, _1),
                          face);

  face->setInterestFilter("/localhost/nfd/fib",
                          bind(&InternalFaceFixture::validateNoOnInterestCallback,
                               this, _1, _2));

  // generate command whose name is canonically
  // ordered before /localhost/nfd/fib so that
  // we hit the beginning of the std::map

  Name commandName("/localhost/nfd");
  Interest command(commandName);
  face->sendInterest(command);

  BOOST_REQUIRE(didNoOnInterestFire() == false);
}



BOOST_FIXTURE_TEST_CASE(SendInterestHitExact, InternalFaceFixture)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(new InternalFace);
  Fib fib;
  FibManager manager(fib,
                     bind(&InternalFaceFixture::getFace,
                          this, _1),
                          face);

  face->setInterestFilter("/localhost/nfd/eib",
                          bind(&InternalFaceFixture::validateNoOnInterestCallback,
                               this, _1, _2));

  face->setInterestFilter("/localhost/nfd/fib",
                          bind(&InternalFaceFixture::validateOnInterestCallback,
                           this, _1, _2));

  face->setInterestFilter("/localhost/nfd/gib",
                          bind(&InternalFaceFixture::validateNoOnInterestCallback,
                               this, _1, _2));

  // generate command whose name exactly matches
  // /localhost/nfd/fib

  Name commandName("/localhost/nfd/fib");
  Interest command(commandName);
  face->sendInterest(command);

  BOOST_REQUIRE(didOnInterestFire());
  BOOST_REQUIRE(didNoOnInterestFire() == false);
}



BOOST_FIXTURE_TEST_CASE(SendInterestHitPrevious, InternalFaceFixture)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(new InternalFace);
  Fib fib;
  FibManager manager(fib,
                     bind(&InternalFaceFixture::getFace,
                          this, _1),
                          face);

  face->setInterestFilter("/localhost/nfd/fib",
                          bind(&InternalFaceFixture::validateOnInterestCallback,
                               this, _1, _2));

  face->setInterestFilter("/localhost/nfd/fib/zzzzzzzzzzzzz/",
                          bind(&InternalFaceFixture::validateNoOnInterestCallback,
                               this, _1, _2));

  // generate command whose name exactly matches
  // an Interest filter

  Name commandName("/localhost/nfd/fib/previous");
  Interest command(commandName);
  face->sendInterest(command);

  BOOST_REQUIRE(didOnInterestFire());
  BOOST_REQUIRE(didNoOnInterestFire() == false);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace nfd
