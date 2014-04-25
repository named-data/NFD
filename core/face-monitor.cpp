/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology,
 *                     The University of Memphis
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

#include "face-monitor.hpp"
#include "core/logger.hpp"

#include <ndn-cxx/face.hpp>

namespace nfd {

NFD_LOG_INIT("FaceMonitor");

FaceMonitor::FaceMonitor(ndn::Face& face)
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

  NFD_LOG_DEBUG("startNotification: Interest Sent: " << interest);

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

  NFD_LOG_DEBUG("In onTimeout, sending interest: " << newInterest);

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

  NFD_LOG_DEBUG("onNotification: Interest sent: " <<  interest);

  m_lastInterestId = m_face.expressInterest(interest,
                                            bind(&FaceMonitor::onNotification, this, _2),
                                            bind(&FaceMonitor::onTimeout, this));
}

} // namespace nfd
