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
#include "mgmt/fib-manager.hpp"

#include "nfd-manager-common-fixture.hpp"
#include "table/fib-nexthop.hpp"
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
  ControlParameters
  makeParameters(const Name& name, const FaceId& id)
  {
    return ControlParameters().setName(name).setFaceId(id);
  }

  ControlParameters
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
   * @param expectedNNextHops use -1 to skip this check
   * @param faceId use face::FACEID_NULL to skip NextHopRecord checks
   * @param expectedCost use -1 to skip this check
   *
   * @retval OK FIB entry is found by exact match and has the expected number of nexthops;
   *            NextHopRe record for faceId is found and has the expected cost
   * @retval NO_FIB_ENTRY FIB entry is not found
   * @retval WRONG_N_NEXTHOPS FIB entry is found but has wrong number of nexthops
   * @retval NO_NEXTHOP NextHopRecord for faceId is not found
   * @retval WRONG_COST NextHopRecord for faceId has wrong cost
   */
  CheckNextHopResult
  checkNextHop(const Name& prefix, ssize_t expectedNNextHops = -1,
               FaceId faceId = face::FACEID_NULL, int32_t expectedCost = -1)
  {
    const fib::Entry* entry = m_fib.findExactMatch(prefix);
    if (entry == nullptr) {
      return CheckNextHopResult::NO_FIB_ENTRY;
    }

    const fib::NextHopList& nextHops = entry->getNextHops();
    if (expectedNNextHops != -1 && nextHops.size() != static_cast<size_t>(expectedNNextHops)) {
      return CheckNextHopResult::WRONG_N_NEXTHOPS;
    }

    if (faceId != face::FACEID_NULL) {
      for (auto&& record : nextHops) {
        if (record.getFace().getId() == faceId) {
          return expectedCost != -1 && record.getCost() != static_cast<uint32_t>(expectedCost) ?
            CheckNextHopResult::WRONG_COST : CheckNextHopResult::OK;
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
operator<<(std::ostream &os, const FibManagerFixture::CheckNextHopResult& result)
{
  switch (result) {
  case FibManagerFixture::CheckNextHopResult::OK:
    os << "OK";
    break;
  case FibManagerFixture::CheckNextHopResult::NO_FIB_ENTRY:
    os << "NO_FIB_ENTRY";
    break;
  case FibManagerFixture::CheckNextHopResult::WRONG_N_NEXTHOPS:
    os << "WRONG_N_NEXTHOPS";
    break;
  case FibManagerFixture::CheckNextHopResult::NO_NEXTHOP:
    os << "NO_NEXTHOP";
    break;
  case FibManagerFixture::CheckNextHopResult::WRONG_COST:
    os << "WRONG_COST";
    break;
  default:
    break;
  };

  return os;
}

BOOST_FIXTURE_TEST_SUITE(Mgmt, FibManagerFixture)
BOOST_AUTO_TEST_SUITE(TestFibManager)

BOOST_AUTO_TEST_SUITE(AddNextHop)

BOOST_AUTO_TEST_CASE(UnknownFaceId)
{
  auto command = makeControlCommandRequest("/localhost/nfd/fib/add-nexthop",
                                           makeParameters("hello", face::FACEID_NULL, 101));
  receiveInterest(command);
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);

  // check response
  BOOST_CHECK_EQUAL(checkResponse(0, command->getName(), ControlResponse(410, "Face not found")),
                    CheckResponseResult::OK);

  // double check that the next hop was not added
  BOOST_CHECK_EQUAL(checkNextHop("/hello", -1, face::FACEID_NULL, 101), CheckNextHopResult::NO_FIB_ENTRY);
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
    auto command = makeControlCommandRequest("/localhost/nfd/fib/add-nexthop", parameters,
                   [&faceId] (shared_ptr<Interest> interest) {
                     interest->setTag(make_shared<lp::IncomingFaceIdTag>(faceId));
                   });
    m_responses.clear();
    expectedName = command->getName();
    expectedResponse = makeResponse(200, "Success", parameters.setFaceId(faceId));
    receiveInterest(command);
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
  auto command = makeControlCommandRequest("/localhost/nfd/fib/add-nexthop", parameters);

  receiveInterest(command);
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(checkResponse(0, command->getName(), makeResponse(200, "Success", parameters)),
                    CheckResponseResult::OK);
  BOOST_CHECK_EQUAL(checkNextHop("/hello", 1, addedFaceId, 101), CheckNextHopResult::OK);
}

BOOST_AUTO_TEST_CASE(ImplicitCost)
{
  FaceId addedFaceId = addFace();
  BOOST_REQUIRE_NE(addedFaceId, face::INVALID_FACEID);

  auto originalParameters = ControlParameters().setName("/hello").setFaceId(addedFaceId);
  auto parameters = makeParameters("/hello", addedFaceId, 0);
  auto command = makeControlCommandRequest("/localhost/nfd/fib/add-nexthop", originalParameters);

  receiveInterest(command);
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(checkResponse(0, command->getName(), makeResponse(200, "Success", parameters)),
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
    auto command = makeControlCommandRequest("/localhost/nfd/fib/add-nexthop", parameters);
    expectedName = command->getName();
    expectedResponse = makeResponse(200, "Success", parameters);
    receiveInterest(command);
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
    auto command = makeControlCommandRequest("/localhost/nfd/fib/remove-nexthop", parameters);
    expectedName = command->getName();
    expectedResponse = makeResponse(200, "Success", parameters);
    receiveInterest(command);
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
  auto command = makeControlCommandRequest("/localhost/nfd/fib/remove-nexthop", parameters);
  auto response = makeResponse(200, "Success", parameters);

  receiveInterest(command);
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(checkResponse(0, command->getName(), response), CheckResponseResult::OK);
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
    auto command = makeControlCommandRequest("/localhost/nfd/fib/remove-nexthop", parameters,
                   [face] (shared_ptr<Interest> interest) {
                     interest->setTag(make_shared<lp::IncomingFaceIdTag>(face));
                   });
    expectedName = command->getName();
    expectedResponse = makeResponse(200, "Success", parameters.setFaceId(face));
    receiveInterest(command);
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
    auto command = makeControlCommandRequest("/localhost/nfd/fib/remove-nexthop", parameters);
    expectedName = command->getName();
    expectedResponse = makeResponse(200, "Success", parameters);
    receiveInterest(command);
  };

  m_fib.insert("/hello").first->addNextHop(*m_faceTable.get(face1), 101);

  testRemoveNextHop(makeParameters("/hello", face2 + 100));
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1); // face does not exist
  BOOST_CHECK_EQUAL(checkResponse(0, expectedName, expectedResponse), CheckResponseResult::OK);
  BOOST_CHECK_EQUAL(checkNextHop("/hello", -1, face2 + 100), CheckNextHopResult::NO_NEXTHOP);

  testRemoveNextHop(makeParameters("/hello", face2));
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1); // record does not exist
  BOOST_CHECK_EQUAL(checkResponse(0, expectedName, expectedResponse), CheckResponseResult::OK);
  BOOST_CHECK_EQUAL(checkNextHop("/hello", -1, face2), CheckNextHopResult::NO_NEXTHOP);
}

BOOST_AUTO_TEST_SUITE_END() // RemoveNextHop

// @todo Remove when ndn::nfd::FibEntry implements operator!= and operator<<
class FibEntry : public ndn::nfd::FibEntry
{
public:
  FibEntry() = default;

  FibEntry(const ndn::nfd::FibEntry& entry)
    : ndn::nfd::FibEntry(entry)
  {
  }
};

bool
operator!=(const FibEntry& left, const FibEntry& right)
{
  if (left.getPrefix() != right.getPrefix()) {
    return true;
  }

  auto leftNextHops = left.getNextHopRecords();
  auto rightNextHops = right.getNextHopRecords();
  if (leftNextHops.size() != rightNextHops.size()) {
    return true;
  }

  for (auto&& nexthop : leftNextHops) {
    auto hitEntry =
      std::find_if(rightNextHops.begin(), rightNextHops.end(), [&] (const ndn::nfd::NextHopRecord& record) {
          return nexthop.getCost() == record.getCost() && nexthop.getFaceId() == record.getFaceId();
        });

    if (hitEntry == rightNextHops.end()) {
      return true;
    }
  }

  return false;
}

std::ostream&
operator<<(std::ostream &os, const FibEntry& entry)
{
  const auto& nexthops = entry.getNextHopRecords();
  os << "[" << entry.getPrefix() << ", " << nexthops.size() << ": ";
  for (auto record : nexthops) {
    os << "{" << record.getFaceId() << ", " << record.getCost() << "} ";
  }
  os << "]";

  return os;
}

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

  receiveInterest(makeInterest("/localhost/nfd/fib/list"));

  Block content;
  BOOST_CHECK_NO_THROW(content = concatenateResponses());
  BOOST_CHECK_NO_THROW(content.parse());
  BOOST_REQUIRE_EQUAL(content.elements().size(), nEntries);

  std::vector<FibEntry> receivedRecords, expectedRecords;
  for (size_t idx = 0; idx < nEntries; ++idx) {
    BOOST_TEST_MESSAGE("processing element: " << idx);

    FibEntry decodedEntry;
    BOOST_REQUIRE_NO_THROW(decodedEntry.wireDecode(content.elements()[idx]));
    receivedRecords.push_back(decodedEntry);

    actualPrefixes.erase(decodedEntry.getPrefix());

    auto matchedEntry = m_fib.findExactMatch(decodedEntry.getPrefix());
    BOOST_REQUIRE(matchedEntry != nullptr);

    FibEntry record;
    record.setPrefix(matchedEntry->getPrefix());
    const auto& nextHops = matchedEntry->getNextHops();
    for (auto&& next : nextHops) {
      ndn::nfd::NextHopRecord nextHopRecord;
      nextHopRecord.setFaceId(next.getFace().getId());
      nextHopRecord.setCost(next.getCost());
      record.addNextHopRecord(nextHopRecord);
    }
    expectedRecords.push_back(record);
  }

  BOOST_CHECK_EQUAL(actualPrefixes.size(), 0);

  BOOST_CHECK_EQUAL_COLLECTIONS(receivedRecords.begin(), receivedRecords.end(),
                                expectedRecords.begin(), expectedRecords.end());
}

BOOST_AUTO_TEST_SUITE_END() // TestFibManager
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace tests
} // namespace nfd
