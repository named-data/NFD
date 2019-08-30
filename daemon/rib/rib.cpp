/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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
#include "fib-updater.hpp"
#include "common/logger.hpp"

namespace nfd {
namespace rib {

NFD_LOG_INIT(Rib);

bool
operator<(const RibRouteRef& lhs, const RibRouteRef& rhs)
{
  return std::tie(lhs.entry->getName(), lhs.route->faceId, lhs.route->origin) <
         std::tie(rhs.entry->getName(), rhs.route->faceId, rhs.route->origin);
}

static inline bool
sortRoutes(const Route& lhs, const Route& rhs)
{
  return lhs.faceId < rhs.faceId;
}

void
Rib::setFibUpdater(FibUpdater* updater)
{
  m_fibUpdater = updater;
}

Rib::const_iterator
Rib::find(const Name& prefix) const
{
  return m_rib.find(prefix);
}

Route*
Rib::find(const Name& prefix, const Route& route) const
{
  auto ribIt = m_rib.find(prefix);

  // Name prefix exists
  if (ribIt != m_rib.end()) {
    shared_ptr<RibEntry> entry = ribIt->second;
    auto routeIt = entry->findRoute(route);
    if (routeIt != entry->end()) {
      return &*routeIt;
    }
  }

  return nullptr;
}

Route*
Rib::findLongestPrefix(const Name& prefix, const Route& route) const
{
  Route* existingRoute = find(prefix, route);
  if (existingRoute == nullptr) {
    auto parent = findParent(prefix);
    if (parent) {
      existingRoute = find(parent->getName(), route);
    }
  }

  return existingRoute;
}

void
Rib::insert(const Name& prefix, const Route& route)
{
  auto ribIt = m_rib.find(prefix);

  // Name prefix exists
  if (ribIt != m_rib.end()) {
    shared_ptr<RibEntry> entry(ribIt->second);

    RibEntry::iterator entryIt;
    bool didInsert = false;
    std::tie(entryIt, didInsert) = entry->insertRoute(route);

    if (didInsert) {
      // The route was new and we successfully inserted it.
      m_nItems++;

      afterAddRoute(RibRouteRef{entry, entryIt});

      // Register with face lookup table
      m_faceEntries.emplace(route.faceId, entry);
    }
    else {
      // Route exists, update fields
      // First cancel old scheduled event, if any, then set the EventId to new one
      if (entryIt->getExpirationEvent()) {
        NFD_LOG_TRACE("Cancelling expiration event for " << entry->getName() << " " << *entryIt);
        entryIt->cancelExpirationEvent();
      }

      *entryIt = route;
    }
  }
  else {
    // New name prefix
    auto entry = make_shared<RibEntry>();

    m_rib[prefix] = entry;
    m_nItems++;

    entry->setName(prefix);
    auto routeIt = entry->insertRoute(route).first;

    // Find prefix's parent
    shared_ptr<RibEntry> parent = findParent(prefix);

    // Add self to parent's children
    if (parent != nullptr) {
      parent->addChild(entry);
    }

    RibEntryList children = findDescendants(prefix);

    for (const auto& child : children) {
      if (child->getParent() == parent) {
        // Remove child from parent and inherit parent's child
        if (parent != nullptr) {
          parent->removeChild(child);
        }

        entry->addChild(child);
      }
    }

    // Register with face lookup table
    m_faceEntries.emplace(route.faceId, entry);

    // do something after inserting an entry
    afterInsertEntry(prefix);
    afterAddRoute(RibRouteRef{entry, routeIt});
  }
}

void
Rib::erase(const Name& prefix, const Route& route)
{
  auto ribIt = m_rib.find(prefix);
  if (ribIt == m_rib.end()) {
    // Name prefix does not exist
    return;
  }

  shared_ptr<RibEntry> entry = ribIt->second;
  auto routeIt = entry->findRoute(route);

  if (routeIt != entry->end()) {
    beforeRemoveRoute(RibRouteRef{entry, routeIt});

    auto faceId = route.faceId;
    entry->eraseRoute(routeIt);
    m_nItems--;

    // If this RibEntry no longer has this faceId, unregister from face lookup table
    if (!entry->hasFaceId(faceId)) {
      auto range = m_faceEntries.equal_range(faceId);
      for (auto it = range.first; it != range.second; ++it) {
        if (it->second == entry) {
          m_faceEntries.erase(it);
          break;
        }
      }
    }

    // If a RibEntry's route list is empty, remove it from the tree
    if (entry->getRoutes().empty()) {
      eraseEntry(ribIt);
    }
  }
}

void
Rib::onRouteExpiration(const Name& prefix, const Route& route)
{
  NFD_LOG_DEBUG(route << " for " << prefix << " has expired");

  RibUpdate update;
  update.setAction(RibUpdate::UNREGISTER)
        .setName(prefix)
        .setRoute(route);

  beginApplyUpdate(update, nullptr, nullptr);
}

shared_ptr<RibEntry>
Rib::findParent(const Name& prefix) const
{
  for (int i = prefix.size() - 1; i >= 0; i--) {
    auto it = m_rib.find(prefix.getPrefix(i));
    if (it != m_rib.end()) {
      return it->second;
    }
  }

  return nullptr;
}

std::list<shared_ptr<RibEntry>>
Rib::findDescendants(const Name& prefix) const
{
  std::list<shared_ptr<RibEntry>> children;

  RibTable::const_iterator it = m_rib.find(prefix);
  if (it != m_rib.end()) {
    ++it;
    for (; it != m_rib.end(); ++it) {
      if (prefix.isPrefixOf(it->first)) {
        children.push_back((it->second));
      }
      else {
        break;
      }
    }
  }

  return children;
}

std::list<shared_ptr<RibEntry>>
Rib::findDescendantsForNonInsertedName(const Name& prefix) const
{
  std::list<shared_ptr<RibEntry>> children;

  for (const auto& pair : m_rib) {
    if (prefix.isPrefixOf(pair.first)) {
      children.push_back(pair.second);
    }
  }

  return children;
}

Rib::RibTable::iterator
Rib::eraseEntry(RibTable::iterator it)
{
  // Entry does not exist
  if (it == m_rib.end()) {
    return m_rib.end();
  }

  shared_ptr<RibEntry> entry(it->second);

  shared_ptr<RibEntry> parent = entry->getParent();

  // Remove self from parent's children
  if (parent != nullptr) {
    parent->removeChild(entry);
  }

  for (auto childIt = entry->getChildren().begin(); childIt != entry->getChildren().end(); ) {
    shared_ptr<RibEntry> child = *childIt;

    // Advance iterator so it is not invalidated by removal
    ++childIt;

    // Remove children from self
    entry->removeChild(child);

    // Update parent's children
    if (parent != nullptr) {
      parent->addChild(child);
    }
  }

  auto nextIt = m_rib.erase(it);

  // do something after erasing an entry.
  afterEraseEntry(entry->getName());

  return nextIt;
}

Rib::RouteSet
Rib::getAncestorRoutes(const RibEntry& entry) const
{
  RouteSet ancestorRoutes(&sortRoutes);

  shared_ptr<RibEntry> parent = entry.getParent();

  while (parent != nullptr) {
    for (const Route& route : parent->getRoutes()) {
      if (route.isChildInherit()) {
        ancestorRoutes.insert(route);
      }
    }

    if (parent->hasCapture()) {
      break;
    }

    parent = parent->getParent();
  }

  return ancestorRoutes;
}

Rib::RouteSet
Rib::getAncestorRoutes(const Name& name) const
{
  RouteSet ancestorRoutes(&sortRoutes);

  shared_ptr<RibEntry> parent = findParent(name);

  while (parent != nullptr) {
    for (const Route& route : parent->getRoutes()) {
      if (route.isChildInherit()) {
        ancestorRoutes.insert(route);
      }
    }

    if (parent->hasCapture()) {
      break;
    }

    parent = parent->getParent();
  }

  return ancestorRoutes;
}

void
Rib::beginApplyUpdate(const RibUpdate& update,
                      const Rib::UpdateSuccessCallback& onSuccess,
                      const Rib::UpdateFailureCallback& onFailure)
{
  BOOST_ASSERT(m_fibUpdater != nullptr);

  addUpdateToQueue(update, onSuccess, onFailure);

  sendBatchFromQueue();
}

void
Rib::beginRemoveFace(uint64_t faceId)
{
  auto range = m_faceEntries.equal_range(faceId);
  for (auto it = range.first; it != range.second; ++it) {
    enqueueRemoveFace(*it->second, faceId);
  }
  sendBatchFromQueue();
}

void
Rib::beginRemoveFailedFaces(const std::set<uint64_t>& activeFaceIds)
{
  for (auto it = m_faceEntries.begin(); it != m_faceEntries.end(); ++it) {
    if (activeFaceIds.count(it->first) > 0) {
      continue;
    }
    enqueueRemoveFace(*it->second, it->first);
  }
  sendBatchFromQueue();
}

void
Rib::enqueueRemoveFace(const RibEntry& entry, uint64_t faceId)
{
  for (const Route& route : entry) {
    if (route.faceId != faceId) {
      continue;
    }

    RibUpdate update;
    update.setAction(RibUpdate::REMOVE_FACE)
          .setName(entry.getName())
          .setRoute(route);
    addUpdateToQueue(update, nullptr, nullptr);
  }
}

void
Rib::addUpdateToQueue(const RibUpdate& update,
                      const Rib::UpdateSuccessCallback& onSuccess,
                      const Rib::UpdateFailureCallback& onFailure)
{
  RibUpdateBatch batch(update.getRoute().faceId);
  batch.add(update);

  UpdateQueueItem item{batch, onSuccess, onFailure};
  m_updateBatches.push_back(std::move(item));
}

void
Rib::sendBatchFromQueue()
{
  if (m_updateBatches.empty() || m_isUpdateInProgress) {
    return;
  }

  m_isUpdateInProgress = true;

  UpdateQueueItem item = std::move(m_updateBatches.front());
  m_updateBatches.pop_front();

  RibUpdateBatch& batch = item.batch;

  // Until task #1698, each RibUpdateBatch contains exactly one RIB update
  BOOST_ASSERT(batch.size() == 1);

  auto fibSuccessCb = bind(&Rib::onFibUpdateSuccess, this, batch, _1, item.managerSuccessCallback);
  auto fibFailureCb = bind(&Rib::onFibUpdateFailure, this, item.managerFailureCallback, _1, _2);

  m_fibUpdater->computeAndSendFibUpdates(batch, fibSuccessCb, fibFailureCb);
}

void
Rib::onFibUpdateSuccess(const RibUpdateBatch& batch,
                        const RibUpdateList& inheritedRoutes,
                        const Rib::UpdateSuccessCallback& onSuccess)
{
  for (const RibUpdate& update : batch) {
    switch (update.getAction()) {
    case RibUpdate::REGISTER:
      insert(update.getName(), update.getRoute());
      break;
    case RibUpdate::UNREGISTER:
    case RibUpdate::REMOVE_FACE:
      erase(update.getName(), update.getRoute());
      break;
    }
  }

  // Add and remove precalculated inherited routes to RibEntries
  modifyInheritedRoutes(inheritedRoutes);

  m_isUpdateInProgress = false;

  if (onSuccess != nullptr) {
    onSuccess();
  }

  // Try to advance the batch queue
  sendBatchFromQueue();
}

void
Rib::onFibUpdateFailure(const Rib::UpdateFailureCallback& onFailure,
                        uint32_t code, const std::string& error)
{
  m_isUpdateInProgress = false;

  if (onFailure != nullptr) {
    onFailure(code, error);
  }

  // Try to advance the batch queue
  sendBatchFromQueue();
}

void
Rib::modifyInheritedRoutes(const RibUpdateList& inheritedRoutes)
{
  for (const RibUpdate& update : inheritedRoutes) {
    auto ribIt = m_rib.find(update.getName());
    BOOST_ASSERT(ribIt != m_rib.end());
    shared_ptr<RibEntry> entry(ribIt->second);

    switch (update.getAction()) {
    case RibUpdate::REGISTER:
      entry->addInheritedRoute(update.getRoute());
      break;
    case RibUpdate::UNREGISTER:
      entry->removeInheritedRoute(update.getRoute());
      break;
    case RibUpdate::REMOVE_FACE:
      break;
    }
  }
}

std::ostream&
operator<<(std::ostream& os, const Rib& rib)
{
  for (const auto& item : rib) {
    os << *item.second << "\n";
  }

  return os;
}

} // namespace rib
} // namespace nfd
