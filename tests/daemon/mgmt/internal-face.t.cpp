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

#include "mgmt/internal-face.hpp"
#include "tests/daemon/face/dummy-face.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

class InternalFaceFixture : protected BaseFixture
{
public:

  InternalFaceFixture()
    : m_onInterestFired(false),
      m_noOnInterestFired(false)
  {

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

protected:
  ndn::KeyChain m_keyChain;

private:
  std::vector<shared_ptr<Face> > m_faces;
  bool m_onInterestFired;
  bool m_noOnInterestFired;
};

BOOST_FIXTURE_TEST_SUITE(MgmtInternalFace, InternalFaceFixture)

void
validatePutData(bool& called, const Name& expectedName, const Data& data)
{
  called = true;
  BOOST_CHECK_EQUAL(expectedName, data.getName());
}

BOOST_AUTO_TEST_CASE(PutData)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(new InternalFace);

  bool didPutData = false;
  Name dataName("/hello");
  face->onReceiveData.connect(bind(&validatePutData, ref(didPutData), dataName, _1));

  Data testData(dataName);
  m_keyChain.sign(testData);
  face->put(testData);

  BOOST_REQUIRE(didPutData);

  BOOST_CHECK_THROW(face->close(), InternalFace::Error);
}

BOOST_AUTO_TEST_CASE(SendInterestHitEnd)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(new InternalFace);

  face->setInterestFilter("/localhost/nfd/fib",
                          bind(&InternalFaceFixture::validateOnInterestCallback,
                               this, _1, _2));

  // generate command whose name is canonically
  // ordered after /localhost/nfd/fib so that
  // we hit the end of the std::map

  Name commandName("/localhost/nfd/fib/end");
  shared_ptr<Interest> command = makeInterest(commandName);
  face->sendInterest(*command);
  g_io.run_one();

  BOOST_REQUIRE(didOnInterestFire());
  BOOST_REQUIRE(didNoOnInterestFire() == false);
}

BOOST_AUTO_TEST_CASE(SendInterestHitBegin)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(new InternalFace);

  face->setInterestFilter("/localhost/nfd/fib",
                          bind(&InternalFaceFixture::validateNoOnInterestCallback,
                               this, _1, _2));

  // generate command whose name is canonically
  // ordered before /localhost/nfd/fib so that
  // we hit the beginning of the std::map

  Name commandName("/localhost/nfd");
  shared_ptr<Interest> command = makeInterest(commandName);
  face->sendInterest(*command);
  g_io.run_one();

  BOOST_REQUIRE(didNoOnInterestFire() == false);
}

BOOST_AUTO_TEST_CASE(SendInterestHitExact)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(new InternalFace);

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
  shared_ptr<Interest> command = makeInterest(commandName);
  face->sendInterest(*command);
  g_io.run_one();

  BOOST_REQUIRE(didOnInterestFire());
  BOOST_REQUIRE(didNoOnInterestFire() == false);
}

BOOST_AUTO_TEST_CASE(SendInterestHitPrevious)
{
  addFace(make_shared<DummyFace>());

  shared_ptr<InternalFace> face(new InternalFace);

  face->setInterestFilter("/localhost/nfd/fib",
                          bind(&InternalFaceFixture::validateOnInterestCallback,
                               this, _1, _2));

  face->setInterestFilter("/localhost/nfd/fib/zzzzzzzzzzzzz/",
                          bind(&InternalFaceFixture::validateNoOnInterestCallback,
                               this, _1, _2));

  // generate command whose name exactly matches
  // an Interest filter

  Name commandName("/localhost/nfd/fib/previous");
  shared_ptr<Interest> command = makeInterest(commandName);
  face->sendInterest(*command);
  g_io.run_one();

  BOOST_REQUIRE(didOnInterestFire());
  BOOST_REQUIRE(didNoOnInterestFire() == false);
}

BOOST_AUTO_TEST_CASE(InterestGone)
{
  shared_ptr<InternalFace> face = make_shared<InternalFace>();
  shared_ptr<Interest> interest = makeInterest("ndn:/localhost/nfd/gone");
  face->sendInterest(*interest);

  interest.reset();
  BOOST_CHECK_NO_THROW(g_io.poll());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
