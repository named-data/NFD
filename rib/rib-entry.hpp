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

#ifndef NFD_RIB_RIB_ENTRY_HPP
#define NFD_RIB_RIB_ENTRY_HPP

#include "route.hpp"

namespace nfd {
namespace rib {

/** \brief represents a RIB entry, which contains one or more Routes with the same prefix
 */
class RibEntry : public enable_shared_from_this<RibEntry>
{
public:
  typedef std::list<Route> RouteList;
  typedef RouteList::iterator iterator;
  typedef RouteList::const_iterator const_iterator;

  RibEntry()
    : m_nRoutesWithCaptureSet(0)
  {
  }

  void
  setName(const Name& prefix);

  const Name&
  getName() const;

  shared_ptr<RibEntry>
  getParent() const;

  bool
  hasParent() const;

  void
  addChild(shared_ptr<RibEntry> child);

  void
  removeChild(shared_ptr<RibEntry> child);

  const std::list<shared_ptr<RibEntry>>&
  getChildren() const;

  bool
  hasChildren() const;

  /** \brief inserts a new route into the entry's route list
   *  If another route already exists with the same faceId and origin,
   *  the new route is not inserted.
   *  \return{ true if the route is inserted, false otherwise }
   */
  bool
  insertRoute(const Route& route);

  /** \brief erases a Route with the same faceId and origin
   */
  void
  eraseRoute(const Route& route);

  /** \brief erases a Route with the passed iterator
   *  \return{ an iterator to the element that followed the erased iterator }
   */
  iterator
  eraseRoute(RouteList::iterator route);

  bool
  hasFaceId(const uint64_t faceId) const;

  RouteList&
  getRoutes();

  size_t
  getNRoutes() const;

  iterator
  findRoute(const Route& route);

  const_iterator
  findRoute(const Route& route) const;

  bool
  hasRoute(const Route& route);

  void
  addInheritedRoute(const Route& route);

  void
  removeInheritedRoute(const Route& route);

  /** \brief Returns the routes this namespace has inherited.
   *  The inherited routes returned represent inherited routes this namespace has in the FIB.
   *  \return{ routes inherited by this namespace }
   */
  const RouteList&
  getInheritedRoutes() const;

  /** \brief Finds an inherited route with a matching face ID.
   *  \return{ An iterator to the matching route if one is found;
   *           otherwise, an iterator to the end of the entry's
   *           inherited route list }
   */
  RouteList::const_iterator
  findInheritedRoute(const Route& route) const;

  /** \brief Determines if the entry has an inherited route with a matching face ID.
   *  \return{ True, if a matching inherited route is found; otherwise, false. }
   */
  bool
  hasInheritedRoute(const Route& route) const;

  bool
  hasCapture() const;

  /** \brief Determines if the entry has an inherited route with the passed
   *         face ID and its child inherit flag set.
   *  \return{ True, if a matching inherited route is found; otherwise, false. }
   */
  bool
  hasChildInheritOnFaceId(uint64_t faceId) const;

  /** \brief Returns the route with the lowest cost that has the passed face ID.
   *  \return{ The route with the lowest cost that has the passed face ID}
   */
  const Route*
  getRouteWithLowestCostByFaceId(uint64_t faceId) const;

  const Route*
  getRouteWithSecondLowestCostByFaceId(uint64_t faceId) const;

  /** \brief Returns the route with the lowest cost that has the passed face ID
   *         and its child inherit flag set.
   *  \return{ The route with the lowest cost that has the passed face ID
   *           and its child inherit flag set }
   */
  const Route*
  getRouteWithLowestCostAndChildInheritByFaceId(uint64_t faceId) const;

  const_iterator
  begin() const;

  const_iterator
  end() const;

  iterator
  begin();

  iterator
  end();

private:
  void
  setParent(shared_ptr<RibEntry> parent);

private:
  Name m_name;
  std::list<shared_ptr<RibEntry>> m_children;
  shared_ptr<RibEntry> m_parent;
  RouteList m_routes;
  RouteList m_inheritedRoutes;

  /** \brief The number of routes on this namespace with the capture flag set.
   *
   *  This count is used to check if the namespace will block inherited routes.
   *  If the number is greater than zero, a route on the namespace has it's capture
   *  flag set which means the namespace should not inherit any routes.
   */
  uint64_t m_nRoutesWithCaptureSet;
};

inline void
RibEntry::setName(const Name& prefix)
{
  m_name = prefix;
}

inline const Name&
RibEntry::getName() const
{
  return m_name;
}

inline void
RibEntry::setParent(shared_ptr<RibEntry> parent)
{
  m_parent = parent;
}

inline shared_ptr<RibEntry>
RibEntry::getParent() const
{
  return m_parent;
}

inline const std::list<shared_ptr<RibEntry> >&
RibEntry::getChildren() const
{
  return m_children;
}

inline RibEntry::RouteList&
RibEntry::getRoutes()
{
  return m_routes;
}

inline const RibEntry::RouteList&
RibEntry::getInheritedRoutes() const
{
  return m_inheritedRoutes;
}

inline RibEntry::const_iterator
RibEntry::begin() const
{
  return m_routes.begin();
}

inline RibEntry::const_iterator
RibEntry::end() const
{
  return m_routes.end();
}

inline RibEntry::iterator
RibEntry::begin()
{
  return m_routes.begin();
}

inline RibEntry::iterator
RibEntry::end()
{
  return m_routes.end();
}

std::ostream&
operator<<(std::ostream& os, const RibEntry& entry);

} // namespace rib
} // namespace nfd

#endif // NFD_RIB_RIB_ENTRY_HPP
