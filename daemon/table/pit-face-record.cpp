/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "pit-face-record.hpp"

namespace nfd {
namespace pit {

FaceRecord::FaceRecord(shared_ptr<Face> face)
  : m_face(face)
  , m_lastNonce(0)
  , m_lastRenewed(0)
  , m_expiry(0)
{
}

FaceRecord::FaceRecord(const FaceRecord& other)
  : m_face(other.m_face)
  , m_lastNonce(other.m_lastNonce)
  , m_lastRenewed(other.m_lastRenewed)
  , m_expiry(other.m_expiry)
{
}

void
FaceRecord::update(const Interest& interest)
{
  m_lastNonce = interest.getNonce();
  m_lastRenewed = time::now();
  m_expiry = m_lastRenewed + time::milliseconds(interest.getInterestLifetime());
}


} // namespace pit
} // namespace nfd
