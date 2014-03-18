/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_MGMT_STATUS_SERVER_HPP
#define NFD_MGMT_STATUS_SERVER_HPP

#include "mgmt/app-face.hpp"
#include <ndn-cpp-dev/management/nfd-status.hpp>

namespace nfd {

class Forwarder;

class StatusServer : noncopyable
{
public:
  StatusServer(shared_ptr<AppFace> face, Forwarder& forwarder);

private:
  void
  onInterest(const Interest& interest) const;

  shared_ptr<ndn::nfd::Status>
  collectStatus() const;

private:
  static const Name DATASET_PREFIX;
  static const time::milliseconds RESPONSE_FRESHNESS;

  shared_ptr<AppFace> m_face;
  Forwarder& m_forwarder;
  time::system_clock::TimePoint m_startTimestamp;
};

} // namespace nfd

#endif // NFD_MGMT_STATUS_SERVER_HPP
