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

#ifndef NFD_RIB_RIB_HPP
#define NFD_RIB_RIB_HPP

#include "rib-entry.hpp"
#include "rib-update-batch.hpp"

#include <ndn-cxx/mgmt/nfd/control-parameters.hpp>

namespace nfd {
namespace rib {

using ndn::nfd::ControlParameters;

class FibUpdater;

  /** \brief references a route
   */
struct RibRouteRef
{
  shared_ptr<RibEntry> entry;
  RibEntry::const_iterator route;
};

bool
operator<(const RibRouteRef& lhs, const RibRouteRef& rhs);

/** \brief represents the Routing Information Base

    The Routing Information Base contains a collection of Routes, each
    represents a piece of static or dynamic routing information
    registered by applications, operators, or NFD itself. Routes
    associated with the same namespace are collected into a RIB entry.
 */
class Rib : noncopyable
{
public:
  typedef std::list<shared_ptr<RibEntry>> RibEntryList;
  typedef std::map<Name, shared_ptr<RibEntry>> RibTable;
  typedef RibTable::const_iterator const_iterator;
  typedef std::map<uint64_t, std::list<shared_ptr<RibEntry>>> FaceLookupTable;
  typedef bool (*RouteComparePredicate)(const Route&, const Route&);
  typedef std::set<Route, RouteComparePredicate> RouteSet;

  Rib();

  ~Rib();

  void
  setFibUpdater(FibUpdater* updater);

  const_iterator
  find(const Name& prefix) const;

  Route*
  find(const Name& prefix, const Route& route) const;

  const_iterator
  begin() const;

  const_iterator
  end() const;

  size_t
  size() const;

  bool
  empty() const;

  shared_ptr<RibEntry>
  findParent(const Name& prefix) const;

  /** \brief finds namespaces under the passed prefix
   *  \return{ a list of entries which are under the passed prefix }
   */
  std::list<shared_ptr<RibEntry>>
  findDescendants(const Name& prefix) const;

  /** \brief finds namespaces under the passed prefix
   *
   *  \note Unlike findDescendants, needs to find where prefix would fit in tree
   *  before collecting list of descendant prefixes
   *
   *  \return{ a list of entries which would be under the passed prefix if the prefix
   *  existed in the RIB }
   */
  std::list<shared_ptr<RibEntry>>
  findDescendantsForNonInsertedName(const Name& prefix) const;

public:
  using UpdateSuccessCallback = std::function<void()>;
  using UpdateFailureCallback = std::function<void(uint32_t code, const std::string& error)>;

  /** \brief passes the provided RibUpdateBatch to FibUpdater to calculate and send FibUpdates.
   *
   *  If the FIB is updated successfully, onFibUpdateSuccess() will be called, and the
   *  RIB will be updated
   *
   *  If the FIB update fails, onFibUpdateFailure() will be called, and the RIB will not
   *  be updated.
   */
  void
  beginApplyUpdate(const RibUpdate& update,
                   const UpdateSuccessCallback& onSuccess,
                   const UpdateFailureCallback& onFailure);

  /** \brief starts the FIB update process when a face has been destroyed
   */
  void
  beginRemoveFace(uint64_t faceId);

  void
  onFibUpdateSuccess(const RibUpdateBatch& batch,
                     const RibUpdateList& inheritedRoutes,
                     const Rib::UpdateSuccessCallback& onSuccess);

  void
  onFibUpdateFailure(const Rib::UpdateFailureCallback& onFailure,
                     uint32_t code, const std::string& error);

  void
  onRouteExpiration(const Name& prefix, const Route& route);

  void
  insert(const Name& prefix, const Route& route);

private:
  /** \brief adds the passed update to a RibUpdateBatch and adds the batch to
  *          the end of the update queue.
  *
  *   If an update is not in progress, the front update batch in the queue will be
  *   processed by the RIB.
  *
  *   If an update is in progress, the added update will eventually be processed
  *   when it reaches the front of the queue; after other update batches are
  *   processed, the queue is advanced.
  */
  void
  addUpdateToQueue(const RibUpdate& update,
                   const Rib::UpdateSuccessCallback& onSuccess,
                   const Rib::UpdateFailureCallback& onFailure);

  /** \brief Attempts to send the front update batch in the queue.
  *
  *   If an update is not in progress, the front update batch in the queue will be
  *   sent to the RIB for processing.
  *
  *   If an update is in progress, nothing will be done.
  */
  void
  sendBatchFromQueue();

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  // Used by RibManager unit-tests to get sent batch to simulate successful FIB update
  std::function<void(RibUpdateBatch)> m_onSendBatchFromQueue;

  void
  erase(const Name& prefix, const Route& route);

private:
  RibTable::iterator
  eraseEntry(RibTable::iterator it);

  void
  updateRib(const RibUpdateBatch& batch);

  /** \brief returns routes inherited from the entry's ancestors
   *
   *  \return{ a list of inherited routes }
   */
  RouteSet
  getAncestorRoutes(const RibEntry& entry) const;

  /** \brief returns routes inherited from the parent of the name and the parent's ancestors
   *
   *  \note A parent is first found for the passed name before inherited routes are collected
   *
   *  \return{ a list of inherited routes }
   */
  RouteSet
  getAncestorRoutes(const Name& name) const;

  /** \brief applies the passed inheritedRoutes and their actions to the corresponding RibEntries'
   *  inheritedRoutes lists
   */
  void
  modifyInheritedRoutes(const RibUpdateList& inheritedRoutes);

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  typedef std::pair<const Name&,const Route&> NameAndRoute;

  std::list<NameAndRoute>
  findRoutesWithFaceId(uint64_t faceId);

public:
  /** \brief signals after a RIB entry is inserted
   *
   *  A RIB entry is inserted when the first route associated with a
   *  certain namespace is added.
   */
  ndn::util::signal::Signal<Rib, Name> afterInsertEntry;

  /** \brief signals after a RIB entry is erased
   *
   *  A RIB entry is erased when the last route associated with a
   *  certain namespace is removed.
   */

  ndn::util::signal::Signal<Rib, Name> afterEraseEntry;

  /** \brief signals after a Route is added
   */
  ndn::util::signal::Signal<Rib, RibRouteRef> afterAddRoute;

  /** \brief signals before a route is removed
   */
  ndn::util::signal::Signal<Rib, RibRouteRef> beforeRemoveRoute;

private:
  RibTable m_rib;
  FaceLookupTable m_faceMap;
  FibUpdater* m_fibUpdater;

  size_t m_nItems;

  friend class FibUpdater;

private:
  struct UpdateQueueItem
  {
    RibUpdateBatch batch;
    const Rib::UpdateSuccessCallback managerSuccessCallback;
    const Rib::UpdateFailureCallback managerFailureCallback;
  };

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  typedef std::list<UpdateQueueItem> UpdateQueue;
  UpdateQueue m_updateBatches;

private:
  bool m_isUpdateInProgress;
};

inline Rib::const_iterator
Rib::begin() const
{
  return m_rib.begin();
}

inline Rib::const_iterator
Rib::end() const
{
  return m_rib.end();
}

inline size_t
Rib::size() const
{
  return m_nItems;
}

inline bool
Rib::empty() const
{
  return m_rib.empty();
}

std::ostream&
operator<<(std::ostream& os, const Rib& rib);

} // namespace rib
} // namespace nfd

#endif // NFD_RIB_RIB_HPP
