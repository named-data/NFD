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

#include "fw/strategy.hpp"
#include "dummy-strategy.hpp"
#include "tests/daemon/face/dummy-face.hpp"
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/copy.hpp>

#include "tests/test-common.hpp"

namespace nfd {
namespace fw {
namespace tests {

using namespace nfd::tests;

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_FIXTURE_TEST_SUITE(TestStrategy, BaseFixture)

class FaceTableAccessTestStrategy : public DummyStrategy
{
public:
  explicit
  FaceTableAccessTestStrategy(Forwarder& forwarder)
    : DummyStrategy(forwarder, Name("ndn:/strategy"))
  {
    this->afterAddFace.connect([this] (const Face& face) {
      this->addedFaces.push_back(face.getId());
    });
    this->beforeRemoveFace.connect([this] (const Face& face) {
      this->removedFaces.push_back(face.getId());
    });
  }

  std::vector<FaceId>
  getLocalFaces()
  {
    auto enumerable = this->getFaceTable() |
                      boost::adaptors::filtered([] (Face& face) {
                        return face.getScope() == ndn::nfd::FACE_SCOPE_LOCAL;
                      }) |
                      boost::adaptors::transformed([] (Face& face) {
                        return face.getId();
                      });

    std::vector<FaceId> results;
    boost::copy(enumerable, std::back_inserter(results));
    return results;
  }

public:
  std::vector<FaceId> addedFaces;
  std::vector<FaceId> removedFaces;
};

BOOST_AUTO_TEST_CASE(FaceTableAccess)
{
  Forwarder forwarder;
  FaceTableAccessTestStrategy strategy(forwarder);

  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>("dummy://", "dummy://", ndn::nfd::FACE_SCOPE_LOCAL);
  forwarder.addFace(face1);
  forwarder.addFace(face2);
  FaceId id1 = face1->getId();
  FaceId id2 = face2->getId();

  BOOST_CHECK(strategy.getLocalFaces() == std::vector<FaceId>{id2});

  face2->close();
  face1->close();

  BOOST_CHECK((strategy.addedFaces   == std::vector<FaceId>{id1, id2}));
  BOOST_CHECK((strategy.removedFaces == std::vector<FaceId>{id2, id1}));
}

class LookupFibFixture : public BaseFixture
{
protected:
  class TestStrategy : public DummyStrategy
  {
  public:
    explicit
    TestStrategy(Forwarder& forwarder)
      : DummyStrategy(forwarder, Name("ndn:/strategy"))
    {
    }

    const fib::Entry&
    lookupFib(const pit::Entry& pitEntry) const
    {
      return this->Strategy::lookupFib(pitEntry);
    }
  };

  LookupFibFixture()
    : strategy(forwarder)
    , fib(forwarder.getFib())
    , pit(forwarder.getPit())
    , nrt(forwarder.getNetworkRegionTable())
    , inFace(make_shared<DummyFace>())
    , outFace(make_shared<DummyFace>())
    , link(makeLink("/net/ndnsim", {{10, "/telia/terabits"}, {20, "/ucla/cs"}}))
  {
    forwarder.addFace(inFace);
    forwarder.addFace(outFace);
  }

  const fib::Entry&
  lookupFib(Interest& interest)
  {
    shared_ptr<pit::Entry> pitEntry = pit.insert(interest).first;
    pitEntry->insertOrUpdateInRecord(*inFace, interest);
    return strategy.lookupFib(*pitEntry);
  }

protected:
  Forwarder forwarder;
  TestStrategy strategy;
  Fib& fib;
  Pit& pit;
  NetworkRegionTable& nrt;

  shared_ptr<Face> inFace;
  shared_ptr<Face> outFace;
  shared_ptr<Link> link;
};

BOOST_FIXTURE_TEST_SUITE(LookupFib, LookupFibFixture)

BOOST_AUTO_TEST_CASE(NoLink)
{
  fib.insert("/net/ndnsim").first->addNextHop(*outFace, 10);

  auto interest = makeInterest("/net/ndnsim/www/index.html");
  const fib::Entry& fibEntry = this->lookupFib(*interest);

  BOOST_CHECK_EQUAL(fibEntry.getPrefix(), "/net/ndnsim");
  BOOST_CHECK_EQUAL(interest->hasSelectedDelegation(), false);
}

BOOST_AUTO_TEST_CASE(ConsumerRegion)
{
  nrt.insert("/arizona/cs/avenir");
  fib.insert("/").first->addNextHop(*outFace, 10);

  auto interest = makeInterest("/net/ndnsim/www/index.html");
  interest->setLink(link->wireEncode());
  const fib::Entry& fibEntry = this->lookupFib(*interest);

  BOOST_CHECK_EQUAL(fibEntry.getPrefix(), "/");
  BOOST_CHECK_EQUAL(interest->hasSelectedDelegation(), false);
}

BOOST_AUTO_TEST_CASE(DefaultFreeFirstDelegation)
{
  nrt.insert("/arizona/cs/hobo");
  fib.insert("/telia").first->addNextHop(*outFace, 20);
  fib.insert("/ucla").first->addNextHop(*outFace, 10);

  auto interest = makeInterest("/net/ndnsim/www/index.html");
  interest->setLink(link->wireEncode());
  const fib::Entry& fibEntry = this->lookupFib(*interest);

  BOOST_CHECK_EQUAL(fibEntry.getPrefix(), "/telia");
  BOOST_REQUIRE_EQUAL(interest->hasSelectedDelegation(), true);
  BOOST_CHECK_EQUAL(interest->getSelectedDelegation(), "/telia/terabits");
}

BOOST_AUTO_TEST_CASE(DefaultFreeSecondDelegation)
{
  nrt.insert("/arizona/cs/hobo");
  fib.insert("/ucla").first->addNextHop(*outFace, 10);

  auto interest = makeInterest("/net/ndnsim/www/index.html");
  interest->setLink(link->wireEncode());
  const fib::Entry& fibEntry = this->lookupFib(*interest);

  BOOST_CHECK_EQUAL(fibEntry.getPrefix(), "/ucla");
  BOOST_REQUIRE_EQUAL(interest->hasSelectedDelegation(), true);
  BOOST_CHECK_EQUAL(interest->getSelectedDelegation(), "/ucla/cs");
}

BOOST_AUTO_TEST_CASE(DefaultFreeHasSelectedDelegation)
{
  nrt.insert("/ucsd/caida/click");
  fib.insert("/telia").first->addNextHop(*outFace, 10);
  fib.insert("/ucla").first->addNextHop(*outFace, 10);

  auto interest = makeInterest("/net/ndnsim/www/index.html");
  interest->setLink(link->wireEncode());
  interest->setSelectedDelegation("/ucla/cs");
  const fib::Entry& fibEntry = this->lookupFib(*interest);

  BOOST_CHECK_EQUAL(fibEntry.getPrefix(), "/ucla");
  BOOST_REQUIRE_EQUAL(interest->hasSelectedDelegation(), true);
  BOOST_CHECK_EQUAL(interest->getSelectedDelegation(), "/ucla/cs");
}

BOOST_AUTO_TEST_CASE(ProducerRegion)
{
  nrt.insert("/ucla/cs/spurs");
  fib.insert("/").first->addNextHop(*outFace, 10);
  fib.insert("/ucla").first->addNextHop(*outFace, 10);
  fib.insert("/net/ndnsim").first->addNextHop(*outFace, 10);

  auto interest = makeInterest("/net/ndnsim/www/index.html");
  interest->setLink(link->wireEncode());
  interest->setSelectedDelegation("/ucla/cs");
  const fib::Entry& fibEntry = this->lookupFib(*interest);

  BOOST_CHECK_EQUAL(fibEntry.getPrefix(), "/net/ndnsim");
  BOOST_REQUIRE_EQUAL(interest->hasSelectedDelegation(), true);
  BOOST_CHECK_EQUAL(interest->getSelectedDelegation(), "/ucla/cs");
}

BOOST_AUTO_TEST_SUITE_END() // LookupFib

BOOST_AUTO_TEST_SUITE_END() // TestStrategy
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace fw
} // namespace nfd
