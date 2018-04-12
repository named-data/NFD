/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#include "mgmt/fib-manager.hpp"
#include "table/fib-nexthop.hpp"

#include "nfd-manager-common-fixture.hpp"
#include "../face/dummy-face.hpp"

#include <ndn-cxx/lp/tags.hpp>
#include <ndn-cxx/mgmt/nfd/fib-entry.hpp>

namespace nfd {
namespace tests {

class FibManagerFixture : public NfdManagerCommonFixture
{
public:
  FibManagerFixture()
    : m_fib(m_forwarder.getFib())
    , m_faceTable(m_forwarder.getFaceTable())
    , m_manager(m_fib, m_faceTable, m_dispatcher, *m_authenticator)
  {
    setTopPrefix();
    setPrivilege("fib");
  }

public: // for test
  static ControlParameters
  makeParameters(const Name& name, const FaceId& id)
  {
    return ControlParameters().setName(name).setFaceId(id);
  }

  static ControlParameters
  makeParameters(const Name& name, const FaceId& id, const uint32_t& cost)
  {
    return ControlParameters().setName(name).setFaceId(id).setCost(cost);
  }

  FaceId
  addFace()
  {
    auto face = make_shared<DummyFace>();
    m_faceTable.add(face);
    advanceClocks(time::milliseconds(1), 10);
    m_responses.clear(); // clear all event notifications, if any
    return face->getId();
  }

public: // for check
  enum class CheckNextHopResult
  {
    OK,
    NO_FIB_ENTRY,
    WRONG_N_NEXTHOPS,
    NO_NEXTHOP,
    WRONG_COST
  };

  /**
   * @brief check whether the nexthop record is added / removed properly
   *
   * @param expectedNNextHops use nullopt to skip this check
   * @param faceId use nullopt to skip NextHopRecord checks
   * @param expectedCost use nullopt to skip this check
   *
   * @retval OK FIB entry is found by exact match and has the expected number of nexthops;
   *            NextHopRe record for faceId is found and has the expected cost
   * @retval NO_FIB_ENTRY FIB entry is not found
   * @retval WRONG_N_NEXTHOPS FIB entry is found but has wrong number of nexthops
   * @retval NO_NEXTHOP NextHopRecord for faceId is not found
   * @retval WRONG_COST NextHopRecord for faceId has wrong cost
   */
  CheckNextHopResult
  checkNextHop(const Name& prefix,
               optional<size_t> expectedNNextHops = nullopt,
               optional<FaceId> faceId = nullopt,
               optional<uint64_t> expectedCost = nullopt) const
  {
    const fib::Entry* entry = m_fib.findExactMatch(prefix);
    if (entry == nullptr) {
      return CheckNextHopResult::NO_FIB_ENTRY;
    }

    const auto& nextHops = entry->getNextHops();
    if (expectedNNextHops && nextHops.size() != *expectedNNextHops) {
      return CheckNextHopResult::WRONG_N_NEXTHOPS;
    }

    if (faceId) {
      for (const auto& record : nextHops) {
        if (record.getFace().getId() == *faceId) {
          if (expectedCost && record.getCost() != *expectedCost)
            return CheckNextHopResult::WRONG_COST;
          else
            return CheckNextHopResult::OK;
        }
      }
      return CheckNextHopResult::NO_NEXTHOP;
    }
    return CheckNextHopResult::OK;
  }

protected:
  Fib&       m_fib;
  FaceTable& m_faceTable;
  FibManager m_manager;
};

std::ostream&
operator<<(std::ostream& os, FibManagerFixture::CheckNextHopResult result)
{
  switch (result) {
  case FibManagerFixture::CheckNextHopResult::OK:
    return os << "OK";
  case FibManagerFixture::CheckNextHopResult::NO_FIB_ENTRY:
    return os << "NO_FIB_ENTRY";
  case FibManagerFixture::CheckNextHopResult::WRONG_N_NEXTHOPS:
    return os << "WRONG_N_NEXTHOPS";
  case FibManagerFixture::CheckNextHopResult::NO_NEXTHOP:
    return os << "NO_NEXTHOP";
  case FibManagerFixture::CheckNextHopResult::WRONG_COST:
    return os << "WRONG_COST";
  };

  return os << static_cast<int>(result);
}

BOOST_AUTO_TEST_SUITE(Mgmt)
BOOST_FIXTURE_TEST_SUITE(TestFibManager, FibManagerFixture)

BOOST_AUTO_TEST_SUITE(AddNextHop)

BOOST_AUTO_TEST_CASE(UnknownFaceId)
{
  auto req = makeControlCommandRequest("/localhost/nfd/fib/add-nexthop",
                                       makeParameters("hello", face::FACEID_NULL, 101));
  receiveInterest(req);
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);

  // check response
  BOOST_CHECK_EQUAL(checkResponse(0, req.getName(), ControlResponse(410, "Face not found")),
                    CheckResponseResult::OK);

  // double check that the next hop was not added
  BOOST_CHECK_EQUAL(checkNextHop("/hello", nullopt, nullopt, 101), CheckNextHopResult::NO_FIB_ENTRY);
}

BOOST_AUTO_TEST_CASE(NameTooLong)
{
  Name prefix;
  while (prefix.size() <= FIB_MAX_DEPTH) {
    prefix.append("A");
  }

  auto req = makeControlCommandRequest("/localhost/nfd/fib/add-nexthop",
                                       makeParameters(prefix, addFace()));
  receiveInterest(req);

  ControlResponse expected(414, "FIB entry prefix cannot exceed " +
                                ndn::to_string(Fib::getMaxDepth()) + " components");
  BOOST_CHECK_EQUAL(checkResponse(0, req.getName(), expected), CheckResponseResult::OK);

  BOOST_CHECK_EQUAL(checkNextHop(prefix), CheckNextHopResult::NO_FIB_ENTRY);
}

BOOST_AUTO_TEST_CASE(ImplicitFaceId)
{
  auto face1 = addFace();
  auto face2 = addFace();
  BOOST_REQUIRE_NE(face1, face::INVALID_FACEID);
  BOOST_REQUIRE_NE(face2, face::INVALID_FACEID);

  Name expectedName;
  ControlResponse expectedResponse;
  auto testAddNextHop = [&] (ControlParameters parameters, const FaceId& faceId) {
    auto req = makeControlCommandRequest("/localhost/nfd/fib/add-nexthop", parameters);
    req.setTag(make_shared<lp::IncomingFaceIdTag>(faceId));
    m_responses.clear();
    expectedName = req.getName();
    expectedResponse = makeResponse(200, "Success", parameters.setFaceId(faceId));
    receiveInterest(req);
  };

  testAddNextHop(ControlParameters().setName("/hello").setCost(100).setFaceId(0), face1);
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(checkResponse(0, expectedName, expectedResponse), CheckResponseResult::OK);
  BOOST_CHECK_EQUAL(checkNextHop("/hello", 1, face1, 100), CheckNextHopResult::OK);

  testAddNextHop(ControlParameters().setName("/hello").setCost(100), face2);
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(checkResponse(0, expectedName, expectedResponse), CheckResponseResult::OK);
  BOOST_CHECK_EQUAL(checkNextHop("/hello", 2, face2, 100), CheckNextHopResult::OK);
}

BOOST_AUTO_TEST_CASE(InitialAdd)
{
  FaceId addedFaceId = addFace();
  BOOST_REQUIRE_NE(addedFaceId, face::INVALID_FACEID);

  auto parameters = makeParameters("hello", addedFaceId, 101);
  auto req = makeControlCommandRequest("/localhost/nfd/fib/add-nexthop", parameters);
  receiveInterest(req);

  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(checkResponse(0, req.getName(), makeResponse(200, "Success", parameters)),
                    CheckResponseResult::OK);
  BOOST_CHECK_EQUAL(checkNextHop("/hello", 1, addedFaceId, 101), CheckNextHopResult::OK);
}

BOOST_AUTO_TEST_CASE(ImplicitCost)
{
  FaceId addedFaceId = addFace();
  BOOST_REQUIRE_NE(addedFaceId, face::INVALID_FACEID);

  auto originalParameters = ControlParameters().setName("/hello").setFaceId(addedFaceId);
  auto parameters = makeParameters("/hello", addedFaceId, 0);
  auto req = makeControlCommandRequest("/localhost/nfd/fib/add-nexthop", originalParameters);
  receiveInterest(req);

  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(checkResponse(0, req.getName(), makeResponse(200, "Success", parameters)),
                    CheckResponseResult::OK);
  BOOST_CHECK_EQUAL(checkNextHop("/hello", 1, addedFaceId, 0), CheckNextHopResult::OK);
}

BOOST_AUTO_TEST_CASE(AddToExisting)
{
  FaceId face = addFace();
  BOOST_REQUIRE_NE(face, face::INVALID_FACEID);

  Name expectedName;
  ControlResponse expectedResponse;
  auto testAddNextHop = [&] (const ControlParameters& parameters) {
    m_responses.clear();
    auto req = makeControlCommandRequest("/localhost/nfd/fib/add-nexthop", parameters);
    expectedName = req.getName();
    expectedResponse = makeResponse(200, "Success", parameters);
    receiveInterest(req);
  };

  // add initial, succeeds
  testAddNextHop(makeParameters("/hello", face, 101));
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(checkResponse(0, expectedName, expectedResponse), CheckResponseResult::OK);

  // add to existing --> update cost, succeeds
  testAddNextHop(makeParameters("/hello", face, 102));
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(checkResponse(0, expectedName, expectedResponse), CheckResponseResult::OK);

  BOOST_CHECK_EQUAL(checkNextHop("/hello", 2, face, 102), CheckNextHopResult::WRONG_N_NEXTHOPS);
  BOOST_CHECK_EQUAL(checkNextHop("/hello", 1, face, 101), CheckNextHopResult::WRONG_COST);
  BOOST_CHECK_EQUAL(checkNextHop("/hello", 1, face, 102), CheckNextHopResult::OK);
}

BOOST_AUTO_TEST_SUITE_END() // AddNextHop

BOOST_AUTO_TEST_SUITE(RemoveNextHop)

BOOST_AUTO_TEST_CASE(Basic)
{
  Name expectedName;
  ControlResponse expectedResponse;
  auto testRemoveNextHop = [&] (const ControlParameters& parameters) {
    m_responses.clear();
    auto req = makeControlCommandRequest("/localhost/nfd/fib/remove-nexthop", parameters);
    expectedName = req.getName();
    expectedResponse = makeResponse(200, "Success", parameters);
    receiveInterest(req);
  };

  FaceId face1 = addFace();
  FaceId face2 = addFace();
  FaceId face3 = addFace();
  BOOST_REQUIRE_NE(face1, face::INVALID_FACEID);
  BOOST_REQUIRE_NE(face2, face::INVALID_FACEID);
  BOOST_REQUIRE_NE(face3, face::INVALID_FACEID);

  fib::Entry* entry = m_fib.insert("/hello").first;
  entry->addNextHop(*m_faceTable.get(face1), 101);
  entry->addNextHop(*m_faceTable.get(face2), 202);
  entry->addNextHop(*m_faceTable.get(face3), 303);

  testRemoveNextHop(makeParameters("/hello", face1));
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(checkResponse(0, expectedName, expectedResponse), CheckResponseResult::OK);
  BOOST_CHECK_EQUAL(checkNextHop("/hello", 2, face1, 101), CheckNextHopResult::NO_NEXTHOP);

  testRemoveNextHop(makeParameters("/hello", face2));
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(checkResponse(0, expectedName, expectedResponse), CheckResponseResult::OK);
  BOOST_CHECK_EQUAL(checkNextHop("/hello", 1, face2, 202), CheckNextHopResult::NO_NEXTHOP);

  testRemoveNextHop(makeParameters("/hello", face3));
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(checkResponse(0, expectedName, expectedResponse), CheckResponseResult::OK);
  BOOST_CHECK_EQUAL(checkNextHop("/hello", 0, face3, 303), CheckNextHopResult::NO_FIB_ENTRY);
}

BOOST_AUTO_TEST_CASE(PrefixNotFound)
{
  FaceId addedFaceId = addFace();
  BOOST_REQUIRE_NE(addedFaceId, face::INVALID_FACEID);

  auto parameters = makeParameters("hello", addedFaceId);
  auto req = makeControlCommandRequest("/localhost/nfd/fib/remove-nexthop", parameters);
  receiveInterest(req);
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);

  auto expectedResponse = makeResponse(200, "Success", parameters);
  BOOST_CHECK_EQUAL(checkResponse(0, req.getName(), expectedResponse), CheckResponseResult::OK);
}

BOOST_AUTO_TEST_CASE(ImplicitFaceId)
{
  auto face1 = addFace();
  auto face2 = addFace();
  BOOST_REQUIRE_NE(face1, face::INVALID_FACEID);
  BOOST_REQUIRE_NE(face2, face::INVALID_FACEID);

  Name expectedName;
  ControlResponse expectedResponse;
  auto testWithImplicitFaceId = [&] (ControlParameters parameters, FaceId face) {
    m_responses.clear();
    auto req = makeControlCommandRequest("/localhost/nfd/fib/remove-nexthop", parameters);
    req.setTag(make_shared<lp::IncomingFaceIdTag>(face));
    expectedName = req.getName();
    expectedResponse = makeResponse(200, "Success", parameters.setFaceId(face));
    receiveInterest(req);
  };

  fib::Entry* entry = m_fib.insert("/hello").first;
  entry->addNextHop(*m_faceTable.get(face1), 101);
  entry->addNextHop(*m_faceTable.get(face2), 202);

  testWithImplicitFaceId(ControlParameters().setName("/hello").setFaceId(0), face1);
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(checkResponse(0, expectedName, expectedResponse), CheckResponseResult::OK);
  BOOST_CHECK_EQUAL(checkNextHop("/hello", 1, face1, 101), CheckNextHopResult::NO_NEXTHOP);

  testWithImplicitFaceId(ControlParameters().setName("/hello"), face2);
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(checkResponse(0, expectedName, expectedResponse), CheckResponseResult::OK);
  BOOST_CHECK_EQUAL(checkNextHop("/hello", 0, face2, 202), CheckNextHopResult::NO_FIB_ENTRY);
}

BOOST_AUTO_TEST_CASE(RecordNotExist)
{
  auto face1 = addFace();
  auto face2 = addFace();
  BOOST_REQUIRE_NE(face1, face::INVALID_FACEID);
  BOOST_REQUIRE_NE(face2, face::INVALID_FACEID);

  Name expectedName;
  ControlResponse expectedResponse;
  auto testRemoveNextHop = [&] (ControlParameters parameters) {
    m_responses.clear();
    auto req = makeControlCommandRequest("/localhost/nfd/fib/remove-nexthop", parameters);
    expectedName = req.getName();
    expectedResponse = makeResponse(200, "Success", parameters);
    receiveInterest(req);
  };

  m_fib.insert("/hello").first->addNextHop(*m_faceTable.get(face1), 101);

  testRemoveNextHop(makeParameters("/hello", face2 + 100));
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1); // face does not exist
  BOOST_CHECK_EQUAL(checkResponse(0, expectedName, expectedResponse), CheckResponseResult::OK);
  BOOST_CHECK_EQUAL(checkNextHop("/hello", nullopt, face2 + 100), CheckNextHopResult::NO_NEXTHOP);

  testRemoveNextHop(makeParameters("/hello", face2));
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1); // record does not exist
  BOOST_CHECK_EQUAL(checkResponse(0, expectedName, expectedResponse), CheckResponseResult::OK);
  BOOST_CHECK_EQUAL(checkNextHop("/hello", nullopt, face2), CheckNextHopResult::NO_NEXTHOP);
}

BOOST_AUTO_TEST_SUITE_END() // RemoveNextHop

BOOST_AUTO_TEST_SUITE(List)

BOOST_AUTO_TEST_CASE(FibDataset)
{
  const size_t nEntries = 108;
  std::set<Name> actualPrefixes;
  for (size_t i = 0 ; i < nEntries ; i ++) {
    Name prefix = Name("test").appendSegment(i);
    actualPrefixes.insert(prefix);
    fib::Entry* fibEntry = m_fib.insert(prefix).first;
    fibEntry->addNextHop(*m_faceTable.get(addFace()), std::numeric_limits<uint8_t>::max() - 1);
    fibEntry->addNextHop(*m_faceTable.get(addFace()), std::numeric_limits<uint8_t>::max() - 2);
  }

  receiveInterest(Interest("/localhost/nfd/fib/list"));

  Block content = concatenateResponses();
  content.parse();
  BOOST_REQUIRE_EQUAL(content.elements().size(), nEntries);

  std::vector<ndn::nfd::FibEntry> receivedRecords, expectedRecords;
  for (size_t idx = 0; idx < nEntries; ++idx) {
    ndn::nfd::FibEntry decodedEntry(content.elements()[idx]);
    receivedRecords.push_back(decodedEntry);
    actualPrefixes.erase(decodedEntry.getPrefix());

    auto matchedEntry = m_fib.findExactMatch(decodedEntry.getPrefix());
    BOOST_REQUIRE(matchedEntry != nullptr);

    expectedRecords.emplace_back();
    expectedRecords.back().setPrefix(matchedEntry->getPrefix());
    for (const auto& nh : matchedEntry->getNextHops()) {
      expectedRecords.back().addNextHopRecord(ndn::nfd::NextHopRecord()
                                              .setFaceId(nh.getFace().getId())
                                              .setCost(nh.getCost()));
    }
  }

  BOOST_CHECK_EQUAL(actualPrefixes.size(), 0);
  BOOST_CHECK_EQUAL_COLLECTIONS(receivedRecords.begin(), receivedRecords.end(),
                                expectedRecords.begin(), expectedRecords.end());
}

BOOST_AUTO_TEST_SUITE_END() // List

BOOST_AUTO_TEST_SUITE_END() // TestFibManager
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace tests
} // namespace nfd
