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
 **/

#include "face-query-status-publisher.hpp"

#include <boost/range/adaptor/reversed.hpp>
#include <ndn-cxx/management/nfd-face-status.hpp>

namespace nfd {

FaceQueryStatusPublisher::FaceQueryStatusPublisher(const FaceTable& faceTable,
                                                   AppFace& face,
                                                   const Name& prefix,
                                                   const ndn::nfd::FaceQueryFilter& filter,
                                                   ndn::KeyChain& keyChain)
  : SegmentPublisher(face, prefix, keyChain)
  , m_faceTable(faceTable)
  , m_faceFilter(filter)
{
}

FaceQueryStatusPublisher::~FaceQueryStatusPublisher()
{
}

bool
FaceQueryStatusPublisher::doesMatchFilter(const shared_ptr<Face>& face)
{
  if (m_faceFilter.hasFaceId() &&
      m_faceFilter.getFaceId() != static_cast<uint64_t>(face->getId())) {
    return false;
  }

  if (m_faceFilter.hasUriScheme() &&
      (m_faceFilter.getUriScheme() != face->getRemoteUri().getScheme() ||
       m_faceFilter.getUriScheme() != face->getLocalUri().getScheme())) {
    return false;
  }

  if (m_faceFilter.hasRemoteUri() &&
      m_faceFilter.getRemoteUri() != face->getRemoteUri().toString()) {
    return false;
  }

  if (m_faceFilter.hasLocalUri() && m_faceFilter.getLocalUri() != face->getLocalUri().toString()) {
    return false;
  }

  if (m_faceFilter.hasFaceScope() &&
      (m_faceFilter.getFaceScope() == ndn::nfd::FACE_SCOPE_LOCAL) != face->isLocal()) {
    return false;
  }

  if (m_faceFilter.hasFacePersistency() &&
      m_faceFilter.getFacePersistency() != face->getPersistency()) {
    return false;
  }

  if (m_faceFilter.hasLinkType() &&
      (m_faceFilter.getLinkType() == ndn::nfd::LINK_TYPE_MULTI_ACCESS) != face->isMultiAccess()) {
    return false;
  }

  return true;
}

size_t
FaceQueryStatusPublisher::generate(ndn::EncodingBuffer& outBuffer)
{
  size_t totalLength = 0;

  for (const shared_ptr<Face>& face : m_faceTable | boost::adaptors::reversed) {
    if (doesMatchFilter(face)) {
      ndn::nfd::FaceStatus status = face->getFaceStatus();
      totalLength += status.wireEncode(outBuffer);
    }
  }
  return totalLength;
}

} // namespace nfd
