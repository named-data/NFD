/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2014 Regents of the University of California.
 * See COPYING for copyright and distribution information.
 */
#ifndef FACE_MONITOR_HPP
#define FACE_MONITOR_HPP

#include "common.hpp"
#include <ndn-cpp-dev/management/nfd-face-event-notification.hpp>
#include <ndn-cpp-dev/management/nrd-controller.hpp>

namespace ndn {

class FaceMonitor
{
public:
  typedef function<void(const nfd::FaceEventNotification&)> NotificationCallback;
  typedef function<void()> TimeoutCallback;

  typedef std::vector<NotificationCallback> NotificationCallbacks;
  typedef std::vector<TimeoutCallback> TimeoutCallbacks;

  explicit
  FaceMonitor(Face& face);

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
  Face& m_face;
  uint64_t m_lastSequence;
  bool m_isStopped;
  NotificationCallbacks m_notificationCallbacks;
  TimeoutCallbacks m_timeoutCallbacks;
  const PendingInterestId* m_lastInterestId;
};

}//namespace ndn
#endif //FACE_MONITOR_HPP
