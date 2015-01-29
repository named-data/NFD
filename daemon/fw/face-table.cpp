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

#include "face-table.hpp"
#include "forwarder.hpp"
#include "core/logger.hpp"

namespace nfd {

NFD_LOG_INIT("FaceTable");

FaceTable::FaceTable(Forwarder& forwarder)
  : m_forwarder(forwarder)
  , m_lastFaceId(FACEID_RESERVED_MAX)
{
}

FaceTable::~FaceTable()
{

}

shared_ptr<Face>
FaceTable::get(FaceId id) const
{
  std::map<FaceId, shared_ptr<Face> >::const_iterator i = m_faces.find(id);
  return (i == m_faces.end()) ? (shared_ptr<Face>()) : (i->second);
}

size_t
FaceTable::size() const
{
  return m_faces.size();
}

void
FaceTable::add(shared_ptr<Face> face)
{
  if (face->getId() != INVALID_FACEID && m_faces.count(face->getId()) > 0) {
    NFD_LOG_WARN("Trying to add existing face id=" << face->getId() << " to the face table");
    return;
  }

  FaceId faceId = ++m_lastFaceId;
  BOOST_ASSERT(faceId > FACEID_RESERVED_MAX);
  this->addImpl(face, faceId);
}

void
FaceTable::addReserved(shared_ptr<Face> face, FaceId faceId)
{
  BOOST_ASSERT(face->getId() == INVALID_FACEID);
  BOOST_ASSERT(m_faces.count(face->getId()) == 0);
  BOOST_ASSERT(faceId <= FACEID_RESERVED_MAX);
  this->addImpl(face, faceId);
}

void
FaceTable::addImpl(shared_ptr<Face> face, FaceId faceId)
{
  face->setId(faceId);
  m_faces[faceId] = face;
  NFD_LOG_INFO("Added face id=" << faceId << " remote=" << face->getRemoteUri()
                                          << " local=" << face->getLocalUri());

  face->onReceiveInterest.connect(bind(&Forwarder::onInterest, &m_forwarder, ref(*face), _1));
  face->onReceiveData.connect(bind(&Forwarder::onData, &m_forwarder, ref(*face), _1));
  face->onFail.connectSingleShot(bind(&FaceTable::remove, this, face, _1));

  this->onAdd(face);
}

void
FaceTable::remove(shared_ptr<Face> face, const std::string& reason)
{
  this->onRemove(face);

  FaceId faceId = face->getId();
  m_faces.erase(faceId);
  face->setId(INVALID_FACEID);

  NFD_LOG_INFO("Removed face id=" << faceId <<
               " remote=" << face->getRemoteUri() <<
               " local=" << face->getLocalUri() <<
               " (" << reason << ")");

  m_forwarder.getFib().removeNextHopFromAllEntries(face);
}

FaceTable::ForwardRange
FaceTable::getForwardRange() const
{
  return m_faces | boost::adaptors::map_values;
}

FaceTable::const_iterator
FaceTable::begin() const
{
  return this->getForwardRange().begin();
}

FaceTable::const_iterator
FaceTable::end() const
{
  return this->getForwardRange().end();
}

} // namespace nfd
