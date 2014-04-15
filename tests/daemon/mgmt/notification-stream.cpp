/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
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
 **/

#include "mgmt/notification-stream.hpp"
#include "mgmt/internal-face.hpp"

#include "tests/test-common.hpp"


namespace nfd {
namespace tests {

NFD_LOG_INIT("NotificationStreamTest");



class NotificationStreamFixture : public BaseFixture
{
public:
  NotificationStreamFixture()
    : m_callbackFired(false)
    , m_prefix("/localhost/nfd/NotificationStreamTest")
    , m_message("TestNotificationMessage")
    , m_sequenceNo(0)
  {
  }

  virtual
  ~NotificationStreamFixture()
  {
  }

  void
  validateCallback(const Data& data)
  {
    Name expectedName(m_prefix);
    expectedName.appendSegment(m_sequenceNo);
    BOOST_REQUIRE_EQUAL(data.getName(), expectedName);

    ndn::Block payload = data.getContent();
    std::string message;

    message.append(reinterpret_cast<const char*>(payload.value()), payload.value_size());

    BOOST_REQUIRE_EQUAL(message, m_message);

    m_callbackFired = true;
    ++m_sequenceNo;
  }

  void
  resetCallbackFired()
  {
    m_callbackFired = false;
  }

protected:
  bool m_callbackFired;
  const std::string m_prefix;
  const std::string m_message;
  uint64_t m_sequenceNo;
};

BOOST_FIXTURE_TEST_SUITE(MgmtNotificationStream, NotificationStreamFixture)

class TestNotification
{
public:
  TestNotification(const std::string& message)
    : m_message(message)
  {
  }

  ~TestNotification()
  {
  }

  Block
  wireEncode() const
  {
    ndn::EncodingBuffer buffer;

    prependByteArrayBlock(buffer,
                          ndn::Tlv::Content,
                          reinterpret_cast<const uint8_t*>(m_message.c_str()),
                          m_message.size());
    return buffer.block();
  }

private:
  const std::string m_message;
};

BOOST_AUTO_TEST_CASE(TestPostEvent)
{
  shared_ptr<InternalFace> face(make_shared<InternalFace>());
  NotificationStream notificationStream(face, "/localhost/nfd/NotificationStreamTest");

  face->onReceiveData += bind(&NotificationStreamFixture::validateCallback, this, _1);

  TestNotification event1(m_message);
  notificationStream.postNotification(event1);

  BOOST_REQUIRE(m_callbackFired);

  resetCallbackFired();

  TestNotification event2(m_message);
  notificationStream.postNotification(event2);

  BOOST_REQUIRE(m_callbackFired);
}


BOOST_AUTO_TEST_SUITE_END()


} // namespace tests
} // namespace nfd
