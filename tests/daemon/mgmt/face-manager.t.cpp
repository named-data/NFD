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

#include "mgmt/face-manager.hpp"
#include "core/random.hpp"
#include "face/protocol-factory.hpp"

#include "nfd-manager-common-fixture.hpp"
#include "../face/dummy-face.hpp"
#include "../face/dummy-transport.hpp"

#include <ndn-cxx/encoding/tlv.hpp>
#include <ndn-cxx/encoding/tlv-nfd.hpp>
#include <ndn-cxx/mgmt/nfd/channel-status.hpp>
#include <ndn-cxx/net/network-monitor-stub.hpp>

namespace nfd {
namespace tests {

class FaceManagerFixture : public NfdManagerCommonFixture
{
public:
  FaceManagerFixture()
    : m_faceTable(m_forwarder.getFaceTable())
    , m_faceSystem(m_faceTable, make_shared<ndn::net::NetworkMonitorStub>(0))
    , m_manager(m_faceSystem, m_dispatcher, *m_authenticator)
  {
    setTopPrefix();
    setPrivilege("faces");
  }

public:
  enum AddFaceFlags {
    REMOVE_LAST_NOTIFICATION = 1 << 0,
    SET_SCOPE_LOCAL          = 1 << 1,
    SET_URI_TEST             = 1 << 2,
    RANDOMIZE_COUNTERS       = 1 << 3,
  };

  /** \brief adds a face to the FaceTable
   *  \param options bitwise OR'ed AddFaceFlags
   */
  shared_ptr<Face>
  addFace(unsigned int flags = 0)
  {
    std::string uri = "dummy://";
    ndn::nfd::FaceScope scope = ndn::nfd::FACE_SCOPE_NON_LOCAL;

    if ((flags & SET_SCOPE_LOCAL) != 0) {
      scope = ndn::nfd::FACE_SCOPE_LOCAL;
    }
    if ((flags & SET_URI_TEST) != 0) {
      uri = "test://";
    }

    auto face = make_shared<DummyFace>(uri, uri, scope);
    m_faceTable.add(face);

    if ((flags & RANDOMIZE_COUNTERS) != 0) {
      const face::FaceCounters& counters = face->getCounters();
      randomizeCounter(counters.nInInterests);
      randomizeCounter(counters.nOutInterests);
      randomizeCounter(counters.nInData);
      randomizeCounter(counters.nOutData);
      randomizeCounter(counters.nInNacks);
      randomizeCounter(counters.nOutNacks);
      randomizeCounter(counters.nInPackets);
      randomizeCounter(counters.nOutPackets);
      randomizeCounter(counters.nInBytes);
      randomizeCounter(counters.nOutBytes);
    }

    advanceClocks(time::milliseconds(1), 10); // wait for notification posted
    if ((flags & REMOVE_LAST_NOTIFICATION) != 0) {
      m_responses.pop_back();
    }

    return face;
  }

private:
  template<typename T>
  static void
  randomizeCounter(const T& counter)
  {
    static std::uniform_int_distribution<typename T::rep> dist;
    const_cast<T&>(counter).set(dist(getGlobalRng()));
  }

protected:
  FaceTable& m_faceTable;
  FaceSystem m_faceSystem;
  FaceManager m_manager;
};

BOOST_AUTO_TEST_SUITE(Mgmt)
BOOST_FIXTURE_TEST_SUITE(TestFaceManager, FaceManagerFixture)

BOOST_AUTO_TEST_SUITE(DestroyFace)

BOOST_AUTO_TEST_CASE(Existing)
{
  auto addedFace = addFace(REMOVE_LAST_NOTIFICATION | SET_SCOPE_LOCAL); // clear notification for creation

  auto parameters = ControlParameters().setFaceId(addedFace->getId());
  auto req = makeControlCommandRequest("/localhost/nfd/faces/destroy", parameters);
  receiveInterest(req);

  BOOST_REQUIRE_EQUAL(m_responses.size(), 2); // one response and one notification
  // notification is already tested, so ignore it

  BOOST_CHECK_EQUAL(checkResponse(1, req.getName(), makeResponse(200, "OK", parameters)),
                    CheckResponseResult::OK);

  BOOST_CHECK_EQUAL(addedFace->getId(), face::INVALID_FACEID);
}

BOOST_AUTO_TEST_CASE(NonExisting)
{
  auto parameters = ControlParameters().setFaceId(65535);
  auto req = makeControlCommandRequest("/localhost/nfd/faces/destroy", parameters);
  receiveInterest(req);

  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  BOOST_CHECK_EQUAL(checkResponse(0, req.getName(), makeResponse(200, "OK", parameters)),
                    CheckResponseResult::OK);
}

BOOST_AUTO_TEST_SUITE_END() // DestroyFace

BOOST_AUTO_TEST_SUITE(Datasets)

BOOST_AUTO_TEST_CASE(FaceDataset)
{
  const size_t nEntries = 303;
  for (size_t i = 0; i < nEntries; ++i) {
    addFace(REMOVE_LAST_NOTIFICATION | SET_URI_TEST | RANDOMIZE_COUNTERS);
  }

  receiveInterest(Interest("/localhost/nfd/faces/list"));

  Block content = concatenateResponses();
  content.parse();
  BOOST_REQUIRE_EQUAL(content.elements().size(), nEntries);

  std::set<FaceId> faceIds;
  for (size_t idx = 0; idx < nEntries; ++idx) {
    ndn::nfd::FaceStatus decodedStatus(content.elements()[idx]);
    BOOST_CHECK(m_faceTable.get(decodedStatus.getFaceId()) != nullptr);
    faceIds.insert(decodedStatus.getFaceId());
  }

  BOOST_CHECK_EQUAL(faceIds.size(), nEntries);
  // TODO#3325 check dataset contents including counter values
}

BOOST_AUTO_TEST_CASE(FaceQuery)
{
  using ndn::nfd::FaceQueryFilter;

  auto face1 = addFace(REMOVE_LAST_NOTIFICATION); // dummy://
  auto face2 = addFace(REMOVE_LAST_NOTIFICATION | SET_SCOPE_LOCAL); // dummy://, local
  auto face3 = addFace(REMOVE_LAST_NOTIFICATION | SET_URI_TEST); // test://

  auto generateQueryName = [] (const FaceQueryFilter& filter) {
    return Name("/localhost/nfd/faces/query").append(filter.wireEncode());
  };

  auto querySchemeName = generateQueryName(FaceQueryFilter().setUriScheme("dummy"));
  auto queryIdName = generateQueryName(FaceQueryFilter().setFaceId(face1->getId()));
  auto queryScopeName = generateQueryName(FaceQueryFilter().setFaceScope(ndn::nfd::FACE_SCOPE_NON_LOCAL));
  auto invalidQueryName = Name("/localhost/nfd/faces/query")
                          .append(ndn::makeStringBlock(tlv::Content, "invalid"));

  receiveInterest(Interest(querySchemeName)); // face1 and face2 expected
  receiveInterest(Interest(queryIdName)); // face1 expected
  receiveInterest(Interest(queryScopeName)); // face1 and face3 expected
  receiveInterest(Interest(invalidQueryName)); // nack expected

  BOOST_REQUIRE_EQUAL(m_responses.size(), 4);

  Block content;
  ndn::nfd::FaceStatus status;

  content = m_responses[0].getContent();
  content.parse();
  BOOST_CHECK_EQUAL(content.elements().size(), 2); // face1 and face2
  status.wireDecode(content.elements()[0]);
  BOOST_CHECK_EQUAL(face1->getId(), status.getFaceId());
  status.wireDecode(content.elements()[1]);
  BOOST_CHECK_EQUAL(face2->getId(), status.getFaceId());

  content = m_responses[1].getContent();
  content.parse();
  BOOST_CHECK_EQUAL(content.elements().size(), 1); // face1
  status.wireDecode(content.elements()[0]);
  BOOST_CHECK_EQUAL(face1->getId(), status.getFaceId());

  content = m_responses[2].getContent();
  content.parse();
  BOOST_CHECK_EQUAL(content.elements().size(), 2); // face1 and face3
  status.wireDecode(content.elements()[0]);
  BOOST_CHECK_EQUAL(face1->getId(), status.getFaceId());
  status.wireDecode(content.elements()[1]);
  BOOST_CHECK_EQUAL(face3->getId(), status.getFaceId());

  ControlResponse expectedResponse(400, "Malformed filter"); // nack, 400, malformed filter
  BOOST_CHECK_EQUAL(checkResponse(3, invalidQueryName, expectedResponse, tlv::ContentType_Nack),
                    CheckResponseResult::OK);
}

class TestChannel : public face::Channel
{
public:
  explicit
  TestChannel(const std::string& uri)
  {
    setUri(FaceUri(uri));
  }

  bool
  isListening() const final
  {
    return false;
  }

  size_t
  size() const final
  {
    return 0;
  }
};

class TestProtocolFactory : public face::ProtocolFactory
{
public:
  TestProtocolFactory(const CtorParams& params)
    : ProtocolFactory(params)
  {
  }

  void
  processConfig(OptionalConfigSection configSection,
                FaceSystem::ConfigContext& context) final
  {
  }

  void
  createFace(const CreateFaceRequest& req,
             const face::FaceCreatedCallback& onCreated,
             const face::FaceCreationFailedCallback& onConnectFailed) final
  {
  }

  std::vector<shared_ptr<const face::Channel>>
  getChannels() const final
  {
    return m_channels;
  }

public:
  shared_ptr<TestChannel>
  addChannel(const std::string& channelUri)
  {
    auto channel = make_shared<TestChannel>(channelUri);
    m_channels.push_back(channel);
    return channel;
  }

private:
  std::vector<shared_ptr<const face::Channel>> m_channels;
};

BOOST_AUTO_TEST_CASE(ChannelDataset)
{
  m_faceSystem.m_factories["test"] =
    make_unique<TestProtocolFactory>(m_faceSystem.makePFCtorParams());
  auto factory = static_cast<TestProtocolFactory*>(m_faceSystem.getFactoryById("test"));

  const size_t nEntries = 404;
  std::map<std::string, shared_ptr<TestChannel>> addedChannels;
  for (size_t i = 0; i < nEntries; i++) {
    auto channel = factory->addChannel("test" + to_string(i) + "://");
    addedChannels[channel->getUri().toString()] = channel;
  }

  receiveInterest(Interest("/localhost/nfd/faces/channels"));

  Block content = concatenateResponses();
  content.parse();
  BOOST_REQUIRE_EQUAL(content.elements().size(), nEntries);

  for (size_t idx = 0; idx < nEntries; ++idx) {
    ndn::nfd::ChannelStatus decodedStatus(content.elements()[idx]);
    BOOST_CHECK(addedChannels.find(decodedStatus.getLocalUri()) != addedChannels.end());
  }
}

BOOST_AUTO_TEST_SUITE_END() // Datasets

BOOST_AUTO_TEST_SUITE(Notifications)

BOOST_AUTO_TEST_CASE(FaceEventCreated)
{
  auto face = addFace(); // trigger FACE_EVENT_CREATED notification
  BOOST_CHECK_NE(face->getId(), face::INVALID_FACEID);
  FaceId faceId = face->getId();

  BOOST_CHECK_EQUAL(m_manager.m_faceStateChangeConn.count(faceId), 1);

  // check notification
  BOOST_REQUIRE_EQUAL(m_responses.size(), 1);
  Block payload = m_responses.back().getContent().blockFromValue();
  BOOST_CHECK_EQUAL(payload.type(), ndn::tlv::nfd::FaceEventNotification);
  ndn::nfd::FaceEventNotification notification(payload);
  BOOST_CHECK_EQUAL(notification.getKind(), ndn::nfd::FACE_EVENT_CREATED);
  BOOST_CHECK_EQUAL(notification.getFaceId(), faceId);
  BOOST_CHECK_EQUAL(notification.getRemoteUri(), face->getRemoteUri().toString());
  BOOST_CHECK_EQUAL(notification.getLocalUri(), face->getLocalUri().toString());
  BOOST_CHECK_EQUAL(notification.getFaceScope(), ndn::nfd::FACE_SCOPE_NON_LOCAL);
  BOOST_CHECK_EQUAL(notification.getFacePersistency(), ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  BOOST_CHECK_EQUAL(notification.getLinkType(), ndn::nfd::LinkType::LINK_TYPE_POINT_TO_POINT);
  BOOST_CHECK_EQUAL(notification.getFlags(), 0);
}

BOOST_AUTO_TEST_CASE(FaceEventDownUp)
{
  auto face = addFace();
  BOOST_CHECK_NE(face->getId(), face::INVALID_FACEID);
  FaceId faceId = face->getId();

  // trigger FACE_EVENT_DOWN notification
  dynamic_cast<face::tests::DummyTransport*>(face->getTransport())->setState(face::FaceState::DOWN);
  advanceClocks(time::milliseconds(1), 10);
  BOOST_CHECK_EQUAL(face->getState(), face::FaceState::DOWN);

  // check notification
  {
    BOOST_REQUIRE_EQUAL(m_responses.size(), 2);
    Block payload = m_responses.back().getContent().blockFromValue();
    BOOST_CHECK_EQUAL(payload.type(), ndn::tlv::nfd::FaceEventNotification);
    ndn::nfd::FaceEventNotification notification(payload);
    BOOST_CHECK_EQUAL(notification.getKind(), ndn::nfd::FACE_EVENT_DOWN);
    BOOST_CHECK_EQUAL(notification.getFaceId(), faceId);
    BOOST_CHECK_EQUAL(notification.getRemoteUri(), face->getRemoteUri().toString());
    BOOST_CHECK_EQUAL(notification.getLocalUri(), face->getLocalUri().toString());
    BOOST_CHECK_EQUAL(notification.getFaceScope(), ndn::nfd::FACE_SCOPE_NON_LOCAL);
    BOOST_CHECK_EQUAL(notification.getFacePersistency(), ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
    BOOST_CHECK_EQUAL(notification.getLinkType(), ndn::nfd::LinkType::LINK_TYPE_POINT_TO_POINT);
    BOOST_CHECK_EQUAL(notification.getFlags(), 0);
  }

  // trigger FACE_EVENT_UP notification
  dynamic_cast<face::tests::DummyTransport*>(face->getTransport())->setState(face::FaceState::UP);
  advanceClocks(time::milliseconds(1), 10);
  BOOST_CHECK_EQUAL(face->getState(), face::FaceState::UP);

  // check notification
  {
    BOOST_REQUIRE_EQUAL(m_responses.size(), 3);
    Block payload = m_responses.back().getContent().blockFromValue();
    BOOST_CHECK_EQUAL(payload.type(), ndn::tlv::nfd::FaceEventNotification);
    ndn::nfd::FaceEventNotification notification(payload);
    BOOST_CHECK_EQUAL(notification.getKind(), ndn::nfd::FACE_EVENT_UP);
    BOOST_CHECK_EQUAL(notification.getFaceId(), faceId);
    BOOST_CHECK_EQUAL(notification.getRemoteUri(), face->getRemoteUri().toString());
    BOOST_CHECK_EQUAL(notification.getLocalUri(), face->getLocalUri().toString());
    BOOST_CHECK_EQUAL(notification.getFaceScope(), ndn::nfd::FACE_SCOPE_NON_LOCAL);
    BOOST_CHECK_EQUAL(notification.getFacePersistency(), ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
    BOOST_CHECK_EQUAL(notification.getLinkType(), ndn::nfd::LinkType::LINK_TYPE_POINT_TO_POINT);
    BOOST_CHECK_EQUAL(notification.getFlags(), 0);
  }
}

BOOST_AUTO_TEST_CASE(FaceEventDestroyed)
{
  auto face = addFace();
  BOOST_CHECK_NE(face->getId(), face::INVALID_FACEID);
  FaceId faceId = face->getId();

  BOOST_CHECK_EQUAL(m_manager.m_faceStateChangeConn.count(faceId), 1);

  face->close(); // trigger FaceDestroy FACE_EVENT_DESTROYED
  advanceClocks(time::milliseconds(1), 10);

  // check notification
  BOOST_REQUIRE_EQUAL(m_responses.size(), 2);
  Block payload = m_responses.back().getContent().blockFromValue();
  BOOST_CHECK_EQUAL(payload.type(), ndn::tlv::nfd::FaceEventNotification);
  ndn::nfd::FaceEventNotification notification(payload);
  BOOST_CHECK_EQUAL(notification.getKind(), ndn::nfd::FACE_EVENT_DESTROYED);
  BOOST_CHECK_EQUAL(notification.getFaceId(), faceId);
  BOOST_CHECK_EQUAL(notification.getRemoteUri(), face->getRemoteUri().toString());
  BOOST_CHECK_EQUAL(notification.getLocalUri(), face->getLocalUri().toString());
  BOOST_CHECK_EQUAL(notification.getFaceScope(), ndn::nfd::FACE_SCOPE_NON_LOCAL);
  BOOST_CHECK_EQUAL(notification.getFacePersistency(), ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  BOOST_CHECK_EQUAL(notification.getLinkType(), ndn::nfd::LinkType::LINK_TYPE_POINT_TO_POINT);
  BOOST_CHECK_EQUAL(notification.getFlags(), 0);

  BOOST_CHECK_EQUAL(face->getId(), face::INVALID_FACEID);
  BOOST_CHECK_EQUAL(m_manager.m_faceStateChangeConn.count(faceId), 0);
}

BOOST_AUTO_TEST_SUITE_END() // Notifications

BOOST_AUTO_TEST_SUITE_END() // TestFaceManager
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace tests
} // namespace nfd
