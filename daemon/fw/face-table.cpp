/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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
#include "core/global-io.hpp"
#include "core/logger.hpp"
#include "face/channel.hpp"

#include <ndn-cxx/util/concepts.hpp>

namespace nfd {

NDN_CXX_ASSERT_FORWARD_ITERATOR(FaceTable::const_iterator);

NFD_LOG_INIT("FaceTable");

FaceTable::FaceTable()
  : m_lastFaceId(face::FACEID_RESERVED_MAX)
{
}

Face*
FaceTable::get(FaceId id) const
{
  auto i = m_faces.find(id);
  return i == m_faces.end() ? nullptr : i->second.get();
}

size_t
FaceTable::size() const
{
  return m_faces.size();
}

void
FaceTable::add(shared_ptr<Face> face)
{
  if (face->getId() != face::INVALID_FACEID && m_faces.count(face->getId()) > 0) {
    NFD_LOG_WARN("Trying to add existing face id=" << face->getId() << " to the face table");
    return;
  }

  FaceId faceId = ++m_lastFaceId;
  BOOST_ASSERT(faceId > face::FACEID_RESERVED_MAX);
  this->addImpl(std::move(face), faceId);
}

void
FaceTable::addReserved(shared_ptr<Face> face, FaceId faceId)
{
  BOOST_ASSERT(face->getId() == face::INVALID_FACEID);
  BOOST_ASSERT(faceId <= face::FACEID_RESERVED_MAX);
  this->addImpl(std::move(face), faceId);
}

void
FaceTable::addImpl(shared_ptr<Face> face, FaceId faceId)
{
  face->setId(faceId);
  auto ret = m_faces.emplace(faceId, face);
  BOOST_VERIFY(ret.second);

  NFD_LOG_INFO("Added face id=" << faceId <<
               " remote=" << face->getRemoteUri() <<
               " local=" << face->getLocalUri());

  connectFaceClosedSignal(*face, bind(&FaceTable::remove, this, faceId));

  this->afterAdd(*face);
}

void
FaceTable::remove(FaceId faceId)
{
  auto i = m_faces.find(faceId);
  BOOST_ASSERT(i != m_faces.end());
  shared_ptr<Face> face = i->second;

  this->beforeRemove(*face);

  m_faces.erase(i);
  face->setId(face::INVALID_FACEID);

  NFD_LOG_INFO("Removed face id=" << faceId <<
               " remote=" << face->getRemoteUri() <<
               " local=" << face->getLocalUri());

  // defer Face deallocation, so that Transport isn't deallocated during afterStateChange signal
  getGlobalIoService().post([face] {});
}

FaceTable::ForwardRange
FaceTable::getForwardRange() const
{
  return m_faces | boost::adaptors::map_values | boost::adaptors::indirected;
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
