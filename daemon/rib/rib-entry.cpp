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

#include "rib-entry.hpp"
#include "common/logger.hpp"

#include <ndn-cxx/mgmt/nfd/control-command.hpp>

namespace nfd {
namespace rib {

NFD_LOG_INIT(RibEntry);

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

std::pair<RibEntry::iterator, bool>
RibEntry::insertRoute(const Route& route)
{
  iterator it = findRoute(route);

  if (it == end()) {
    if (route.flags & ndn::nfd::ROUTE_FLAG_CAPTURE) {
      m_nRoutesWithCaptureSet++;
    }

    m_routes.push_back(route);
    return {std::prev(m_routes.end()), true};
  }

  return {it, false};
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
  BOOST_ASSERT(!child->getParent());
  child->setParent(this->shared_from_this());
  m_children.push_back(std::move(child));
}

void
RibEntry::removeChild(shared_ptr<RibEntry> child)
{
  BOOST_ASSERT(child->getParent().get() == this);
  child->setParent(nullptr);
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
    NFD_LOG_TRACE("Cancelling expiration event: " << route->getExpirationEvent());
    route->cancelExpirationEvent();

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
  m_inheritedRoutes.remove_if(bind(&compareFaceId, _1, route.faceId));
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
  return findInheritedRoute(route) != m_inheritedRoutes.end();
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
  std::vector<const Route*> matches;

  // Copy routes which have faceId
  for (const Route& route : m_routes) {
    if (route.faceId == faceId) {
      matches.push_back(&route);
    }
  }

  // If there are less than 2 routes, there is no second lowest
  if (matches.size() < 2) {
    return nullptr;
  }

  // Get second lowest cost
  std::nth_element(matches.begin(), matches.begin() + 1, matches.end(),
    [] (const Route* lhs, const Route* rhs) { return lhs->cost < rhs->cost; });

  return matches.at(1);
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

ndn::PrefixAnnouncement
RibEntry::getPrefixAnnouncement(time::milliseconds minExpiration,
                                time::milliseconds maxExpiration) const
{
  const Route* bestAnnRoute = nullptr;
  auto entryExpiry = time::steady_clock::TimePoint::min();

  for (const Route& route : *this) {
    if (route.expires) {
      entryExpiry = std::max(entryExpiry, *route.expires);
      if (route.announcement) {
        if (bestAnnRoute == nullptr || *route.expires > *bestAnnRoute->expires) {
          bestAnnRoute = &route;
        }
      }
    }
    else {
      entryExpiry = time::steady_clock::TimePoint::max();
    }
  }

  if (bestAnnRoute != nullptr) {
    return *bestAnnRoute->announcement;
  }

  ndn::PrefixAnnouncement ann;
  ann.setAnnouncedName(m_name);
  ann.setExpiration(ndn::clamp(
    time::duration_cast<time::milliseconds>(entryExpiry - time::steady_clock::now()),
    minExpiration, maxExpiration));
  return ann;
}

std::ostream&
operator<<(std::ostream& os, const RibEntry& entry)
{
  os << "RibEntry {\n";
  os << "  Name: " << entry.getName() << "\n";

  for (const Route& route : entry) {
    os << "  " << route << "\n";
  }

  os << "}";

  return os;
}

} // namespace rib
} // namespace nfd
