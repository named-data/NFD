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

#include "rib-entry.hpp"

#include "core/logger.hpp"

#include <ndn-cxx/management/nfd-control-command.hpp>

NFD_LOG_INIT("RibEntry");

namespace nfd {
namespace rib {

RibEntry::RouteList::iterator
RibEntry::findRoute(const Route& route)
{
  return std::find_if(begin(), end(), bind(&compareFaceIdAndOrigin, _1, route));
}

RibEntry::RouteList::const_iterator
RibEntry::findRoute(const Route& route) const
{
  return std::find_if(begin(), end(), bind(&compareFaceIdAndOrigin, _1, route));
}

bool
RibEntry::insertRoute(const Route& route)
{
  iterator it = findRoute(route);

  if (it == end()) {
    if (route.flags & ndn::nfd::ROUTE_FLAG_CAPTURE) {
      m_nRoutesWithCaptureSet++;
    }

    m_routes.push_back(route);

    return true;
  }
  else {
    return false;
  }
}

void
RibEntry::eraseRoute(const Route& route)
{
  RibEntry::iterator it = findRoute(route);
  eraseRoute(it);
}

bool
RibEntry::hasRoute(const Route& route)
{
  RibEntry::const_iterator it = findRoute(route);

  return it != end();
}

bool
RibEntry::hasFaceId(const uint64_t faceId) const
{
  RibEntry::const_iterator it = std::find_if(begin(), end(), bind(&compareFaceId, _1, faceId));

  return it != end();
}

size_t
RibEntry::getNRoutes() const
{
  return m_routes.size();
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

RibEntry::RouteList::iterator
RibEntry::eraseRoute(RouteList::iterator route)
{
  if (route != m_routes.end()) {
    if (route->flags & ndn::nfd::ROUTE_FLAG_CAPTURE) {
      m_nRoutesWithCaptureSet--;
    }

    // Cancel any scheduled event
    NFD_LOG_TRACE("Cancelling expiration eventId: " << route->getExpirationEvent());
    scheduler::cancel(route->getExpirationEvent());

    return m_routes.erase(route);
  }

  return m_routes.end();
}

void
RibEntry::addInheritedRoute(const Route& route)
{
  m_inheritedRoutes.push_back(route);
}

void
RibEntry::removeInheritedRoute(const Route& route)
{
  RouteList::iterator it = std::find_if(m_inheritedRoutes.begin(), m_inheritedRoutes.end(),
                                        bind(&compareFaceId, _1, route.faceId));
  m_inheritedRoutes.erase(it);
}

RibEntry::RouteList::const_iterator
RibEntry::findInheritedRoute(const Route& route) const
{
  return std::find_if(m_inheritedRoutes.begin(), m_inheritedRoutes.end(),
                      bind(&compareFaceId, _1, route.faceId));
}

bool
RibEntry::hasInheritedRoute(const Route& route) const
{
  RouteList::const_iterator it = findInheritedRoute(route);

  return (it != m_inheritedRoutes.end());
}

bool
RibEntry::hasCapture() const
{
  return m_nRoutesWithCaptureSet > 0;
}

bool
RibEntry::hasChildInheritOnFaceId(uint64_t faceId) const
{
  for (const Route& route : m_routes) {
    if (route.faceId == faceId && (route.flags & ndn::nfd::ROUTE_FLAG_CHILD_INHERIT)) {
      return true;
    }
  }

  return false;
}

const Route*
RibEntry::getRouteWithLowestCostByFaceId(uint64_t faceId) const
{
  const Route* candidate = nullptr;

  for (const Route& route : m_routes) {
    // Matching face ID
    if (route.faceId == faceId) {
      // If this is the first route with this Face ID found
      if (candidate == nullptr) {
        candidate = &route;
      }
      else if (route.cost < candidate->cost) {
        // Found a route with a lower cost
        candidate = &route;
      }
    }
  }

  return candidate;
}

const Route*
RibEntry::getRouteWithSecondLowestCostByFaceId(uint64_t faceId) const
{
  std::vector<Route> matches;

  // Copy routes which have faceId
  std::copy_if(m_routes.begin(), m_routes.end(), std::back_inserter(matches),
    [faceId] (const Route& route) { return route.faceId == faceId; });

  // If there are not at least 2 routes, there is no second lowest
  if (matches.size() < 2) {
    return nullptr;
  }

  // Get second lowest cost
  std::nth_element(matches.begin(), matches.begin() + 1, matches.end(),
    [] (const Route& lhs, const Route& rhs) { return lhs.cost < rhs.cost; });

  return &matches.at(1);
}

const Route*
RibEntry::getRouteWithLowestCostAndChildInheritByFaceId(uint64_t faceId) const
{
  const Route* candidate = nullptr;

  for (const Route& route : m_routes) {
    // Correct face ID and Child Inherit flag set
    if (route.faceId == faceId &&
        (route.flags & ndn::nfd::ROUTE_FLAG_CHILD_INHERIT) == ndn::nfd::ROUTE_FLAG_CHILD_INHERIT)
    {
      // If this is the first route with this Face ID found
      if (candidate == nullptr) {
        candidate = &route;
      }
      else if (route.cost < candidate->cost) {
        // Found a route with a lower cost
        candidate = &route;
      }
    }
  }

  return candidate;
}

std::ostream&
operator<<(std::ostream& os, const RibEntry& entry)
{
  os << "RibEntry {\n";
  os << "\tName: " << entry.getName() << "\n";

  for (const Route& route : entry) {
    os << "\t" << route << "\n";
  }

  os << "}";

  return os;
}

} // namespace rib
} // namespace nfd
