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

#ifndef NFD_CORE_FACE_MONITOR_HPP
#define NFD_CORE_FACE_MONITOR_HPP

#include "common.hpp"

#include <ndn-cxx/management/nfd-face-event-notification.hpp>

namespace ndn {
class Face;
class PendingInterestId;
}

namespace nfd {

using ndn::nfd::FaceEventNotification;

/**
 * \brief Helper class to subscribe to face notification events
 *
 * \todo Write test cases
 */
class FaceMonitor : noncopyable
{
public:
  typedef function<void(const ndn::nfd::FaceEventNotification&)> NotificationCallback;
  typedef function<void()> TimeoutCallback;

  typedef std::vector<NotificationCallback> NotificationCallbacks;
  typedef std::vector<TimeoutCallback> TimeoutCallbacks;

  explicit
  FaceMonitor(ndn::Face& face);

  ~FaceMonitor();

  /** \brief Stops all notifications. This method  doesn't remove registered callbacks.
   */
  void
  stopNotifications();

  /** \brief Resumes notifications for added subscribers.
   */
  void
  startNotifications();

  /** \brief Removes all notification subscribers.
   */
  void
  removeAllSubscribers();

  /** \brief Adds a notification subscriber. This method doesn't return on timeouts.
   */
  void
  addSubscriber(const NotificationCallback& notificationCallback);

  /** \brief Adds a notification subscriber.
   */
  void
  addSubscriber(const NotificationCallback& notificationCallback,
                const TimeoutCallback&  timeoutCallback);

private:
  void
  onTimeout();

  void
  onNotification(const Data& data);

private:
  ndn::Face& m_face;
  uint64_t m_lastSequence;
  bool m_isStopped;
  NotificationCallbacks m_notificationCallbacks;
  TimeoutCallbacks m_timeoutCallbacks;
  const ndn::PendingInterestId* m_lastInterestId;
};

} // namespace nfd

#endif // NFD_CORE_FACE_MONITOR_HPP
