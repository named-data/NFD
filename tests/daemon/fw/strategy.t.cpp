/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2022,  Regents of the University of California,
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

#include "tests/test-common.hpp"
#include "tests/daemon/global-io-fixture.hpp"
#include "tests/daemon/face/dummy-face.hpp"
#include "dummy-strategy.hpp"

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/copy.hpp>

namespace nfd::tests {

using namespace nfd::fw;

BOOST_AUTO_TEST_SUITE(Fw)
BOOST_FIXTURE_TEST_SUITE(TestStrategy, GlobalIoFixture)

// Strategy registry is tested in strategy-choice.t.cpp and strategy-instantiation.t.cpp

class FaceTableAccessTestStrategy : public DummyStrategy
{
public:
  explicit
  FaceTableAccessTestStrategy(Forwarder& forwarder)
    : DummyStrategy(forwarder)
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
  FaceTable faceTable;
  Forwarder forwarder(faceTable);
  FaceTableAccessTestStrategy strategy(forwarder);

  auto face1 = make_shared<DummyFace>();
  auto face2 = make_shared<DummyFace>("dummy://", "dummy://", ndn::nfd::FACE_SCOPE_LOCAL);
  faceTable.add(face1);
  faceTable.add(face2);
  FaceId id1 = face1->getId();
  FaceId id2 = face2->getId();

  BOOST_TEST(strategy.getLocalFaces() == std::vector<FaceId>{id2}, boost::test_tools::per_element());

  face2->close();
  face1->close();

  BOOST_TEST((strategy.addedFaces   == std::vector<FaceId>{id1, id2}), boost::test_tools::per_element());
  BOOST_TEST((strategy.removedFaces == std::vector<FaceId>{id2, id1}), boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(ParseParameters)
{
  BOOST_TEST(Strategy::parseParameters("").empty());
  BOOST_TEST(Strategy::parseParameters("/").empty());
  BOOST_CHECK_THROW(Strategy::parseParameters("/foo"), std::invalid_argument);
  BOOST_CHECK_THROW(Strategy::parseParameters("/foo~"), std::invalid_argument);
  BOOST_CHECK_THROW(Strategy::parseParameters("/~bar"), std::invalid_argument);
  BOOST_CHECK_THROW(Strategy::parseParameters("/~"), std::invalid_argument);
  BOOST_CHECK_THROW(Strategy::parseParameters("/~~"), std::invalid_argument);

  StrategyParameters expected;
  expected["foo"] = "bar";
  BOOST_TEST(Strategy::parseParameters("/foo~bar") == expected);
  BOOST_CHECK_THROW(Strategy::parseParameters("/foo~bar/42"), std::invalid_argument);
  expected["the-answer"] = "42";
  BOOST_TEST(Strategy::parseParameters("/foo~bar/the-answer~42") == expected);
  expected["foo"] = "foo2";
  BOOST_TEST(Strategy::parseParameters("/foo~bar/the-answer~42/foo~foo2") == expected);
}

BOOST_AUTO_TEST_SUITE_END() // TestStrategy
BOOST_AUTO_TEST_SUITE_END() // Fw

} // namespace nfd::tests
