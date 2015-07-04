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

#include "face-query-status-publisher-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(MgmtFaceQueryStatusPublisher, FaceQueryStatusPublisherFixture)

BOOST_AUTO_TEST_CASE(NoConditionFilter)
{
  // filter without conditions matches all faces
  ndn::nfd::FaceQueryFilter filter;
  FaceQueryStatusPublisher faceQueryStatusPublisher(m_table, *m_face,
                                                    "/localhost/nfd/FaceStatusPublisherFixture",
                                                    filter, m_keyChain);

  BOOST_CHECK_EQUAL(faceQueryStatusPublisher.doesMatchFilter(m_dummyFace), true);
  BOOST_CHECK_EQUAL(faceQueryStatusPublisher.doesMatchFilter(m_dummyLocalFace), true);
}

BOOST_AUTO_TEST_CASE(AllConditionFilter)
{
  ndn::nfd::FaceQueryFilter filter;
  filter.setUriScheme("dummy")
        .setRemoteUri("dummy://")
        .setLocalUri("dummy://")
        .setFaceScope(ndn::nfd::FACE_SCOPE_NON_LOCAL)
        .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT)
        .setLinkType(ndn::nfd::LINK_TYPE_POINT_TO_POINT);

  FaceQueryStatusPublisher faceQueryStatusPublisher(m_table, *m_face,
                                                    "/localhost/nfd/FaceStatusPublisherFixture",
                                                    filter, m_keyChain);
  BOOST_CHECK_EQUAL(faceQueryStatusPublisher.doesMatchFilter(m_dummyFace), true);
  BOOST_CHECK_EQUAL(faceQueryStatusPublisher.doesMatchFilter(m_dummyLocalFace), false);
}

BOOST_AUTO_TEST_CASE(UriSchemeFilter)
{
  ndn::nfd::FaceQueryFilter filter;
  filter.setUriScheme("dummyurischeme");
  FaceQueryStatusPublisher faceQueryStatusPublisher(m_table, *m_face,
                                                    "/localhost/nfd/FaceStatusPublisherFixture",
                                                    filter, m_keyChain);
  BOOST_CHECK_EQUAL(faceQueryStatusPublisher.doesMatchFilter(m_dummyFace), false);
  auto dummyUriScheme = make_shared<DummyFace>("dummyurischeme://", "dummyurischeme://");
  BOOST_CHECK_EQUAL(faceQueryStatusPublisher.doesMatchFilter(dummyUriScheme), true);
}

BOOST_AUTO_TEST_CASE(RemoteUriFilter)
{
  ndn::nfd::FaceQueryFilter filter;
  filter.setRemoteUri("dummy://remoteUri");
  FaceQueryStatusPublisher faceQueryStatusPublisher(m_table, *m_face,
                                                    "/localhost/nfd/FaceStatusPublisherFixture",
                                                    filter, m_keyChain);
  BOOST_CHECK_EQUAL(faceQueryStatusPublisher.doesMatchFilter(m_dummyFace), false);
  BOOST_CHECK_EQUAL(faceQueryStatusPublisher.doesMatchFilter(m_dummyUri), true);
}

BOOST_AUTO_TEST_CASE(LocalUriFilter)
{
  ndn::nfd::FaceQueryFilter filter;
  filter.setLocalUri("dummy://localUri");
  FaceQueryStatusPublisher faceQueryStatusPublisher(m_table, *m_face,
                                                    "/localhost/nfd/FaceStatusPublisherFixture",
                                                    filter, m_keyChain);
  BOOST_CHECK_EQUAL(faceQueryStatusPublisher.doesMatchFilter(m_dummyFace), false);
  BOOST_CHECK_EQUAL(faceQueryStatusPublisher.doesMatchFilter(m_dummyUri), true);
}


BOOST_AUTO_TEST_CASE(LinkTypeFilter)
{
  shared_ptr<MulticastUdpFace> multicastFace = m_factory.createMulticastFace("0.0.0.0",
                                                                             "224.0.0.1",
                                                                             "20070");
  ndn::nfd::FaceQueryFilter filter;
  filter.setLinkType(ndn::nfd::LINK_TYPE_MULTI_ACCESS);
  FaceQueryStatusPublisher faceQueryStatusPublisher(m_table, *m_face,
                                                    "/localhost/nfd/FaceStatusPublisherFixture",
                                                    filter, m_keyChain);
  BOOST_CHECK_EQUAL(faceQueryStatusPublisher.doesMatchFilter(m_dummyFace), false);
  BOOST_CHECK_EQUAL(faceQueryStatusPublisher.doesMatchFilter(multicastFace), true);
}

BOOST_AUTO_TEST_CASE(PersistencyFilter)
{
  shared_ptr<MulticastUdpFace> multicastFace = m_factory.createMulticastFace("0.0.0.0",
                                                                             "224.0.0.1",
                                                                             "20070");
  ndn::nfd::FaceQueryFilter filter;
  filter.setFacePersistency(ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  FaceQueryStatusPublisher faceQueryStatusPublisher(m_table, *m_face,
                                                    "/localhost/nfd/FaceStatusPublisherFixture",
                                                    filter, m_keyChain);
  BOOST_CHECK_EQUAL(faceQueryStatusPublisher.doesMatchFilter(m_dummyFace), false);
  multicastFace->setPersistency(ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  BOOST_CHECK_EQUAL(faceQueryStatusPublisher.doesMatchFilter(multicastFace), true);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
