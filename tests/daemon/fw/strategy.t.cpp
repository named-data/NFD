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
  {
  }

protected:
  Forwarder forwarder;
  TestStrategy strategy;
  Fib& fib;
  Pit& pit;
  NetworkRegionTable& nrt;
};

BOOST_FIXTURE_TEST_CASE(LookupFib, LookupFibFixture)
{
  /// \todo test lookupFib without Link
  /// \todo split each step to a separate test case

  shared_ptr<Face> face1 = make_shared<DummyFace>();
  shared_ptr<Face> face2 = make_shared<DummyFace>();
  forwarder.addFace(face1);
  forwarder.addFace(face2);

  shared_ptr<Link> link = makeLink("/net/ndnsim", {{10, "/telia/terabits"}, {20, "/ucla/cs"}});

  // consumer region
  nrt.clear();
  nrt.insert("/arizona/cs/avenir");
  fib.insert("/").first->addNextHop(*face2, 10);

  auto interest1 = makeInterest("/net/ndnsim/www/1.html");
  interest1->setLink(link->wireEncode());
  shared_ptr<pit::Entry> pit1 = pit.insert(*interest1).first;
  pit1->insertOrUpdateInRecord(*face1, *interest1);

  BOOST_CHECK_EQUAL(strategy.lookupFib(*pit1).getPrefix(), "/");
  BOOST_CHECK_EQUAL(interest1->hasSelectedDelegation(), false);

  fib.insert("/").first->removeNextHop(*face2);

  // first default-free router, both delegations are available
  nrt.clear();
  nrt.insert("/arizona/cs/hobo");
  fib.insert("/telia").first->addNextHop(*face2, 10);
  fib.insert("/ucla").first->addNextHop(*face2, 10);

  auto interest2 = makeInterest("/net/ndnsim/www/2.html");
  interest2->setLink(link->wireEncode());
  shared_ptr<pit::Entry> pit2 = pit.insert(*interest2).first;
  pit2->insertOrUpdateInRecord(*face1, *interest2);

  BOOST_CHECK_EQUAL(strategy.lookupFib(*pit2).getPrefix(), "/telia");
  BOOST_REQUIRE_EQUAL(interest2->hasSelectedDelegation(), true);
  BOOST_CHECK_EQUAL(interest2->getSelectedDelegation(), "/telia/terabits");

  fib.erase("/telia");
  fib.erase("/ucla");

  // first default-free router, only second delegation is available
  nrt.clear();
  nrt.insert("/arizona/cs/hobo");
  fib.insert("/ucla").first->addNextHop(*face2, 10);

  auto interest3 = makeInterest("/net/ndnsim/www/3.html");
  interest3->setLink(link->wireEncode());
  shared_ptr<pit::Entry> pit3 = pit.insert(*interest3).first;
  pit3->insertOrUpdateInRecord(*face1, *interest3);

  BOOST_CHECK_EQUAL(strategy.lookupFib(*pit3).getPrefix(), "/ucla");
  BOOST_REQUIRE_EQUAL(interest3->hasSelectedDelegation(), true);
  BOOST_CHECK_EQUAL(interest3->getSelectedDelegation(), "/ucla/cs");

  fib.erase("/ucla");

  // default-free router, chosen SelectedDelegation
  nrt.clear();
  nrt.insert("/ucsd/caida/click");
  fib.insert("/telia").first->addNextHop(*face2, 10);
  fib.insert("/ucla").first->addNextHop(*face2, 10);

  auto interest4 = makeInterest("/net/ndnsim/www/4.html");
  interest4->setLink(link->wireEncode());
  interest4->setSelectedDelegation("/ucla/cs");
  shared_ptr<pit::Entry> pit4 = pit.insert(*interest4).first;
  pit4->insertOrUpdateInRecord(*face1, *interest4);

  BOOST_CHECK_EQUAL(strategy.lookupFib(*pit4).getPrefix(), "/ucla");
  BOOST_REQUIRE_EQUAL(interest4->hasSelectedDelegation(), true);
  BOOST_CHECK_EQUAL(interest4->getSelectedDelegation(), "/ucla/cs");

  fib.erase("/telia");
  fib.erase("/ucla");

  // producer region
  nrt.clear();
  nrt.insert("/ucla/cs/spurs");
  fib.insert("/").first->addNextHop(*face2, 10);
  fib.insert("/ucla").first->addNextHop(*face2, 10);
  fib.insert("/net/ndnsim").first->addNextHop(*face2, 10);

  auto interest5 = makeInterest("/net/ndnsim/www/5.html");
  interest5->setLink(link->wireEncode());
  interest5->setSelectedDelegation("/ucla/cs");
  shared_ptr<pit::Entry> pit5 = pit.insert(*interest5).first;
  pit5->insertOrUpdateInRecord(*face1, *interest5);

  BOOST_CHECK_EQUAL(strategy.lookupFib(*pit1).getPrefix(), "/net/ndnsim");
  BOOST_REQUIRE_EQUAL(interest5->hasSelectedDelegation(), true);
  BOOST_CHECK_EQUAL(interest5->getSelectedDelegation(), "/ucla/cs");

  fib.insert("/").first->removeNextHop(*face2);
  fib.erase("/ucla");
  fib.erase("/ndnsim");
}

BOOST_AUTO_TEST_SUITE_END() // TestStrategy
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace tests
} // namespace fw
} // namespace nfd
