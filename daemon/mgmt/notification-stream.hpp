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

#ifndef NFD_DAEMON_MGMT_NOTIFICATION_STREAM_HPP
#define NFD_DAEMON_MGMT_NOTIFICATION_STREAM_HPP

#include "mgmt/app-face.hpp"

namespace nfd {

class NotificationStream
{
public:
  NotificationStream(shared_ptr<AppFace> face, const Name& prefix, ndn::KeyChain& keyChain);

  ~NotificationStream();

  template <typename T> void
  postNotification(const T& notification);

private:
  shared_ptr<AppFace> m_face;
  const Name m_prefix;
  uint64_t m_sequenceNo;
  ndn::KeyChain& m_keyChain;
};

inline
NotificationStream::NotificationStream(shared_ptr<AppFace> face,
                                       const Name& prefix,
                                       ndn::KeyChain& keyChain)
  : m_face(face)
  , m_prefix(prefix)
  , m_sequenceNo(0)
  , m_keyChain(keyChain)
{
}

template <typename T>
inline void
NotificationStream::postNotification(const T& notification)
{
  Name dataName(m_prefix);
  dataName.appendSegment(m_sequenceNo);
  shared_ptr<Data> data(make_shared<Data>(dataName));
  data->setContent(notification.wireEncode());
  data->setFreshnessPeriod(time::seconds(1));

  m_keyChain.sign(*data);
  m_face->put(*data);

  ++m_sequenceNo;
}

inline
NotificationStream::~NotificationStream()
{
}

} // namespace nfd


#endif // NFD_DAEMON_MGMT_NOTIFICATION_STREAM_HPP
