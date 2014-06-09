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

#include "rib.hpp"

namespace nfd {
namespace rib {

Rib::Rib()
  : m_nItems(0)
{
}


Rib::~Rib()
{
}

Rib::const_iterator
Rib::find(const Name& prefix) const
{
  return m_rib.find(prefix);
}

shared_ptr<FaceEntry>
Rib::find(const Name& prefix, const FaceEntry& face) const
{
  RibTable::const_iterator ribIt = m_rib.find(prefix);

  // Name prefix exists
  if (ribIt != m_rib.end())
    {
      shared_ptr<RibEntry> entry(ribIt->second);

      RibEntry::const_iterator faceIt = std::find_if(entry->begin(), entry->end(),
                                                     bind(&compareFaceIdAndOrigin, _1, face));

      if (faceIt != entry->end())
        {
          return make_shared<FaceEntry>(*faceIt);
        }
    }

  return shared_ptr<FaceEntry>();
}


void
Rib::insert(const Name& prefix, const FaceEntry& face)
{
  RibTable::iterator ribIt = m_rib.find(prefix);

  // Name prefix exists
  if (ribIt != m_rib.end())
    {
      shared_ptr<RibEntry> entry(ribIt->second);

      RibEntry::iterator faceIt = std::find_if(entry->getFaces().begin(),
                                               entry->getFaces().end(),
                                               bind(&compareFaceIdAndOrigin, _1, face));

      if (faceIt == entry->end())
        {
          // New face
          entry->insertFace(face);
          m_nItems++;

          // Register with face lookup table
          m_faceMap[face.faceId].push_back(entry);
        }
      else
        {
          // Entry exists, update other fields
          faceIt->flags = face.flags;
          faceIt->cost = face.cost;
          faceIt->expires = face.expires;
        }
    }
  else // New name prefix
    {
      shared_ptr<RibEntry> entry(make_shared<RibEntry>(RibEntry()));

      m_rib[prefix] = entry;
      m_nItems++;

      entry->setName(prefix);
      entry->insertFace(face);

      // Find prefix's parent
      shared_ptr<RibEntry> parent = findParent(prefix);

      // Add self to parent's children
      if (static_cast<bool>(parent))
        {
          parent->addChild(entry);
        }

      RibEntryList children = findDescendants(prefix);

      for (std::list<shared_ptr<RibEntry> >::iterator child = children.begin();
           child != children.end(); ++child)
        {
          if ((*child)->getParent() == parent)
            {
              // Remove child from parent and inherit parent's child
              if (static_cast<bool>(parent))
                {
                  parent->removeChild((*child));
                }

              entry->addChild((*child));
            }
        }

      // Register with face lookup table
      m_faceMap[face.faceId].push_back(entry);
    }
}


void
Rib::erase(const Name& prefix, const FaceEntry& face)
{
  RibTable::iterator ribIt = m_rib.find(prefix);

  // Name prefix exists
  if (ribIt != m_rib.end())
    {
      shared_ptr<RibEntry> entry(ribIt->second);

      if (entry->eraseFace(face))
        {
          m_nItems--;

          // If this RibEntry no longer has this faceId, unregister from face lookup table
          if (!entry->hasFaceId(face.faceId))
            {
              m_faceMap[face.faceId].remove(entry);
            }

          // If a RibEntry's facelist is empty, remove it from the tree
          if (entry->getFaces().size() == 0)
            {
              eraseEntry(ribIt);
            }
        }
    }
}

void
Rib::erase(const uint64_t faceId)
{
  FaceLookupTable::iterator lookupIt = m_faceMap.find(faceId);

  // No RIB entries have this face
  if (lookupIt == m_faceMap.end())
    {
      return;
    }

  RibEntryList& ribEntries = lookupIt->second;

  // For each RIB entry that has faceId, remove the face from the entry
  for (RibEntryList::iterator entryIt = ribEntries.begin(); entryIt != ribEntries.end(); ++entryIt)
    {
      shared_ptr<RibEntry> entry = *entryIt;

      // Find the faces in the entry
      for (RibEntry::iterator faceIt = entry->begin(); faceIt != entry->end(); ++faceIt)
        {
          if (faceIt->faceId == faceId)
            {
              faceIt = entry->eraseFace(faceIt);
            }
        }

        // If a RibEntry's facelist is empty, remove it from the tree
        if (entry->getFaces().size() == 0)
          {
            eraseEntry(m_rib.find(entry->getName()));
          }
    }

  // Face no longer exists, remove from face lookup table
  FaceLookupTable::iterator entryToDelete = m_faceMap.find(faceId);

  if (entryToDelete != m_faceMap.end())
    {
      m_faceMap.erase(entryToDelete);
    }
}

shared_ptr<RibEntry>
Rib::findParent(const Name& prefix) const
{
  for (int i = prefix.size() - 1; i >= 0; i--)
    {
      RibTable::const_iterator it = m_rib.find(prefix.getPrefix(i));
      if (it != m_rib.end())
        {
          return (it->second);
        }
    }

  return shared_ptr<RibEntry>();
}

std::list<shared_ptr<RibEntry> >
Rib::findDescendants(const Name& prefix) const
{
  std::list<shared_ptr<RibEntry> > children;

  RibTable::const_iterator it = m_rib.find(prefix);

  if (it != m_rib.end())
    {
      ++it;
      for (; it != m_rib.end(); ++it)
        {
          if (prefix.isPrefixOf(it->first))
            {
              children.push_back((it->second));
            }
          else
            {
              break;
            }
        }
    }

  return children;
}

Rib::RibTable::iterator
Rib::eraseEntry(RibTable::iterator it)
{
  // Entry does not exist
  if (it == m_rib.end())
    {
      return m_rib.end();
    }

  shared_ptr<RibEntry> entry(it->second);

  shared_ptr<RibEntry> parent = entry->getParent();

  // Remove self from parent's children
  if (static_cast<bool>(parent))
    {
      parent->removeChild(entry);
    }

  std::list<shared_ptr<RibEntry> > children = entry->getChildren();

  for (RibEntryList::iterator child = children.begin(); child != children.end(); ++child)
    {
      // Remove children from self
      entry->removeChild(*child);

      // Update parent's children
      if (static_cast<bool>(parent))
        {
          parent->addChild(*child);
        }
    }

  // Must save and advance iterator to return a valid iterator
  RibTable::iterator nextIt = it;
  nextIt++;

  m_rib.erase(it);

  return nextIt;
}

std::ostream&
operator<<(std::ostream& os, const Rib& rib)
{
  for (Rib::RibTable::const_iterator it = rib.begin(); it != rib.end(); ++it)
    {
      os << *(it->second) << "\n";
    }

  return os;
}

} // namespace rib
} // namespace nfd
