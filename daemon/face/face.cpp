/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
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

#include "face.hpp"

#include <ndn-cxx/management/nfd-face-event-notification.hpp>

namespace nfd {

Face::Face(const FaceUri& remoteUri, const FaceUri& localUri, bool isLocal, bool isMultiAccess)
  : m_id(INVALID_FACEID)
  , m_remoteUri(remoteUri)
  , m_localUri(localUri)
  , m_isLocal(isLocal)
  , m_persistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT)
  , m_isMultiAccess(isMultiAccess)
  , m_isFailed(false)
{
  onReceiveInterest.connect([this] (const ndn::Interest&) { ++m_counters.getNInInterests(); });
  onReceiveData    .connect([this] (const ndn::Data&)     { ++m_counters.getNInDatas(); });
  onSendInterest   .connect([this] (const ndn::Interest&) { ++m_counters.getNOutInterests(); });
  onSendData       .connect([this] (const ndn::Data&)     { ++m_counters.getNOutDatas(); });
}

Face::~Face()
{
}

bool
Face::isUp() const
{
  return true;
}

bool
Face::decodeAndDispatchInput(const Block& element)
{
  try {
    /// \todo Ensure lazy field decoding process

    if (element.type() == tlv::Interest)
      {
        shared_ptr<Interest> i = make_shared<Interest>();
        i->wireDecode(element);
        this->onReceiveInterest(*i);
      }
    else if (element.type() == tlv::Data)
      {
        shared_ptr<Data> d = make_shared<Data>();
        d->wireDecode(element);
        this->onReceiveData(*d);
      }
    else
      return false;

    return true;
  }
  catch (const tlv::Error&) {
    return false;
  }
}

void
Face::fail(const std::string& reason)
{
  if (m_isFailed) {
    return;
  }

  m_isFailed = true;
  this->onFail(reason);
}

template<typename FaceTraits>
void
Face::copyStatusTo(FaceTraits& traits) const
{
  traits.setFaceId(getId())
        .setRemoteUri(getRemoteUri().toString())
        .setLocalUri(getLocalUri().toString())
        .setFaceScope(isLocal() ? ndn::nfd::FACE_SCOPE_LOCAL
                                : ndn::nfd::FACE_SCOPE_NON_LOCAL)
        .setFacePersistency(getPersistency())
        .setLinkType(isMultiAccess() ? ndn::nfd::LINK_TYPE_MULTI_ACCESS
                                     : ndn::nfd::LINK_TYPE_POINT_TO_POINT);
}

template void
Face::copyStatusTo<ndn::nfd::FaceStatus>(ndn::nfd::FaceStatus&) const;

template void
Face::copyStatusTo<ndn::nfd::FaceEventNotification>(ndn::nfd::FaceEventNotification&) const;

ndn::nfd::FaceStatus
Face::getFaceStatus() const
{
  ndn::nfd::FaceStatus status;

  copyStatusTo(status);
  this->getCounters().copyTo(status);

  return status;
}

} // namespace nfd
