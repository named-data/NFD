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

#include "core/logger.hpp"

NFD_LOG_INIT("Rib");

namespace nfd {
namespace rib {

static inline bool
sortFace(const FaceEntry& entry1, const FaceEntry& entry2)
{
  return entry1.faceId < entry2.faceId;
}

static inline bool
isChildInheritFlagSet(uint64_t flags)
{
  return flags & ndn::nfd::ROUTE_FLAG_CHILD_INHERIT;
}

static inline bool
isCaptureFlagSet(uint64_t flags)
{
  return flags & ndn::nfd::ROUTE_FLAG_CAPTURE;
}

static inline bool
isAnyFlagSet(uint64_t flags)
{
  return isChildInheritFlagSet(flags) || isCaptureFlagSet(flags);
}

static inline bool
areBothFlagsSet(uint64_t flags)
{
  return isChildInheritFlagSet(flags) && isCaptureFlagSet(flags);
}

Rib::Rib()
  : m_nItems(0)
{
}

Rib::const_iterator
Rib::find(const Name& prefix) const
{
  return m_rib.find(prefix);
}

FaceEntry*
Rib::find(const Name& prefix, const FaceEntry& face) const
{
  RibTable::const_iterator ribIt = m_rib.find(prefix);

  // Name prefix exists
  if (ribIt != m_rib.end())
    {
      shared_ptr<RibEntry> entry(ribIt->second);

      RibEntry::iterator faceIt = std::find_if(entry->begin(), entry->end(),
                                                     bind(&compareFaceIdAndOrigin, _1, face));

      if (faceIt != entry->end())
        {
          return &((*faceIt));
        }
    }
  return 0;
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
          // Will the new face change the namespace's capture flag?
          bool captureWasTurnedOn = (entry->hasCapture() == false && isCaptureFlagSet(face.flags));

          // New face
          entry->insertFace(face);
          m_nItems++;

          // Register with face lookup table
          m_faceMap[face.faceId].push_back(entry);

          createFibUpdatesForNewFaceEntry(*entry, face, captureWasTurnedOn);
        }
      else // Entry exists, update fields
        {
          // First cancel old scheduled event, if any, then set the EventId to new one
          if (static_cast<bool>(faceIt->getExpirationEvent()))
            {
              NFD_LOG_TRACE("Cancelling expiration event for " << entry->getName() << " "
                                                               << *faceIt);
              scheduler::cancel(faceIt->getExpirationEvent());
            }

          // No checks are required here as the iterator needs to be updated in all cases.
          faceIt->setExpirationEvent(face.getExpirationEvent());

          // Save flags for update processing
          uint64_t previousFlags = faceIt->flags;

          // If the entry's cost didn't change and child inherit is not set,
          // no need to traverse subtree.
          uint64_t previousCost = faceIt->cost;

          faceIt->flags = face.flags;
          faceIt->cost = face.cost;
          faceIt->expires = face.expires;

          createFibUpdatesForUpdatedEntry(*entry, face, previousFlags, previousCost);
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

      createFibUpdatesForNewRibEntry(*entry, face);

      // do something after inserting an entry
      afterInsertEntry(prefix);
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

      const bool hadCapture = entry->hasCapture();

      // Need to copy face to do FIB updates with correct cost and flags since nfdc does not
      // pass flags or cost
      RibEntry::iterator faceIt = entry->findFace(face);

      if (faceIt != entry->end())
        {
          FaceEntry faceToErase = *faceIt;
          faceToErase.flags = faceIt->flags;
          faceToErase.cost = faceIt->cost;

          entry->eraseFace(faceIt);

          m_nItems--;

          const bool captureWasTurnedOff = (hadCapture && !entry->hasCapture());

          createFibUpdatesForErasedFaceEntry(*entry, faceToErase, captureWasTurnedOff);

          // If this RibEntry no longer has this faceId, unregister from face lookup table
          if (!entry->hasFaceId(face.faceId))
            {
              m_faceMap[face.faceId].remove(entry);
            }
          else
            {
              // The RibEntry still has the face ID; need to update FIB
              // with lowest cost for the same face instead of removing the face from the FIB
              shared_ptr<FaceEntry> lowCostFace = entry->getFaceWithLowestCostByFaceId(face.faceId);

              BOOST_ASSERT(static_cast<bool>(lowCostFace));

              createFibUpdatesForNewFaceEntry(*entry, *lowCostFace, false);
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

      const bool hadCapture = entry->hasCapture();

      // Find the faces in the entry
      for (RibEntry::iterator faceIt = entry->begin(); faceIt != entry->end(); ++faceIt)
        {
          if (faceIt->faceId == faceId)
            {
              FaceEntry copy = *faceIt;

              faceIt = entry->eraseFace(faceIt);
              m_nItems--;

              const bool captureWasTurnedOff = (hadCapture && !entry->hasCapture());
              createFibUpdatesForErasedFaceEntry(*entry, copy, captureWasTurnedOff);
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

  // Remove inherited routes from namespace
  createFibUpdatesForErasedRibEntry(*entry);

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

  // do something after erasing an entry.
  afterEraseEntry(entry->getName());

  return nextIt;
}

bool
compareFibUpdates(const shared_ptr<const FibUpdate> lhs, const shared_ptr<const FibUpdate> rhs)
{
  return ((lhs->name == rhs->name) &&
          (lhs->faceId == rhs->faceId));
}

void
Rib::insertFibUpdate(shared_ptr<FibUpdate> update)
{
  // If an update with the same name and face already exists,
  // replace it
  FibUpdateList::iterator it = std::find_if(m_fibUpdateList.begin(), m_fibUpdateList.end(),
                                            bind(&compareFibUpdates, _1, update));

  if (it != m_fibUpdateList.end())
    {
      // Get rid of the const to alter the action, prevents copying or removal and
      // insertion
      FibUpdate& entry = const_cast<FibUpdate&>(*(*it));
      entry.action = update->action;
      entry.cost = update->cost;
    }
  else
    {
      m_fibUpdateList.push_back(update);
    }
}

void
Rib::createFibUpdatesForNewRibEntry(RibEntry& entry, const FaceEntry& face)
{
  // Create FIB update for new entry
  insertFibUpdate(FibUpdate::createAddUpdate(entry.getName(), face.faceId, face.cost));

  // No flags are set
  if (!isAnyFlagSet(face.flags))
    {
      // Add ancestor faces to self
      addInheritedFacesToEntry(entry, getAncestorFaces(entry));
    }
  else if (areBothFlagsSet(face.flags))
    {
      // Add face to children
      FaceSet facesToAdd;
      facesToAdd.insert(face);

      // Remove faces blocked by capture and add self to children
      modifyChildrensInheritedFaces(entry, facesToAdd, getAncestorFaces(entry));
    }
  else if (isChildInheritFlagSet(face.flags))
    {
      FaceSet ancestorFaces = getAncestorFaces(entry);

      // Add ancestor faces to self
      addInheritedFacesToEntry(entry, ancestorFaces);

      // If there is an ancestor face which is the same as the new face, replace it
      // with the new face
      FaceSet::iterator it = ancestorFaces.find(face);

      // There is a face that needs to be overwritten, erase and then replace
      if (it != ancestorFaces.end())
        {
          ancestorFaces.erase(it);
        }

      // Add new face to ancestor list so it can be added to children
      ancestorFaces.insert(face);

      // Add ancestor faces to children
      modifyChildrensInheritedFaces(entry, ancestorFaces, FaceSet());
    }
  else if (isCaptureFlagSet(face.flags))
    {
      // Remove faces blocked by capture
      modifyChildrensInheritedFaces(entry, FaceSet(), getAncestorFaces(entry));
    }
}

void
Rib::createFibUpdatesForNewFaceEntry(RibEntry& entry, const FaceEntry& face,
                                     bool captureWasTurnedOn)
{
  // Only update if the new face has a lower cost than a previously installed face
  shared_ptr<FaceEntry> prevFace = entry.getFaceWithLowestCostAndChildInheritByFaceId(face.faceId);

  FaceSet facesToAdd;
  if (isChildInheritFlagSet(face.flags))
    {
      // Add to children if this new face doesn't override a previous lower cost, or
      // add to children if this new is lower cost than a previous face.
      // Less than equal, since entry may find this face
      if (!static_cast<bool>(prevFace) || face.cost <= prevFace->cost)
        {
          // Add self to children
          facesToAdd.insert(face);
        }
    }

  FaceSet facesToRemove;
  if (captureWasTurnedOn)
    {
      // Capture flag on
      facesToRemove = getAncestorFaces(entry);

      // Remove ancestor faces from self
      removeInheritedFacesFromEntry(entry, facesToRemove);
    }

  modifyChildrensInheritedFaces(entry, facesToAdd, facesToRemove);

  // If another face with same faceId and lower cost, don't update.
  // Must be done last so that add updates replace removal updates
  // Create FIB update for new entry
  if (face.cost <= entry.getFaceWithLowestCostByFaceId(face.faceId)->cost)
    {
      insertFibUpdate(FibUpdate::createAddUpdate(entry.getName(), face.faceId, face.cost));
    }
}

void
Rib::createFibUpdatesForUpdatedEntry(RibEntry& entry, const FaceEntry& face,
                                     const uint64_t previousFlags, const uint64_t previousCost)
{
  const bool costDidChange = (face.cost != previousCost);

  // Look for an installed face with the lowest cost and child inherit set
  shared_ptr<FaceEntry> prevFace = entry.getFaceWithLowestCostAndChildInheritByFaceId(face.faceId);

  // No flags changed and cost didn't change, no change in FIB
  if (face.flags == previousFlags && !costDidChange)
    {
      return;
    }

  // Cost changed so create update for the entry itself
  if (costDidChange)
    {
      // Create update if this face's cost is lower than other faces
       if (face.cost <= entry.getFaceWithLowestCostByFaceId(face.faceId)->cost)
        {
          // Create FIB update for the updated entry
         insertFibUpdate(FibUpdate::createAddUpdate(entry.getName(), face.faceId, face.cost));
        }
      else if (previousCost < entry.getFaceWithLowestCostByFaceId(face.faceId)->cost)
        {
          // Create update if this face used to be the lowest face but is no longer
          // the lowest cost face.
          insertFibUpdate(FibUpdate::createAddUpdate(entry.getName(), prevFace->faceId,
                                                                          prevFace->cost));
        }

      // If another face with same faceId and lower cost and ChildInherit exists,
      // don't update children.
      if (!static_cast<bool>(prevFace) || face.cost <= prevFace->cost)
        {
          // If no flags changed but child inheritance is set, need to update children
          // with new cost
          if ((face.flags == previousFlags) && isChildInheritFlagSet(face.flags))
          {
            // Add self to children
            FaceSet facesToAdd;
            facesToAdd.insert(face);
            modifyChildrensInheritedFaces(entry, facesToAdd, FaceSet());

            return;
          }
        }
    }

  // Child inherit was turned on
  if (!isChildInheritFlagSet(previousFlags) && isChildInheritFlagSet(face.flags))
    {
      // If another face with same faceId and lower cost and ChildInherit exists,
      // don't update children.
      if (!static_cast<bool>(prevFace) || face.cost <= prevFace->cost)
        {
          // Add self to children
          FaceSet facesToAdd;
          facesToAdd.insert(face);
          modifyChildrensInheritedFaces(entry, facesToAdd, FaceSet());
        }

    } // Child inherit was turned off
  else if (isChildInheritFlagSet(previousFlags) && !isChildInheritFlagSet(face.flags))
    {
      // Remove self from children
      FaceSet facesToRemove;
      facesToRemove.insert(face);

      FaceSet facesToAdd;
      // If another face with same faceId and ChildInherit exists, update children with this face.
      if (static_cast<bool>(prevFace))
        {
          facesToAdd.insert(*prevFace);
        }
      else
        {
          // Look for an ancestor that was blocked previously
          const FaceSet ancestorFaces = getAncestorFaces(entry);
          FaceSet::iterator it = ancestorFaces.find(face);

          // If an ancestor is found, add it to children
          if (it != ancestorFaces.end())
            {
              facesToAdd.insert(*it);
            }
        }

      modifyChildrensInheritedFaces(entry, facesToAdd, facesToRemove);
    }

  // Capture was turned on
  if (!isCaptureFlagSet(previousFlags) && isCaptureFlagSet(face.flags))
    {
      FaceSet ancestorFaces = getAncestorFaces(entry);

      // Remove ancestor faces from self
      removeInheritedFacesFromEntry(entry, ancestorFaces);

      // Remove ancestor faces from children
      modifyChildrensInheritedFaces(entry, FaceSet(), ancestorFaces);
    }  // Capture was turned off
  else if (isCaptureFlagSet(previousFlags) && !isCaptureFlagSet(face.flags))
    {
      FaceSet ancestorFaces = getAncestorFaces(entry);

      // Add ancestor faces to self
      addInheritedFacesToEntry(entry, ancestorFaces);

      // Add ancestor faces to children
      modifyChildrensInheritedFaces(entry, ancestorFaces, FaceSet());
    }
}

void
Rib::createFibUpdatesForErasedFaceEntry(RibEntry& entry, const FaceEntry& face,
                                        const bool captureWasTurnedOff)
{
  insertFibUpdate(FibUpdate::createRemoveUpdate(entry.getName(), face.faceId));

  if (areBothFlagsSet(face.flags))
    {
      // Remove self from children
      FaceSet facesToRemove;
      facesToRemove.insert(face);

      // If capture is turned off for the route, need to add ancestors
      // to self and children
      FaceSet facesToAdd;
      if (captureWasTurnedOff)
        {
          // Look for an ancestors that were blocked previously
          facesToAdd = getAncestorFaces(entry);

          // Add ancestor faces to self
          addInheritedFacesToEntry(entry, facesToAdd);
        }

      modifyChildrensInheritedFaces(entry, facesToAdd, facesToRemove);
    }
  else if (isChildInheritFlagSet(face.flags))
    {
      // If not blocked by capture, add inherited routes to children
      FaceSet facesToAdd;
      if (!entry.hasCapture())
        {
          facesToAdd = getAncestorFaces(entry);
        }

      FaceSet facesToRemove;
      facesToRemove.insert(face);

      // Add ancestor faces to children
      modifyChildrensInheritedFaces(entry, facesToAdd, facesToRemove);
    }
  else if (isCaptureFlagSet(face.flags))
    {
      // If capture is turned off for the route, need to add ancestors
      // to self and children
      FaceSet facesToAdd;
      if (captureWasTurnedOff)
        {
          // Look for an ancestors that were blocked previously
          facesToAdd = getAncestorFaces(entry);

          // Add ancestor faces to self
          addInheritedFacesToEntry(entry, facesToAdd);
        }

      modifyChildrensInheritedFaces(entry, facesToAdd, FaceSet());
    }

  // Need to check if the removed face was blocking an inherited route
  FaceSet ancestorFaces = getAncestorFaces(entry);

  if (!entry.hasCapture())
  {
    // If there is an ancestor face which is the same as the erased face, add that face
    // to the current entry
    FaceSet::iterator it = ancestorFaces.find(face);

    if (it != ancestorFaces.end())
      {
        entry.addInheritedFace(*it);
        insertFibUpdate(FibUpdate::createAddUpdate(entry.getName(), it->faceId, it->cost));
      }
  }
}

void
Rib::createFibUpdatesForErasedRibEntry(RibEntry& entry)
{
  for (RibEntry::FaceList::iterator it = entry.getInheritedFaces().begin();
       it != entry.getInheritedFaces().end(); ++it)
    {
      insertFibUpdate(FibUpdate::createRemoveUpdate(entry.getName(), it->faceId));
    }
}

Rib::FaceSet
Rib::getAncestorFaces(const RibEntry& entry) const
{
  FaceSet ancestorFaces(&sortFace);

  shared_ptr<RibEntry> parent = entry.getParent();

  while (static_cast<bool>(parent))
    {
      for (RibEntry::iterator it = parent->getFaces().begin();
           it != parent->getFaces().end(); ++it)
        {
          if (isChildInheritFlagSet(it->flags))
            {
              ancestorFaces.insert(*it);
            }
        }

      if (parent->hasCapture())
        {
          break;
        }

      parent = parent->getParent();
    }

    return ancestorFaces;
}

void
Rib::addInheritedFacesToEntry(RibEntry& entry, const Rib::FaceSet& facesToAdd)
{
  for (FaceSet::const_iterator it = facesToAdd.begin(); it != facesToAdd.end(); ++it)
    {
      // Don't add an ancestor faceId if the namespace has an entry for that faceId
      if (!entry.hasFaceId(it->faceId))
        {
          entry.addInheritedFace(*it);
          insertFibUpdate(FibUpdate::createAddUpdate(entry.getName(), it->faceId, it->cost));
        }
    }
}

void
Rib::removeInheritedFacesFromEntry(RibEntry& entry, const Rib::FaceSet& facesToRemove)
{
  for (FaceSet::const_iterator it = facesToRemove.begin(); it != facesToRemove.end(); ++it)
    {
      // Only remove if the face has been inherited
      if (entry.hasInheritedFace(*it))
        {
          entry.removeInheritedFace(*it);
          insertFibUpdate(FibUpdate::createRemoveUpdate(entry.getName(), it->faceId));
        }
    }
}

void
Rib::modifyChildrensInheritedFaces(RibEntry& entry, const Rib::FaceSet& facesToAdd,
                                                    const Rib::FaceSet& facesToRemove)
{
  RibEntryList children = entry.getChildren();

  for (RibEntryList::iterator child = children.begin(); child != children.end(); ++child)
    {
      traverseSubTree(*(*child), facesToAdd, facesToRemove);
    }
}

void
Rib::traverseSubTree(RibEntry& entry, Rib::FaceSet facesToAdd,
                                      Rib::FaceSet facesToRemove)
{
  // If a route on the namespace has the capture flag set, ignore self and children
  if (entry.hasCapture())
    {
      return;
    }

  // Remove inherited faces from current namespace
  for (Rib::FaceSet::const_iterator removeIt = facesToRemove.begin();
       removeIt != facesToRemove.end(); )
    {
      // If a route on the namespace has the same face and child inheritance set, ignore this face
      if (entry.hasChildInheritOnFaceId(removeIt->faceId))
        {
          facesToRemove.erase(removeIt++);
          continue;
        }

      // Only remove face if it removes an existing inherited route
      if (entry.hasInheritedFace(*removeIt))
        {
          entry.removeInheritedFace(*removeIt);
          insertFibUpdate(FibUpdate::createRemoveUpdate(entry.getName(), removeIt->faceId));
        }

      ++removeIt;
    }

  // Add inherited faces to current namespace
  for (Rib::FaceSet::const_iterator addIt = facesToAdd.begin();
       addIt != facesToAdd.end(); )
    {
      // If a route on the namespace has the same face and child inherit set, ignore this face
      if (entry.hasChildInheritOnFaceId(addIt->faceId))
      {
        facesToAdd.erase(addIt++);
        continue;
      }

      // Only add face if it does not override an existing route
      if (!entry.hasFaceId(addIt->faceId))
        {
          RibEntry::FaceList::iterator faceIt = entry.findInheritedFace(*addIt);

          // If the entry already has the inherited face, just update the face
          if (faceIt != entry.getInheritedFaces().end())
            {
              faceIt->cost = addIt->cost;
            }
          else // Otherwise, this is a newly inherited face
            {
              entry.addInheritedFace(*addIt);
            }

          insertFibUpdate(FibUpdate::createAddUpdate(entry.getName(), addIt->faceId, addIt->cost));
        }

      ++addIt;
    }

  Rib::RibEntryList children = entry.getChildren();

  // Apply face operations to current namespace's children
  for (Rib::RibEntryList::iterator child = children.begin(); child != children.end(); ++child)
    {
      traverseSubTree(*(*child), facesToAdd, facesToRemove);
    }
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
