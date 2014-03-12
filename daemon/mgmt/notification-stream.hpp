/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */
#ifndef NFD_MGMT_NOTIFICATION_STREAM_HPP
#define NFD_MGMT_NOTIFICATION_STREAM_HPP

#include "mgmt/app-face.hpp"

namespace nfd {

class NotificationStream
{
public:
  NotificationStream(shared_ptr<AppFace> face, const Name& prefix);

  ~NotificationStream();

  template <typename T> void
  postNotification(const T& notification);

private:
  shared_ptr<AppFace> m_face;
  const Name m_prefix;
  uint64_t m_sequenceNo;
};

inline
NotificationStream::NotificationStream(shared_ptr<AppFace> face, const Name& prefix)
  : m_face(face)
  , m_prefix(prefix)
  , m_sequenceNo(0)
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

  m_face->sign(*data);
  m_face->put(*data);

  ++m_sequenceNo;
}

inline
NotificationStream::~NotificationStream()
{
}

} // namespace nfd


#endif // NFD_MGMT_NOTIFICATION_STREAM_HPP
