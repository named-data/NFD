/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2014 Regents of the University of California.
 * See COPYING for copyright and distribution information.
 */

#include "common.hpp"
#include "face-monitor.hpp"
#include <ndn-cpp-dev/management/nfd-face-event-notification.hpp>


namespace ndn {

using namespace nfd;

FaceMonitor::FaceMonitor(Face& face)
  : m_face(face)
  , m_isStopped(true)
{
}

FaceMonitor::~FaceMonitor()
{
}

void
FaceMonitor::removeAllSubscribers()
{
  m_notificationCallbacks.clear();
  m_timeoutCallbacks.clear();
  stopNotifications();
}

void
FaceMonitor::stopNotifications()
{
  if (static_cast<bool>(m_lastInterestId))
    m_face.removePendingInterest(m_lastInterestId);
  m_isStopped = true;
}

void
FaceMonitor::startNotifications()
{
  if (m_isStopped == false)
    return; // Notifications cycle has been started.
  m_isStopped = false;

  Interest interest("/localhost/nfd/faces/events");
  interest
    .setMustBeFresh(true)
    .setChildSelector(1)
    .setInterestLifetime(time::seconds(60))
  ;

  //todo: add logging support.
  std::cout << "startNotification: Interest Sent: " << interest << std::endl;

  m_lastInterestId = m_face.expressInterest(interest,
                                            bind(&FaceMonitor::onNotification, this, _2),
                                            bind(&FaceMonitor::onTimeout, this));
}

void
FaceMonitor::addSubscriber(const NotificationCallback& notificationCallback)
{
  addSubscriber(notificationCallback, TimeoutCallback());
}

void
FaceMonitor::addSubscriber(const NotificationCallback& notificationCallback,
                           const TimeoutCallback&  timeoutCallback)
{
  if (static_cast<bool>(notificationCallback))
    m_notificationCallbacks.push_back(notificationCallback);

  if (static_cast<bool>(timeoutCallback))
    m_timeoutCallbacks.push_back(timeoutCallback);
}

void
FaceMonitor::onTimeout()
{
  if (m_isStopped)
    return;

  std::vector<TimeoutCallback>::iterator it;
  for (it = m_timeoutCallbacks.begin();
       it != m_timeoutCallbacks.end(); ++it) {
    (*it)();
    //One of the registered callbacks has cleared the vector,
    //return now as the iterator has been invalidated and
    //the vector is empty.
    if (m_timeoutCallbacks.empty()) {
       return;
    }
  }

  Interest newInterest("/localhost/nfd/faces/events");
  newInterest
    .setMustBeFresh(true)
    .setChildSelector(1)
    .setInterestLifetime(time::seconds(60))
    ;

  //todo: add logging support.
  std::cout << "In onTimeout, sending interest: " << newInterest << std::endl;

  m_lastInterestId = m_face.expressInterest(newInterest,
                                            bind(&FaceMonitor::onNotification, this, _2),
                                            bind(&FaceMonitor::onTimeout, this));
}

void
FaceMonitor::onNotification(const Data& data)
{
  if (m_isStopped)
    return;

  m_lastSequence = data.getName().get(-1).toSegment();
  ndn::nfd::FaceEventNotification notification(data.getContent().blockFromValue());

  std::vector<NotificationCallback>::iterator it;
  for (it = m_notificationCallbacks.begin();
       it != m_notificationCallbacks.end(); ++it) {
    (*it)(notification);
    if (m_notificationCallbacks.empty()) {
      //One of the registered callbacks has cleared the vector.
      //return back, as no one is interested in notifications anymore.
      return;
    }
  }

  //Setting up next notification
  Name nextNotification("/localhost/nfd/faces/events");
  nextNotification.appendSegment(m_lastSequence + 1);

  Interest interest(nextNotification);
  interest.setInterestLifetime(time::seconds(60));

  //todo: add logging support.
  std::cout << "onNotification: Interest sent: " <<  interest << std::endl;

  m_lastInterestId = m_face.expressInterest(interest,
                                            bind(&FaceMonitor::onNotification, this, _2),
                                            bind(&FaceMonitor::onTimeout, this));
}

}//namespace ndn

