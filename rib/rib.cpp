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

#include "rib.hpp"

#include "core/logger.hpp"

NFD_LOG_INIT("Rib");

namespace nfd {
namespace rib {

static inline bool
sortRoutes(const Route& lhs, const Route& rhs)
{
  return lhs.faceId < rhs.faceId;
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

Route*
Rib::find(const Name& prefix, const Route& route) const
{
  RibTable::const_iterator ribIt = m_rib.find(prefix);

  // Name prefix exists
  if (ribIt != m_rib.end())
    {
      shared_ptr<RibEntry> entry = ribIt->second;

      RibEntry::iterator routeIt = entry->findRoute(route);

      if (routeIt != entry->end())
        {
          return &((*routeIt));
        }
    }

  return nullptr;
}

void
Rib::insert(const Name& prefix, const Route& route)
{
  RibTable::iterator ribIt = m_rib.find(prefix);

  // Name prefix exists
  if (ribIt != m_rib.end())
    {
      shared_ptr<RibEntry> entry(ribIt->second);

      RibEntry::iterator routeIt = entry->findRoute(route);

      if (routeIt == entry->end())
        {
          // Will the new route change the namespace's capture flag?
          bool captureWasTurnedOn = (entry->hasCapture() == false && isCaptureFlagSet(route.flags));

          // New route
          entry->insertRoute(route);
          m_nItems++;

          // Register with face lookup table
          m_faceMap[route.faceId].push_back(entry);

          createFibUpdatesForNewRoute(*entry, route, captureWasTurnedOn);
        }
      else // Route exists, update fields
        {
          // First cancel old scheduled event, if any, then set the EventId to new one
          if (static_cast<bool>(routeIt->getExpirationEvent()))
            {
              NFD_LOG_TRACE("Cancelling expiration event for " << entry->getName() << " "
                                                               << *routeIt);
              scheduler::cancel(routeIt->getExpirationEvent());
            }

          // No checks are required here as the iterator needs to be updated in all cases.
          routeIt->setExpirationEvent(route.getExpirationEvent());

          // Save flags for update processing
          uint64_t previousFlags = routeIt->flags;

          // If the route's cost didn't change and child inherit is not set,
          // no need to traverse subtree.
          uint64_t previousCost = routeIt->cost;

          routeIt->flags = route.flags;
          routeIt->cost = route.cost;
          routeIt->expires = route.expires;

          createFibUpdatesForUpdatedRoute(*entry, route, previousFlags, previousCost);
        }
    }
  else // New name prefix
    {
      shared_ptr<RibEntry> entry(make_shared<RibEntry>(RibEntry()));

      m_rib[prefix] = entry;
      m_nItems++;

      entry->setName(prefix);
      entry->insertRoute(route);

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
      m_faceMap[route.faceId].push_back(entry);

      createFibUpdatesForNewRibEntry(*entry, route);

      // do something after inserting an entry
      afterInsertEntry(prefix);
    }
}

void
Rib::erase(const Name& prefix, const Route& route)
{
  RibTable::iterator ribIt = m_rib.find(prefix);

  // Name prefix exists
  if (ribIt != m_rib.end())
    {
      shared_ptr<RibEntry> entry(ribIt->second);

      const bool hadCapture = entry->hasCapture();

      // Need to copy route to do FIB updates with correct cost and flags since nfdc does not
      // pass flags or cost
      RibEntry::iterator routeIt = entry->findRoute(route);

      if (routeIt != entry->end())
        {
          Route routeToErase = *routeIt;
          routeToErase.flags = routeIt->flags;
          routeToErase.cost = routeIt->cost;

          entry->eraseRoute(routeIt);

          m_nItems--;

          const bool captureWasTurnedOff = (hadCapture && !entry->hasCapture());

          createFibUpdatesForErasedRoute(*entry, routeToErase, captureWasTurnedOff);

          // If this RibEntry no longer has this faceId, unregister from face lookup table
          if (!entry->hasFaceId(route.faceId))
            {
              m_faceMap[route.faceId].remove(entry);
            }
          else
            {
              // The RibEntry still has the face ID; need to update FIB
              // with lowest cost for the same route instead of removing the route from the FIB
              shared_ptr<Route> lowCostRoute = entry->getRouteWithLowestCostByFaceId(route.faceId);

              BOOST_ASSERT(static_cast<bool>(lowCostRoute));

              createFibUpdatesForNewRoute(*entry, *lowCostRoute, false);
            }

          // If a RibEntry's route list is empty, remove it from the tree
          if (entry->getRoutes().size() == 0)
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
  for (shared_ptr<RibEntry>& entry : ribEntries)
    {
      const bool hadCapture = entry->hasCapture();

      // Find the routes in the entry
      for (RibEntry::iterator routeIt = entry->begin(); routeIt != entry->end(); ++routeIt)
        {
          if (routeIt->faceId == faceId)
            {
              Route copy = *routeIt;

              routeIt = entry->eraseRoute(routeIt);
              m_nItems--;

              const bool captureWasTurnedOff = (hadCapture && !entry->hasCapture());
              createFibUpdatesForErasedRoute(*entry, copy, captureWasTurnedOff);
            }
        }

        // If a RibEntry's route list is empty, remove it from the tree
        if (entry->getRoutes().size() == 0)
          {
            eraseEntry(m_rib.find(entry->getName()));
          }
    }

  // Face no longer exists, remove from face lookup table
  m_faceMap.erase(lookupIt);
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
  // If an update with the same name and Face ID already exists, replace it
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
Rib::createFibUpdatesForNewRibEntry(RibEntry& entry, const Route& route)
{
  // Create FIB update for new entry
  insertFibUpdate(FibUpdate::createAddUpdate(entry.getName(), route.faceId, route.cost));

  // No flags are set
  if (!isAnyFlagSet(route.flags))
    {
      // Add ancestor routes to self
      addInheritedRoutesToEntry(entry, getAncestorRoutes(entry));
    }
  else if (areBothFlagsSet(route.flags))
    {
      // Add route to children
      RouteSet routesToAdd;
      routesToAdd.insert(route);

      // Remove routes blocked by capture and add self to children
      modifyChildrensInheritedRoutes(entry, routesToAdd, getAncestorRoutes(entry));
    }
  else if (isChildInheritFlagSet(route.flags))
    {
      RouteSet ancestorRoutes = getAncestorRoutes(entry);

      // Add ancestor routes to self
      addInheritedRoutesToEntry(entry, ancestorRoutes);

      // If there is an ancestor route with the same Face ID as the new route, replace it
      // with the new route
      RouteSet::iterator it = ancestorRoutes.find(route);

      // There is a route that needs to be overwritten, erase and then replace
      if (it != ancestorRoutes.end())
        {
          ancestorRoutes.erase(it);
        }

      // Add new route to ancestor list so it can be added to children
      ancestorRoutes.insert(route);

      // Add ancestor routes to children
      modifyChildrensInheritedRoutes(entry, ancestorRoutes, RouteSet());
    }
  else if (isCaptureFlagSet(route.flags))
    {
      // Remove routes blocked by capture
      modifyChildrensInheritedRoutes(entry, RouteSet(), getAncestorRoutes(entry));
    }
}

void
Rib::createFibUpdatesForNewRoute(RibEntry& entry, const Route& route, bool captureWasTurnedOn)
{
  // Only update if the new route has a lower cost than a previously installed route
  shared_ptr<Route> prevRoute = entry.getRouteWithLowestCostAndChildInheritByFaceId(route.faceId);

  RouteSet routesToAdd;
  if (isChildInheritFlagSet(route.flags))
    {
      // Add to children if this new route doesn't override a previous lower cost, or
      // add to children if this new route is lower cost than a previous route.
      // Less than equal, since entry may find this route
      if (prevRoute == nullptr || route.cost <= prevRoute->cost)
        {
          // Add self to children
          routesToAdd.insert(route);
        }
    }

  RouteSet routesToRemove;
  if (captureWasTurnedOn)
    {
      // Capture flag on
      routesToRemove = getAncestorRoutes(entry);

      // Remove ancestor routes from self
      removeInheritedRoutesFromEntry(entry, routesToRemove);
    }

  modifyChildrensInheritedRoutes(entry, routesToAdd, routesToRemove);

  // If another route with same faceId and lower cost, don't update.
  // Must be done last so that add updates replace removal updates
  // Create FIB update for new entry
  if (route.cost <= entry.getRouteWithLowestCostByFaceId(route.faceId)->cost)
    {
      insertFibUpdate(FibUpdate::createAddUpdate(entry.getName(), route.faceId, route.cost));
    }
}

void
Rib::createFibUpdatesForUpdatedRoute(RibEntry& entry, const Route& route,
                                     const uint64_t previousFlags, const uint64_t previousCost)
{
  const bool costDidChange = (route.cost != previousCost);

  // Look for the installed route with the lowest cost and child inherit set
  shared_ptr<Route> prevRoute = entry.getRouteWithLowestCostAndChildInheritByFaceId(route.faceId);

  // No flags changed and cost didn't change, no change in FIB
  if (route.flags == previousFlags && !costDidChange)
    {
      return;
    }

  // Cost changed so create update for the entry itself
  if (costDidChange)
    {
      // Create update if this route's cost is lower than other routes
       if (route.cost <= entry.getRouteWithLowestCostByFaceId(route.faceId)->cost)
        {
          // Create FIB update for the updated entry
         insertFibUpdate(FibUpdate::createAddUpdate(entry.getName(), route.faceId, route.cost));
        }
      else if (previousCost < entry.getRouteWithLowestCostByFaceId(route.faceId)->cost)
        {
          // Create update if this route used to be the lowest cost route but is no longer
          // the lowest cost route.
          insertFibUpdate(FibUpdate::createAddUpdate(entry.getName(), prevRoute->faceId,
                                                                      prevRoute->cost));
        }

      // If another route with same faceId and lower cost and ChildInherit exists,
      // don't update children.
      if (prevRoute == nullptr || route.cost <= prevRoute->cost)
        {
          // If no flags changed but child inheritance is set, need to update children
          // with new cost
          if ((route.flags == previousFlags) && isChildInheritFlagSet(route.flags))
          {
            // Add self to children
            RouteSet routesToAdd;
            routesToAdd.insert(route);
            modifyChildrensInheritedRoutes(entry, routesToAdd, RouteSet());

            return;
          }
        }
    }

  // Child inherit was turned on
  if (!isChildInheritFlagSet(previousFlags) && isChildInheritFlagSet(route.flags))
    {
      // If another route with same faceId and lower cost and ChildInherit exists,
      // don't update children.
      if (prevRoute == nullptr || route.cost <= prevRoute->cost)
        {
          // Add self to children
          RouteSet routesToAdd;
          routesToAdd.insert(route);
          modifyChildrensInheritedRoutes(entry, routesToAdd, RouteSet());
        }

    } // Child inherit was turned off
  else if (isChildInheritFlagSet(previousFlags) && !isChildInheritFlagSet(route.flags))
    {
      // Remove self from children
      RouteSet routesToRemove;
      routesToRemove.insert(route);

      RouteSet routesToAdd;
      // If another route with same faceId and ChildInherit exists, update children with this route.
      if (prevRoute != nullptr)
        {
          routesToAdd.insert(*prevRoute);
        }
      else
        {
          // Look for an ancestor that was blocked previously
          const RouteSet ancestorRoutes = getAncestorRoutes(entry);
          RouteSet::iterator it = ancestorRoutes.find(route);

          // If an ancestor is found, add it to children
          if (it != ancestorRoutes.end())
            {
              routesToAdd.insert(*it);
            }
        }

      modifyChildrensInheritedRoutes(entry, routesToAdd, routesToRemove);
    }

  // Capture was turned on
  if (!isCaptureFlagSet(previousFlags) && isCaptureFlagSet(route.flags))
    {
      RouteSet ancestorRoutes = getAncestorRoutes(entry);

      // Remove ancestor routes from self
      removeInheritedRoutesFromEntry(entry, ancestorRoutes);

      // Remove ancestor routes from children
      modifyChildrensInheritedRoutes(entry, RouteSet(), ancestorRoutes);
    }  // Capture was turned off
  else if (isCaptureFlagSet(previousFlags) && !isCaptureFlagSet(route.flags))
    {
      RouteSet ancestorRoutes = getAncestorRoutes(entry);

      // Add ancestor routes to self
      addInheritedRoutesToEntry(entry, ancestorRoutes);

      // Add ancestor routes to children
      modifyChildrensInheritedRoutes(entry, ancestorRoutes, RouteSet());
    }
}

void
Rib::createFibUpdatesForErasedRoute(RibEntry& entry, const Route& route,
                                    const bool captureWasTurnedOff)
{
  insertFibUpdate(FibUpdate::createRemoveUpdate(entry.getName(), route.faceId));

  if (areBothFlagsSet(route.flags))
    {
      // Remove self from children
      RouteSet routesToRemove;
      routesToRemove.insert(route);

      // If capture is turned off for the route, need to add ancestors
      // to self and children
      RouteSet routesToAdd;
      if (captureWasTurnedOff)
        {
          // Look for ancestors that were blocked previously
          routesToAdd = getAncestorRoutes(entry);

          // Add ancestor routes to self
          addInheritedRoutesToEntry(entry, routesToAdd);
        }

      modifyChildrensInheritedRoutes(entry, routesToAdd, routesToRemove);
    }
  else if (isChildInheritFlagSet(route.flags))
    {
      // If not blocked by capture, add inherited routes to children
      RouteSet routesToAdd;
      if (!entry.hasCapture())
        {
          routesToAdd = getAncestorRoutes(entry);
        }

      RouteSet routesToRemove;
      routesToRemove.insert(route);

      // Add ancestor routes to children
      modifyChildrensInheritedRoutes(entry, routesToAdd, routesToRemove);
    }
  else if (isCaptureFlagSet(route.flags))
    {
      // If capture is turned off for the route, need to add ancestors
      // to self and children
      RouteSet routesToAdd;
      if (captureWasTurnedOff)
        {
          // Look for an ancestors that were blocked previously
          routesToAdd = getAncestorRoutes(entry);

          // Add ancestor routes to self
          addInheritedRoutesToEntry(entry, routesToAdd);
        }

      modifyChildrensInheritedRoutes(entry, routesToAdd, RouteSet());
    }

  // Need to check if the removed route was blocking an inherited route
  RouteSet ancestorRoutes = getAncestorRoutes(entry);

  if (!entry.hasCapture())
  {
    // If there is an ancestor route with the same Face ID as the erased route, add that route
    // to the current entry
    RouteSet::iterator it = ancestorRoutes.find(route);

    if (it != ancestorRoutes.end())
      {
        entry.addInheritedRoute(*it);
        insertFibUpdate(FibUpdate::createAddUpdate(entry.getName(), it->faceId, it->cost));
      }
  }
}

void
Rib::createFibUpdatesForErasedRibEntry(RibEntry& entry)
{
  for (RibEntry::RouteList::iterator it = entry.getInheritedRoutes().begin();
       it != entry.getInheritedRoutes().end(); ++it)
    {
      insertFibUpdate(FibUpdate::createRemoveUpdate(entry.getName(), it->faceId));
    }
}

Rib::RouteSet
Rib::getAncestorRoutes(const RibEntry& entry) const
{
  RouteSet ancestorRoutes(&sortRoutes);

  shared_ptr<RibEntry> parent = entry.getParent();

  while (static_cast<bool>(parent))
    {
      for (RibEntry::iterator it = parent->getRoutes().begin();
           it != parent->getRoutes().end(); ++it)
        {
          if (isChildInheritFlagSet(it->flags))
            {
              ancestorRoutes.insert(*it);
            }
        }

      if (parent->hasCapture())
        {
          break;
        }

      parent = parent->getParent();
    }

    return ancestorRoutes;
}

void
Rib::addInheritedRoutesToEntry(RibEntry& entry, const Rib::RouteSet& routesToAdd)
{
  for (RouteSet::const_iterator it = routesToAdd.begin(); it != routesToAdd.end(); ++it)
    {
      // Don't add an ancestor route if the namespace has a route with that Face ID
      if (!entry.hasFaceId(it->faceId))
        {
          entry.addInheritedRoute(*it);
          insertFibUpdate(FibUpdate::createAddUpdate(entry.getName(), it->faceId, it->cost));
        }
    }
}

void
Rib::removeInheritedRoutesFromEntry(RibEntry& entry, const Rib::RouteSet& routesToRemove)
{
  for (RouteSet::const_iterator it = routesToRemove.begin(); it != routesToRemove.end(); ++it)
    {
      // Only remove if the route has been inherited
      if (entry.hasInheritedRoute(*it))
        {
          entry.removeInheritedRoute(*it);
          insertFibUpdate(FibUpdate::createRemoveUpdate(entry.getName(), it->faceId));
        }
    }
}

void
Rib::modifyChildrensInheritedRoutes(RibEntry& entry, const Rib::RouteSet& routesToAdd,
                                                     const Rib::RouteSet& routesToRemove)
{
  RibEntryList children = entry.getChildren();

  for (RibEntryList::iterator child = children.begin(); child != children.end(); ++child)
    {
      traverseSubTree(*(*child), routesToAdd, routesToRemove);
    }
}

void
Rib::traverseSubTree(RibEntry& entry, Rib::RouteSet routesToAdd, Rib::RouteSet routesToRemove)
{
  // If a route on the namespace has the capture flag set, ignore self and children
  if (entry.hasCapture())
    {
      return;
    }

  // Remove inherited routes from current namespace
  for (Rib::RouteSet::const_iterator removeIt = routesToRemove.begin();
       removeIt != routesToRemove.end(); )
    {
      // If a route on the namespace has the same face and child inheritance set, ignore this route
      if (entry.hasChildInheritOnFaceId(removeIt->faceId))
        {
          routesToRemove.erase(removeIt++);
          continue;
        }

      // Only remove route if it removes an existing inherited route
      if (entry.hasInheritedRoute(*removeIt))
        {
          entry.removeInheritedRoute(*removeIt);
          insertFibUpdate(FibUpdate::createRemoveUpdate(entry.getName(), removeIt->faceId));
        }

      ++removeIt;
    }

  // Add inherited routes to current namespace
  for (Rib::RouteSet::const_iterator addIt = routesToAdd.begin(); addIt != routesToAdd.end(); )
    {
      // If a route on the namespace has the same face and child inherit set, ignore this route
      if (entry.hasChildInheritOnFaceId(addIt->faceId))
      {
        routesToAdd.erase(addIt++);
        continue;
      }

      // Only add route if it does not override an existing route
      if (!entry.hasFaceId(addIt->faceId))
        {
          RibEntry::RouteList::iterator routeIt = entry.findInheritedRoute(*addIt);

          // If the entry already has the inherited route, just update the route
          if (routeIt != entry.getInheritedRoutes().end())
            {
              routeIt->cost = addIt->cost;
            }
          else // Otherwise, this is a newly inherited route
            {
              entry.addInheritedRoute(*addIt);
            }

          insertFibUpdate(FibUpdate::createAddUpdate(entry.getName(), addIt->faceId, addIt->cost));
        }

      ++addIt;
    }

  Rib::RibEntryList children = entry.getChildren();

  // Apply route operations to current namespace's children
  for (Rib::RibEntryList::iterator child = children.begin(); child != children.end(); ++child)
    {
      traverseSubTree(*(*child), routesToAdd, routesToRemove);
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
