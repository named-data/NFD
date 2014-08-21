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

#include "rib-entry.hpp"

#include "core/logger.hpp"

#include <ndn-cxx/management/nfd-control-command.hpp>

NFD_LOG_INIT("RibEntry");

namespace nfd {
namespace rib {

RibEntry::FaceList::iterator
RibEntry::findFace(const FaceEntry& face)
{
  return std::find_if(begin(), end(), bind(&compareFaceIdAndOrigin, _1, face));
}

bool
RibEntry::insertFace(const FaceEntry& entry)
{
  iterator it = findFace(entry);

  if (it == end())
    {
      if (entry.flags & ndn::nfd::ROUTE_FLAG_CAPTURE)
        {
          m_nFacesWithCaptureSet++;
        }

      m_faces.push_back(entry);
      return true;
    }
  else
    {
      return false;
    }
}

void
RibEntry::eraseFace(const FaceEntry& face)
{
  RibEntry::iterator it = std::find_if(begin(), end(), bind(&compareFaceIdAndOrigin, _1, face));
  eraseFace(it);
}

bool
RibEntry::hasFace(const FaceEntry& face)
{
  RibEntry::const_iterator it = std::find_if(cbegin(), cend(),
                                             bind(&compareFaceIdAndOrigin, _1, face));

  return it != cend();
}

bool
RibEntry::hasFaceId(const uint64_t faceId) const
{
  RibEntry::const_iterator it = std::find_if(cbegin(), cend(), bind(&compareFaceId, _1, faceId));

  return it != cend();
}

void
RibEntry::addChild(shared_ptr<RibEntry> child)
{
  BOOST_ASSERT(!static_cast<bool>(child->getParent()));
  child->setParent(this->shared_from_this());
  m_children.push_back(child);
}

void
RibEntry::removeChild(shared_ptr<RibEntry> child)
{
  BOOST_ASSERT(child->getParent().get() == this);
  child->setParent(shared_ptr<RibEntry>());
  m_children.remove(child);
}

RibEntry::FaceList::iterator
RibEntry::eraseFace(FaceList::iterator face)
{
  if (face != m_faces.end())
    {
      if (face->flags & ndn::nfd::ROUTE_FLAG_CAPTURE)
        {
          m_nFacesWithCaptureSet--;
        }

      //cancel any scheduled event
      NFD_LOG_TRACE("Cancelling expiration eventId: " << face->getExpirationEvent());
      scheduler::cancel(face->getExpirationEvent());

      return m_faces.erase(face);
    }

  return m_faces.end();
}

void
RibEntry::addInheritedFace(const FaceEntry& face)
{
  m_inheritedFaces.push_back(face);
}

void
RibEntry::removeInheritedFace(const FaceEntry& face)
{
  FaceList::iterator it = findInheritedFace(face);
  m_inheritedFaces.erase(it);
}

RibEntry::FaceList::iterator
RibEntry::findInheritedFace(const FaceEntry& face)
{
  return std::find_if(m_inheritedFaces.begin(), m_inheritedFaces.end(),
                      bind(&compareFaceId, _1, face.faceId));
}

bool
RibEntry::hasInheritedFace(const FaceEntry& face)
{
  FaceList::const_iterator it = findInheritedFace(face);

  return (it != m_inheritedFaces.end());
}

bool
RibEntry::hasCapture() const
{
  return m_nFacesWithCaptureSet > 0;
}

bool
RibEntry::hasChildInheritOnFaceId(uint64_t faceId) const
{
  for (RibEntry::const_iterator it = m_faces.begin(); it != m_faces.end(); ++it)
    {
      if (it->faceId == faceId && (it->flags & ndn::nfd::ROUTE_FLAG_CHILD_INHERIT))
        {
          return true;
        }
    }

  return false;
}

shared_ptr<FaceEntry>
RibEntry::getFaceWithLowestCostByFaceId(uint64_t faceId)
{
  shared_ptr<FaceEntry> face;

  for (FaceList::iterator it = begin(); it != end(); ++it)
    {
      // Correct face ID
      if (it->faceId == faceId)
        {
          // If this is the first face with this ID found
          if (!static_cast<bool>(face))
            {
              face = make_shared<FaceEntry>(*it);
            }
          else if (it->cost < face->cost) // Found a face with a lower cost
            {
              face = make_shared<FaceEntry>(*it);
            }
        }
    }

    return face;
}

shared_ptr<FaceEntry>
RibEntry::getFaceWithLowestCostAndChildInheritByFaceId(uint64_t faceId)
{
  shared_ptr<FaceEntry> face;

  for (FaceList::iterator it = begin(); it != end(); ++it)
    {
      // Correct face ID and Child Inherit flag set
      if (it->faceId == faceId && it->flags & ndn::nfd::ROUTE_FLAG_CHILD_INHERIT)
        {
          // If this is the first face with this ID found
          if (!static_cast<bool>(face))
            {
              face = make_shared<FaceEntry>(*it);
            }
          else if (it->cost < face->cost) // Found a face with a lower cost
            {
              face = make_shared<FaceEntry>(*it);
            }
        }
    }

    return face;
}

std::ostream&
operator<<(std::ostream& os, const FaceEntry& entry)
{
  os << "FaceEntry("
     << "faceid: " << entry.faceId
     << ", origin: " << entry.origin
     << ", cost: " << entry.cost
     << ", flags: " << entry.flags;
  if (entry.expires != time::steady_clock::TimePoint::max()) {
    os << ", expires in: " << (entry.expires - time::steady_clock::now());
  }
  else {
    os << ", never expires";
  }
  os << ")";

  return os;
}

std::ostream&
operator<<(std::ostream& os, const RibEntry& entry)
{
  os << "RibEntry {\n";
  os << "\tName: " << entry.getName() << "\n";

  for (RibEntry::FaceList::const_iterator faceIt = entry.cbegin(); faceIt != entry.cend(); ++faceIt)
    {
      os << "\t" << (*faceIt) << "\n";
    }

  os << "}";

  return os;
}

} // namespace rib
} // namespace nfd
