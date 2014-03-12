/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

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

