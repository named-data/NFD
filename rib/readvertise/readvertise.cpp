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

#include "readvertise.hpp"
#include "core/logger.hpp"
#include "core/random.hpp"

namespace nfd {
namespace rib {

NFD_LOG_INIT("Readvertise");

const time::milliseconds Readvertise::RETRY_DELAY_MIN = time::seconds(50);
const time::milliseconds Readvertise::RETRY_DELAY_MAX = time::seconds(3600);

static time::milliseconds
randomizeTimer(time::milliseconds baseTimer)
{
  std::uniform_int_distribution<uint64_t> dist(-5, 5);
  time::milliseconds newTime = baseTimer + time::milliseconds(dist(getGlobalRng()));
  return std::max(newTime, time::milliseconds(0));
}

Readvertise::Readvertise(Rib& rib, unique_ptr<ReadvertisePolicy> policy,
                         unique_ptr<ReadvertiseDestination> destination)
  : m_policy(std::move(policy))
  , m_destination(std::move(destination))
{
  m_addRouteConn = rib.afterAddRoute.connect(bind(&Readvertise::afterAddRoute, this, _1));
  m_removeRouteConn = rib.beforeRemoveRoute.connect(bind(&Readvertise::beforeRemoveRoute, this, _1));

  m_destination->afterAvailabilityChange.connect([this] (bool isAvailable) {
    if (isAvailable) {
      this->afterDestinationAvailable();
    }
    else {
      this->afterDestinationUnavailable();
    }
  });
}

void
Readvertise::afterAddRoute(const RibRouteRef& ribRoute)
{
  optional<ReadvertiseAction> action = m_policy->handleNewRoute(ribRoute);
  if (!action) {
    NFD_LOG_DEBUG("add-route " << ribRoute.entry->getName() << '(' << ribRoute.route->faceId <<
                  ',' << ribRoute.route->origin << ") not-readvertising");
    return;
  }

  ReadvertisedRouteContainer::iterator rrIt;
  bool isNew = false;
  std::tie(rrIt, isNew) = m_rrs.emplace(action->prefix);

  if (!isNew && rrIt->signer != action->signer) {
    NFD_LOG_WARN("add-route " << ribRoute.entry->getName() << '(' << ribRoute.route->faceId <<
                  ',' << ribRoute.route->origin << ") readvertising-as " << action->prefix <<
                 " old-signer " << rrIt->signer << " new-signer " << action->signer);
  }
  rrIt->signer = action->signer;

  RouteRrIndex::iterator indexIt;
  std::tie(indexIt, isNew) = m_routeToRr.emplace(ribRoute, rrIt);
  BOOST_ASSERT(isNew);

  if (rrIt->nRibRoutes++ > 0) {
    NFD_LOG_DEBUG("add-route " << ribRoute.entry->getName() << '(' << ribRoute.route->faceId <<
                  ',' << ribRoute.route->origin << ") already-readvertised-as " << action->prefix);
    return;
  }

  NFD_LOG_DEBUG("add-route " << ribRoute.entry->getName() << '(' << ribRoute.route->faceId <<
                ',' << ribRoute.route->origin << ") readvertising-as " << action->prefix <<
                " signer " << action->signer);
  rrIt->retryDelay = RETRY_DELAY_MIN;
  this->advertise(rrIt);
}

void
Readvertise::beforeRemoveRoute(const RibRouteRef& ribRoute)
{
  auto indexIt = m_routeToRr.find(ribRoute);
  if (indexIt == m_routeToRr.end()) {
    NFD_LOG_DEBUG("remove-route " << ribRoute.entry->getName() << '(' << ribRoute.route->faceId <<
                  ',' << ribRoute.route->origin << ") not-readvertised");
    return;
  }

  auto rrIt = indexIt->second;
  m_routeToRr.erase(indexIt);

  if (--rrIt->nRibRoutes > 0) {
    NFD_LOG_DEBUG("remove-route " << ribRoute.entry->getName() << '(' << ribRoute.route->faceId <<
                  ',' << ribRoute.route->origin << ") needed-by " << rrIt->nRibRoutes);
    return;
  }

  rrIt->retryDelay = RETRY_DELAY_MIN;
  this->withdraw(rrIt);
}

void
Readvertise::afterDestinationAvailable()
{
  for (auto rrIt = m_rrs.begin(); rrIt != m_rrs.end(); ++rrIt) {
    rrIt->retryDelay = RETRY_DELAY_MIN;
    this->advertise(rrIt);
  }
}

void
Readvertise::afterDestinationUnavailable()
{
  for (auto rrIt = m_rrs.begin(); rrIt != m_rrs.end();) {
    if (rrIt->nRibRoutes > 0) {
      rrIt->retryEvt.cancel(); // stop retrying or refreshing
      ++rrIt;
    }
    else {
      rrIt = m_rrs.erase(rrIt); // assume withdraw has completed
    }
  }
}

void
Readvertise::advertise(ReadvertisedRouteContainer::iterator rrIt)
{
  BOOST_ASSERT(rrIt->nRibRoutes > 0);

  if (!m_destination->isAvailable()) {
    NFD_LOG_DEBUG("advertise " << rrIt->prefix << " destination-unavailable");
    return;
  }

  m_destination->advertise(*rrIt,
    [this, rrIt] {
      NFD_LOG_DEBUG("advertise " << rrIt->prefix << " success");
      rrIt->retryDelay = RETRY_DELAY_MIN;
      rrIt->retryEvt = scheduler::schedule(randomizeTimer(m_policy->getRefreshInterval()),
                                           bind(&Readvertise::advertise, this, rrIt));
    },
    [this, rrIt] (const std::string& msg) {
      NFD_LOG_DEBUG("advertise " << rrIt->prefix << " failure " << msg);
      rrIt->retryDelay = std::min(RETRY_DELAY_MAX, rrIt->retryDelay * 2);
      rrIt->retryEvt = scheduler::schedule(randomizeTimer(rrIt->retryDelay),
                                           bind(&Readvertise::advertise, this, rrIt));
    });
}

void
Readvertise::withdraw(ReadvertisedRouteContainer::iterator rrIt)
{
  BOOST_ASSERT(rrIt->nRibRoutes == 0);

  if (!m_destination->isAvailable()) {
    NFD_LOG_DEBUG("withdraw " << rrIt->prefix << " destination-unavailable");
    m_rrs.erase(rrIt);
    return;
  }

  m_destination->withdraw(*rrIt,
    [this, rrIt] {
      NFD_LOG_DEBUG("withdraw " << rrIt->prefix << " success");
      m_rrs.erase(rrIt);
    },
    [this, rrIt] (const std::string& msg) {
      NFD_LOG_DEBUG("withdraw " << rrIt->prefix << " failure " << msg);
      rrIt->retryDelay = std::min(RETRY_DELAY_MAX, rrIt->retryDelay * 2);
      rrIt->retryEvt = scheduler::schedule(randomizeTimer(rrIt->retryDelay),
                                           bind(&Readvertise::withdraw, this, rrIt));
    });
}

} // namespace rib
} // namespace nfd
