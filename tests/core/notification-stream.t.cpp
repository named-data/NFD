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

#include "core/notification-stream.hpp"
#include "simple-notification.hpp"

#include "tests/test-common.hpp"
#include <ndn-cxx/util/dummy-client-face.hpp>

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TestNotificationStream, BaseFixture)

BOOST_AUTO_TEST_CASE(Post)
{
  shared_ptr<ndn::util::DummyClientFace> face = ndn::util::makeDummyClientFace();
  ndn::KeyChain keyChain;
  NotificationStream<ndn::util::DummyClientFace> notificationStream(*face,
    "/localhost/nfd/NotificationStreamTest", keyChain);

  SimpleNotification event1("msg1");
  notificationStream.postNotification(event1);
  face->processEvents();
  BOOST_REQUIRE_EQUAL(face->sentDatas.size(), 1);
  BOOST_CHECK_EQUAL(face->sentDatas[0].getName(),
                    "/localhost/nfd/NotificationStreamTest/%FE%00");
  SimpleNotification decoded1;
  BOOST_CHECK_NO_THROW(decoded1.wireDecode(face->sentDatas[0].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(decoded1.getMessage(), "msg1");

  SimpleNotification event2("msg2");
  notificationStream.postNotification(event2);
  face->processEvents();
  BOOST_REQUIRE_EQUAL(face->sentDatas.size(), 2);
  BOOST_CHECK_EQUAL(face->sentDatas[1].getName(),
                    "/localhost/nfd/NotificationStreamTest/%FE%01");
  SimpleNotification decoded2;
  BOOST_CHECK_NO_THROW(decoded2.wireDecode(face->sentDatas[1].getContent().blockFromValue()));
  BOOST_CHECK_EQUAL(decoded2.getMessage(), "msg2");
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
