/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#include "fib-updater.hpp"

#include "core/logger.hpp"

#include <ndn-cxx/mgmt/nfd/control-parameters.hpp>

namespace nfd {
namespace rib {

using ndn::nfd::ControlParameters;

NFD_LOG_INIT("FibUpdater");

const unsigned int FibUpdater::MAX_NUM_TIMEOUTS = 10;
const uint32_t FibUpdater::ERROR_FACE_NOT_FOUND = 410;

FibUpdater::FibUpdater(Rib& rib, ndn::nfd::Controller& controller)
  : m_rib(rib)
  , m_controller(controller)
{
  rib.setFibUpdater(this);
}

void
FibUpdater::computeAndSendFibUpdates(const RibUpdateBatch& batch,
                                     const FibUpdateSuccessCallback& onSuccess,
                                     const FibUpdateFailureCallback& onFailure)
{
  m_batchFaceId = batch.getFaceId();

  // Erase previously calculated inherited routes
  m_inheritedRoutes.clear();

  // Erase previously calculated FIB updates
  m_updatesForBatchFaceId.clear();
  m_updatesForNonBatchFaceId.clear();

  computeUpdates(batch);

  sendUpdatesForBatchFaceId(onSuccess, onFailure);
}

void
FibUpdater::computeUpdates(const RibUpdateBatch& batch)
{
  NFD_LOG_DEBUG("Computing updates for batch with faceID: " << batch.getFaceId());

  // Compute updates and add to m_fibUpdates
  for (const RibUpdate& update : batch) {
    switch (update.getAction()) {
      case RibUpdate::REGISTER:
        computeUpdatesForRegistration(update);
        break;
      case RibUpdate::UNREGISTER:
        computeUpdatesForUnregistration(update);
        break;
      case RibUpdate::REMOVE_FACE:
        computeUpdatesForUnregistration(update);

        // Do not apply updates with the same face ID as the destroyed face
        // since they will be rejected by the FIB
        m_updatesForBatchFaceId.clear();
        break;
    }
  }
}

void
FibUpdater::computeUpdatesForRegistration(const RibUpdate& update)
{
  const Name& prefix = update.getName();
  const Route& route = update.getRoute();

  Rib::const_iterator it = m_rib.find(prefix);

  // Name prefix exists
  if (it != m_rib.end()) {
    shared_ptr<const RibEntry> entry(it->second);

    RibEntry::const_iterator existingRoute = entry->findRoute(route);

    // Route will be new
    if (existingRoute == entry->end()) {
      // Will the new route change the namespace's capture flag?
      bool willCaptureBeTurnedOn = (entry->hasCapture() == false && route.isRibCapture());

      createFibUpdatesForNewRoute(*entry, route, willCaptureBeTurnedOn);
    }
    else {
      // Route already exists
      RibEntry entryCopy = *entry;

      Route& routeToUpdate = *(entryCopy.findRoute(route));

      routeToUpdate.flags = route.flags;
      routeToUpdate.cost = route.cost;
      routeToUpdate.expires = route.expires;

      createFibUpdatesForUpdatedRoute(entryCopy, route, *existingRoute);
    }
  }
  else {
    // New name in RIB
    // Find prefix's parent
    shared_ptr<RibEntry> parent = m_rib.findParent(prefix);

    Rib::RibEntryList descendants = m_rib.findDescendantsForNonInsertedName(prefix);
    Rib::RibEntryList children;

    for (const auto& descendant : descendants) {
      // If the child has the same parent as the new entry,
      // the new entry must be the child's new parent
      if (descendant->getParent() == parent) {
        children.push_back(descendant);
      }
    }

    createFibUpdatesForNewRibEntry(prefix, route, children);
  }
}

void
FibUpdater::computeUpdatesForUnregistration(const RibUpdate& update)
{
  const Name& prefix = update.getName();
  const Route& route = update.getRoute();

  Rib::const_iterator ribIt = m_rib.find(prefix);

  // Name prefix exists
  if (ribIt != m_rib.end()) {
    shared_ptr<const RibEntry> entry(ribIt->second);

    const bool hadCapture = entry->hasCapture();

    RibEntry::const_iterator existing = entry->findRoute(route);

    if (existing != entry->end()) {
      RibEntry temp = *entry;

      // Erase route in temp entry
      temp.eraseRoute(route);

      const bool captureWasTurnedOff = (hadCapture && !temp.hasCapture());

      createFibUpdatesForErasedRoute(temp, *existing, captureWasTurnedOff);

      // The RibEntry still has the face ID; need to update FIB
      // with lowest cost for the same face instead of removing the face from the FIB
      const Route* next = entry->getRouteWithSecondLowestCostByFaceId(route.faceId);

      if (next != nullptr) {
        createFibUpdatesForNewRoute(temp, *next, false);
      }

      // The RibEntry will be empty after this removal
      if (entry->getNRoutes() == 1) {
        createFibUpdatesForErasedRibEntry(*entry);
      }
    }
  }
}

void
FibUpdater::sendUpdates(const FibUpdateList& updates,
                        const FibUpdateSuccessCallback& onSuccess,
                        const FibUpdateFailureCallback& onFailure)
{
  std::string updateString = (updates.size() == 1) ? " update" : " updates";
  NFD_LOG_DEBUG("Applying " << updates.size() << updateString << " to FIB");

  for (const FibUpdate& update : updates) {
    NFD_LOG_DEBUG("Sending FIB update: " << update);

    if (update.action == FibUpdate::ADD_NEXTHOP) {
      sendAddNextHopUpdate(update, onSuccess, onFailure);
    }
    else if (update.action == FibUpdate::REMOVE_NEXTHOP) {
      sendRemoveNextHopUpdate(update, onSuccess, onFailure);
    }
  }
}

void
FibUpdater::sendUpdatesForBatchFaceId(const FibUpdateSuccessCallback& onSuccess,
                                      const FibUpdateFailureCallback& onFailure)
{
  if (m_updatesForBatchFaceId.size() > 0) {
    sendUpdates(m_updatesForBatchFaceId, onSuccess, onFailure);
  }
  else {
    sendUpdatesForNonBatchFaceId(onSuccess, onFailure);
  }
}

void
FibUpdater::sendUpdatesForNonBatchFaceId(const FibUpdateSuccessCallback& onSuccess,
                                         const FibUpdateFailureCallback& onFailure)
{
  if (m_updatesForNonBatchFaceId.size() > 0) {
    sendUpdates(m_updatesForNonBatchFaceId, onSuccess, onFailure);
  }
  else {
    onSuccess(m_inheritedRoutes);
  }
}

void
FibUpdater::sendAddNextHopUpdate(const FibUpdate& update,
                                 const FibUpdateSuccessCallback& onSuccess,
                                 const FibUpdateFailureCallback& onFailure,
                                 uint32_t nTimeouts)
{
  m_controller.start<ndn::nfd::FibAddNextHopCommand>(
    ControlParameters()
      .setName(update.name)
      .setFaceId(update.faceId)
      .setCost(update.cost),
    bind(&FibUpdater::onUpdateSuccess, this, update, onSuccess, onFailure),
    bind(&FibUpdater::onUpdateError, this, update, onSuccess, onFailure, _1, nTimeouts));
}

void
FibUpdater::sendRemoveNextHopUpdate(const FibUpdate& update,
                                    const FibUpdateSuccessCallback& onSuccess,
                                    const FibUpdateFailureCallback& onFailure,
                                    uint32_t nTimeouts)
{
  m_controller.start<ndn::nfd::FibRemoveNextHopCommand>(
    ControlParameters()
      .setName(update.name)
      .setFaceId(update.faceId),
    bind(&FibUpdater::onUpdateSuccess, this, update, onSuccess, onFailure),
    bind(&FibUpdater::onUpdateError, this, update, onSuccess, onFailure, _1, nTimeouts));
}

void
FibUpdater::onUpdateSuccess(const FibUpdate update,
                            const FibUpdateSuccessCallback& onSuccess,
                            const FibUpdateFailureCallback& onFailure)
{
  if (update.faceId == m_batchFaceId) {
    m_updatesForBatchFaceId.remove(update);

    if (m_updatesForBatchFaceId.size() == 0) {
      sendUpdatesForNonBatchFaceId(onSuccess, onFailure);
    }
  }
  else {
    m_updatesForNonBatchFaceId.remove(update);

    if (m_updatesForNonBatchFaceId.size() == 0) {
      onSuccess(m_inheritedRoutes);
    }
  }
}

void
FibUpdater::onUpdateError(const FibUpdate update,
                          const FibUpdateSuccessCallback& onSuccess,
                          const FibUpdateFailureCallback& onFailure,
                          const ndn::nfd::ControlResponse& response, uint32_t nTimeouts)
{
  uint32_t code = response.getCode();
  NFD_LOG_DEBUG("Failed to apply " << update <<
                " (code: " << code << ", error: " << response.getText() << ")");

  if (code == ndn::nfd::Controller::ERROR_TIMEOUT && nTimeouts < MAX_NUM_TIMEOUTS) {
    sendAddNextHopUpdate(update, onSuccess, onFailure, ++nTimeouts);
  }
  else if (code == ERROR_FACE_NOT_FOUND) {
    if (update.faceId == m_batchFaceId) {
      onFailure(code, response.getText());
    }
    else {
      m_updatesForNonBatchFaceId.remove(update);

      if (m_updatesForNonBatchFaceId.size() == 0) {
        onSuccess(m_inheritedRoutes);
      }
    }
  }
  else {
    BOOST_THROW_EXCEPTION(Error("Non-recoverable error: " + response.getText() +
                                " code: " + to_string(code)));
  }
}

void
FibUpdater::addFibUpdate(FibUpdate update)
{
  FibUpdateList& updates = (update.faceId == m_batchFaceId) ? m_updatesForBatchFaceId :
                                                              m_updatesForNonBatchFaceId;

  // If an update with the same name and route already exists,
  // replace it
  FibUpdateList::iterator it = std::find_if(updates.begin(), updates.end(),
    [&update] (const FibUpdate& other) {
      return update.name == other.name && update.faceId == other.faceId;
    });

  if (it != updates.end()) {
    FibUpdate& existingUpdate = *it;
    existingUpdate.action = update.action;
    existingUpdate.cost = update.cost;
  }
  else {
    updates.push_back(update);
  }
}

void
FibUpdater::addInheritedRoutes(const RibEntry& entry, const Rib::RouteSet& routesToAdd)
{
  for (const Route& route : routesToAdd) {
    // Don't add an ancestor faceId if the namespace has an entry for that faceId
    if (!entry.hasFaceId(route.faceId)) {
      // Create a record of the inherited route so it can be added to the RIB later
      addInheritedRoute(entry.getName(), route);

      addFibUpdate(FibUpdate::createAddUpdate(entry.getName(), route.faceId, route.cost));
    }
  }
}

void
FibUpdater::addInheritedRoutes(const Name& name, const Rib::RouteSet& routesToAdd,
                               const Route& ignore)
{
  for (const Route& route : routesToAdd) {
    if (route.faceId != ignore.faceId) {
      // Create a record of the inherited route so it can be added to the RIB later
      addInheritedRoute(name, route);

      addFibUpdate(FibUpdate::createAddUpdate(name, route.faceId, route.cost));
    }
  }
}

void
FibUpdater::removeInheritedRoutes(const RibEntry& entry, const Rib::Rib::RouteSet& routesToRemove)
{
  for (const Route& route : routesToRemove) {
    // Only remove if the route has been inherited
    if (entry.hasInheritedRoute(route)) {
      removeInheritedRoute(entry.getName(), route);
      addFibUpdate(FibUpdate::createRemoveUpdate(entry.getName(), route.faceId));
    }
  }
}

void
FibUpdater::createFibUpdatesForNewRibEntry(const Name& name, const Route& route,
                                           const Rib::RibEntryList& children)
{
  // Create FIB update for new entry
  addFibUpdate(FibUpdate::createAddUpdate(name, route.faceId, route.cost));

  // No flags are set
  if (!route.isChildInherit() && !route.isRibCapture()) {
    // Add ancestor routes to self
    addInheritedRoutes(name, m_rib.getAncestorRoutes(name), route);
  }
  else if (route.isChildInherit() && route.isRibCapture()) {
    // Add route to children
    Rib::RouteSet routesToAdd;
    routesToAdd.insert(route);

    // Remove routes blocked by capture and add self to children
    modifyChildrensInheritedRoutes(children, routesToAdd, m_rib.getAncestorRoutes(name));
  }
  else if (route.isChildInherit()) {
    Rib::RouteSet ancestorRoutes = m_rib.getAncestorRoutes(name);

    // Add ancestor routes to self
    addInheritedRoutes(name, ancestorRoutes, route);

    // If there is an ancestor route which is the same as the new route, replace it
    // with the new route
    Rib::RouteSet::iterator it = ancestorRoutes.find(route);

    // There is a route that needs to be overwritten, erase and then replace
    if (it != ancestorRoutes.end()) {
      ancestorRoutes.erase(it);
    }

    // Add new route to ancestor list so it can be added to children
    ancestorRoutes.insert(route);

    // Add ancestor routes to children
    modifyChildrensInheritedRoutes(children, ancestorRoutes, Rib::RouteSet());
  }
  else if (route.isRibCapture()) {
    // Remove routes blocked by capture
    modifyChildrensInheritedRoutes(children, Rib::RouteSet(), m_rib.getAncestorRoutes(name));
  }
}

void
FibUpdater::createFibUpdatesForNewRoute(const RibEntry& entry, const Route& route,
                                        bool captureWasTurnedOn)
{
  // Only update if the new route has a lower cost than a previously installed route
  const Route* prevRoute = entry.getRouteWithLowestCostAndChildInheritByFaceId(route.faceId);

  Rib::RouteSet routesToAdd;
  if (route.isChildInherit()) {
    // Add to children if this new route doesn't override a previous lower cost, or
    // add to children if this new route is lower cost than a previous route.
    // Less than equal, since entry may find this route
    if (prevRoute == nullptr || route.cost <= prevRoute->cost) {
      // Add self to children
      routesToAdd.insert(route);
    }
  }

  Rib::RouteSet routesToRemove;
  if (captureWasTurnedOn) {
    // Capture flag on
    routesToRemove = m_rib.getAncestorRoutes(entry);

    // Remove ancestor routes from self
    removeInheritedRoutes(entry, routesToRemove);
  }

  modifyChildrensInheritedRoutes(entry.getChildren(), routesToAdd, routesToRemove);

  // If another route with same faceId and lower cost exists, don't update.
  // Must be done last so that add updates replace removal updates
  // Create FIB update for new entry
  const Route* other = entry.getRouteWithLowestCostByFaceId(route.faceId);

  if (other == nullptr || route.cost <= other->cost) {
    addFibUpdate(FibUpdate::createAddUpdate(entry.getName(), route.faceId, route.cost));
  }
}

void
FibUpdater::createFibUpdatesForUpdatedRoute(const RibEntry& entry, const Route& route,
                                            const Route& existingRoute)
{
  const bool costDidChange = (route.cost != existingRoute.cost);

  // Look for an installed route with the lowest cost and child inherit set
  const Route* prevRoute = entry.getRouteWithLowestCostAndChildInheritByFaceId(route.faceId);

  // No flags changed and cost didn't change, no change in FIB
  if (route.flags == existingRoute.flags && !costDidChange) {
    return;
  }

  // Cost changed so create update for the entry itself
  if (costDidChange) {
    // Create update if this route's cost is lower than other routes
    if (route.cost <= entry.getRouteWithLowestCostByFaceId(route.faceId)->cost) {
      // Create FIB update for the updated entry
      addFibUpdate(FibUpdate::createAddUpdate(entry.getName(), route.faceId, route.cost));
    }
    else if (existingRoute.cost < entry.getRouteWithLowestCostByFaceId(route.faceId)->cost) {
      // Create update if this route used to be the lowest route but is no longer
      // the lowest cost route.
      addFibUpdate(FibUpdate::createAddUpdate(entry.getName(), prevRoute->faceId, prevRoute->cost));
    }

    // If another route with same faceId and lower cost and ChildInherit exists,
    // don't update children.
    if (prevRoute == nullptr || route.cost <= prevRoute->cost) {
      // If no flags changed but child inheritance is set, need to update children
      // with new cost
      if ((route.flags == existingRoute.flags) && route.isChildInherit()) {
        // Add self to children
        Rib::RouteSet routesToAdd;
        routesToAdd.insert(route);
        modifyChildrensInheritedRoutes(entry.getChildren(), routesToAdd, Rib::RouteSet());

        return;
      }
    }
  }

  // Child inherit was turned on
  if (!existingRoute.isChildInherit() && route.isChildInherit()) {
    // If another route with same faceId and lower cost and ChildInherit exists,
    // don't update children.
    if (prevRoute == nullptr || route.cost <= prevRoute->cost) {
      // Add self to children
      Rib::RouteSet routesToAdd;
      routesToAdd.insert(route);
      modifyChildrensInheritedRoutes(entry.getChildren(), routesToAdd, Rib::RouteSet());
    }
  } // Child inherit was turned off
  else if (existingRoute.isChildInherit() && !route.isChildInherit()) {
    // Remove self from children
    Rib::RouteSet routesToRemove;
    routesToRemove.insert(route);

    Rib::RouteSet routesToAdd;
    // If another route with same faceId and ChildInherit exists, update children with this route.
    if (prevRoute != nullptr) {
      routesToAdd.insert(*prevRoute);
    }
    else {
      // Look for an ancestor that was blocked previously
      const Rib::RouteSet ancestorRoutes = m_rib.getAncestorRoutes(entry);
      Rib::RouteSet::iterator it = ancestorRoutes.find(route);

      // If an ancestor is found, add it to children
      if (it != ancestorRoutes.end()) {
        routesToAdd.insert(*it);
      }
    }

    modifyChildrensInheritedRoutes(entry.getChildren(), routesToAdd, routesToRemove);
  }

  // Capture was turned on
  if (!existingRoute.isRibCapture() && route.isRibCapture()) {
    Rib::RouteSet ancestorRoutes = m_rib.getAncestorRoutes(entry);

    // Remove ancestor routes from self
    removeInheritedRoutes(entry, ancestorRoutes);

    // Remove ancestor routes from children
    modifyChildrensInheritedRoutes(entry.getChildren(), Rib::RouteSet(), ancestorRoutes);
  }  // Capture was turned off
  else if (existingRoute.isRibCapture() && !route.isRibCapture()) {
    Rib::RouteSet ancestorRoutes = m_rib.getAncestorRoutes(entry);

    // Add ancestor routes to self
    addInheritedRoutes(entry, ancestorRoutes);

    // Add ancestor routes to children
    modifyChildrensInheritedRoutes(entry.getChildren(), ancestorRoutes, Rib::RouteSet());
  }
}

void
FibUpdater::createFibUpdatesForErasedRoute(const RibEntry& entry, const Route& route,
                                               const bool captureWasTurnedOff)
{
  addFibUpdate(FibUpdate::createRemoveUpdate(entry.getName(), route.faceId));

  if (route.isChildInherit() && route.isRibCapture()) {
    // Remove self from children
    Rib::RouteSet routesToRemove;
    routesToRemove.insert(route);

    // If capture is turned off for the route and another route is installed in the RibEntry,
    // add ancestors to self
    Rib::RouteSet routesToAdd;
    if (captureWasTurnedOff && entry.getNRoutes() != 0) {
      // Look for an ancestors that were blocked previously
      routesToAdd = m_rib.getAncestorRoutes(entry);

      // Add ancestor routes to self
      addInheritedRoutes(entry, routesToAdd);
    }

    modifyChildrensInheritedRoutes(entry.getChildren(), routesToAdd, routesToRemove);
  }
  else if (route.isChildInherit()) {
    // If not blocked by capture, add inherited routes to children
    Rib::RouteSet routesToAdd;
    if (!entry.hasCapture()) {
      routesToAdd = m_rib.getAncestorRoutes(entry);
    }

    Rib::RouteSet routesToRemove;
    routesToRemove.insert(route);

    // Add ancestor routes to children
    modifyChildrensInheritedRoutes(entry.getChildren(), routesToAdd, routesToRemove);
  }
  else if (route.isRibCapture()) {
    // If capture is turned off for the route and another route is installed in the RibEntry,
    // add ancestors to self
    Rib::RouteSet routesToAdd;
    if (captureWasTurnedOff && entry.getNRoutes() != 0) {
      // Look for an ancestors that were blocked previously
      routesToAdd = m_rib.getAncestorRoutes(entry);

      // Add ancestor routes to self
      addInheritedRoutes(entry, routesToAdd);
    }

    modifyChildrensInheritedRoutes(entry.getChildren(), routesToAdd, Rib::RouteSet());
  }

  // Need to check if the removed route was blocking an inherited route
  Rib::RouteSet ancestorRoutes = m_rib.getAncestorRoutes(entry);

  // If the current entry has capture set or is pending removal, don't add inherited route
  if (!entry.hasCapture() && entry.getNRoutes() != 0) {
    // If there is an ancestor route which is the same as the erased route, add that route
    // to the current entry
    Rib::RouteSet::iterator it = ancestorRoutes.find(route);

    if (it != ancestorRoutes.end()) {
      addInheritedRoute(entry.getName(), *it);
      addFibUpdate(FibUpdate::createAddUpdate(entry.getName(), it->faceId, it->cost));
    }
  }
}

void
FibUpdater::createFibUpdatesForErasedRibEntry(const RibEntry& entry)
{
  for (const Route& route : entry.getInheritedRoutes()) {
    addFibUpdate(FibUpdate::createRemoveUpdate(entry.getName(), route.faceId));
  }
}

void
FibUpdater::modifyChildrensInheritedRoutes(const Rib::RibEntryList& children,
                                           const Rib::RouteSet& routesToAdd,
                                           const Rib::RouteSet& routesToRemove)
{
  for (const auto& child : children) {
    traverseSubTree(*child, routesToAdd, routesToRemove);
  }
}

void
FibUpdater::traverseSubTree(const RibEntry& entry, Rib::Rib::RouteSet routesToAdd,
                            Rib::Rib::RouteSet routesToRemove)
{
  // If a route on the namespace has the capture flag set, ignore self and children
  if (entry.hasCapture()) {
    return;
  }

  // Remove inherited routes from current namespace
  for (Rib::RouteSet::const_iterator removeIt = routesToRemove.begin();
       removeIt != routesToRemove.end(); )
  {
    // If a route on the namespace has the same face ID and child inheritance set,
    // ignore this route
    if (entry.hasChildInheritOnFaceId(removeIt->faceId)) {
      routesToRemove.erase(removeIt++);
      continue;
    }

    // Only remove route if it removes an existing inherited route
    if (entry.hasInheritedRoute(*removeIt)) {
      removeInheritedRoute(entry.getName(), *removeIt);
      addFibUpdate(FibUpdate::createRemoveUpdate(entry.getName(), removeIt->faceId));
    }

    ++removeIt;
  }

  // Add inherited routes to current namespace
  for (Rib::RouteSet::const_iterator addIt = routesToAdd.begin(); addIt != routesToAdd.end(); ) {

    // If a route on the namespace has the same face ID and child inherit set, ignore this face
    if (entry.hasChildInheritOnFaceId(addIt->faceId)) {
      routesToAdd.erase(addIt++);
      continue;
    }

    // Only add route if it does not override an existing route
    if (!entry.hasFaceId(addIt->faceId)) {
      addInheritedRoute(entry.getName(), *addIt);
      addFibUpdate(FibUpdate::createAddUpdate(entry.getName(), addIt->faceId, addIt->cost));
    }

    ++addIt;
  }

  modifyChildrensInheritedRoutes(entry.getChildren(), routesToAdd, routesToRemove);
}

void
FibUpdater::addInheritedRoute(const Name& name, const Route& route)
{
  RibUpdate update;
  update.setAction(RibUpdate::REGISTER)
        .setName(name)
        .setRoute(route);

  m_inheritedRoutes.push_back(update);
}

void
FibUpdater::removeInheritedRoute(const Name& name, const Route& route)
{
  RibUpdate update;
  update.setAction(RibUpdate::UNREGISTER)
        .setName(name)
        .setRoute(route);

  m_inheritedRoutes.push_back(update);
}

} // namespace rib
} // namespace nfd
