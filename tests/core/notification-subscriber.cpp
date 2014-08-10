/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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

#include "core/notification-subscriber.hpp"
#include "core/notification-stream.hpp"
#include "simple-notification.hpp"

#include "tests/test-common.hpp"
#include "tests/dummy-client-face.hpp"


namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(CoreNotificationSubscriber, BaseFixture)

class EndToEndFixture : public BaseFixture
{
public:
  EndToEndFixture()
    : streamPrefix("ndn:/NotificationSubscriberTest")
    , publisherFace(makeDummyClientFace())
    , notificationStream(*publisherFace, streamPrefix, publisherKeyChain)
    , subscriberFace(makeDummyClientFace())
    , subscriber(*subscriberFace, streamPrefix)
  {
  }

  /** \brief post one notification and deliver to subscriber
   */
  void
  deliverNotification(const std::string& msg)
  {
    publisherFace->m_sentDatas.clear();
    SimpleNotification notification(msg);
    notificationStream.postNotification(notification);
    publisherFace->processEvents();
    BOOST_REQUIRE_EQUAL(publisherFace->m_sentDatas.size(), 1);

    lastDeliveredSeqNo = publisherFace->m_sentDatas[0].getName().at(-1).toSequenceNumber();

    lastNotification.setMessage("");
    subscriberFace->receive(publisherFace->m_sentDatas[0]);
  }

  void
  afterNotification(const SimpleNotification& notification)
  {
    lastNotification = notification;
  }

  void
  clearNotificationHandlers()
  {
    subscriber.onNotification.clear();
  }

  void
  afterTimeout()
  {
    hasTimeout = true;
  }

  void
  afterDecodeError(const Data& data)
  {
    lastDecodeErrorData = data;
  }

  /** \return true if subscriberFace has an initial request as sole sent Interest
   */
  bool
  hasInitialRequest() const
  {
    if (subscriberFace->m_sentInterests.size() != 1)
      return 0;

    const Interest& interest = subscriberFace->m_sentInterests[0];
    return interest.getName() == streamPrefix &&
           interest.getChildSelector() == 1 &&
           interest.getMustBeFresh() &&
           interest.getInterestLifetime() ==
             NotificationSubscriber<SimpleNotification>::getInterestLifetime();
  }

  /** \return sequence number of the continuation request sent from subscriberFace
   *          or 0 if there's no such request as sole sent Interest
   */
  uint64_t
  getRequestSeqNo() const
  {
    if (subscriberFace->m_sentInterests.size() != 1)
      return 0;

    const Interest& interest = subscriberFace->m_sentInterests[0];
    const Name& name = interest.getName();
    if (streamPrefix.isPrefixOf(name) &&
        name.size() == streamPrefix.size() + 1 &&
        interest.getInterestLifetime() ==
          NotificationSubscriber<SimpleNotification>::getInterestLifetime())
      return name[-1].toSequenceNumber();
    else
      return 0;
  }

protected:
  Name streamPrefix;
  shared_ptr<DummyClientFace> publisherFace;
  ndn::KeyChain publisherKeyChain;
  NotificationStream<DummyClientFace> notificationStream;
  shared_ptr<DummyClientFace> subscriberFace;
  NotificationSubscriber<SimpleNotification> subscriber;

  uint64_t lastDeliveredSeqNo;

  SimpleNotification lastNotification;
  bool hasTimeout;
  Data lastDecodeErrorData;
};

BOOST_FIXTURE_TEST_CASE(EndToEnd, EndToEndFixture)
{
  BOOST_REQUIRE_EQUAL(subscriber.isRunning(), false);

  // has no effect because onNotification has no handler
  subscriber.start();
  BOOST_REQUIRE_EQUAL(subscriber.isRunning(), false);

  subscriber.onNotification += bind(&EndToEndFixture::afterNotification, this, _1);
  subscriber.onTimeout += bind(&EndToEndFixture::afterTimeout, this);
  subscriber.onDecodeError += bind(&EndToEndFixture::afterDecodeError, this, _1);

  // not received when subscriber is not running
  this->deliverNotification("n1");
  subscriberFace->processEvents(time::milliseconds(10));
  BOOST_CHECK(lastNotification.getMessage().empty());
  BOOST_CHECK_EQUAL(subscriberFace->m_sentInterests.size(), 0);

  subscriberFace->m_sentInterests.clear();
  subscriber.start();
  subscriberFace->processEvents(time::milliseconds(10));
  BOOST_REQUIRE_EQUAL(subscriber.isRunning(), true);
  BOOST_CHECK(this->hasInitialRequest());

  // respond to initial request
  subscriberFace->m_sentInterests.clear();
  this->deliverNotification("n2");
  subscriberFace->processEvents(time::milliseconds(10));
  BOOST_CHECK_EQUAL(lastNotification.getMessage(), "n2");
  BOOST_CHECK_EQUAL(this->getRequestSeqNo(), lastDeliveredSeqNo + 1);

  // respond to continuation request
  subscriberFace->m_sentInterests.clear();
  this->deliverNotification("n3");
  subscriberFace->processEvents(time::milliseconds(10));
  BOOST_CHECK_EQUAL(lastNotification.getMessage(), "n3");
  BOOST_CHECK_EQUAL(this->getRequestSeqNo(), lastDeliveredSeqNo + 1);

  // timeout
  subscriberFace->m_sentInterests.clear();
  lastNotification.setMessage("");
  subscriberFace->processEvents(
    NotificationSubscriber<SimpleNotification>::getInterestLifetime() +
    time::milliseconds(1000));
  BOOST_CHECK(lastNotification.getMessage().empty());
  BOOST_CHECK_EQUAL(hasTimeout, true);
  BOOST_CHECK(this->hasInitialRequest());

  // decode error on sequence number
  Name wrongName = streamPrefix;
  wrongName.append("%07%07");
  Data wrongData(wrongName);
  publisherKeyChain.sign(wrongData);
  subscriberFace->receive(wrongData);
  subscriberFace->m_sentInterests.clear();
  lastNotification.setMessage("");
  subscriberFace->processEvents(time::milliseconds(10));
  BOOST_CHECK(lastNotification.getMessage().empty());
  BOOST_CHECK_EQUAL(lastDecodeErrorData.getName(), wrongName);
  BOOST_CHECK(this->hasInitialRequest());

  // decode error in payload
  subscriberFace->m_sentInterests.clear();
  lastNotification.setMessage("");
  this->deliverNotification("\x07n4");
  subscriberFace->processEvents(time::milliseconds(10));
  BOOST_CHECK(lastNotification.getMessage().empty());
  BOOST_CHECK(this->hasInitialRequest());

  // stop if handlers are cleared
  subscriber.onNotification += bind(&EndToEndFixture::clearNotificationHandlers, this);
  subscriberFace->m_sentInterests.clear();
  this->deliverNotification("n5");
  subscriberFace->processEvents(time::milliseconds(10));
  BOOST_CHECK_EQUAL(subscriberFace->m_sentInterests.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
