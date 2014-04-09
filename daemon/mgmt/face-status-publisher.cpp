/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
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

#include "face-status-publisher.hpp"
#include "face-flags.hpp"
#include "core/logger.hpp"
#include "fw/face-table.hpp"

#include <ndn-cpp-dev/management/nfd-face-status.hpp>

namespace nfd {

NFD_LOG_INIT("FaceStatusPublisher");


FaceStatusPublisher::FaceStatusPublisher(const FaceTable& faceTable,
                                         shared_ptr<AppFace> face,
                                         const Name& prefix)
  : SegmentPublisher(face, prefix)
  , m_faceTable(faceTable)
{

}


FaceStatusPublisher::~FaceStatusPublisher()
{

}

size_t
FaceStatusPublisher::generate(ndn::EncodingBuffer& outBuffer)
{
  size_t totalLength = 0;

  for (FaceTable::const_reverse_iterator i = m_faceTable.rbegin();
       i != m_faceTable.rend(); ++i) {
    const shared_ptr<Face>& face = *i;
    const FaceCounters& counters = face->getCounters();

    ndn::nfd::FaceStatus status;
    status.setFaceId(face->getId())
          .setRemoteUri(face->getRemoteUri().toString())
          .setLocalUri(face->getLocalUri().toString())
          .setFlags(getFaceFlags(*face))
          .setNInInterests(counters.getNInInterests())
          .setNInDatas(counters.getNInDatas())
          .setNOutInterests(counters.getNOutInterests())
          .setNOutDatas(counters.getNOutDatas());

    totalLength += status.wireEncode(outBuffer);
  }
  return totalLength;
}

} // namespace nfd
