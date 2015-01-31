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

#include "core/logger.hpp"
#include "propagated-entry.hpp"

namespace nfd {
namespace rib {

void
operator<<(std::ostream& out, PropagationStatus status)
{
  switch (status) {
  case PropagationStatus::NEW:
    out << "NEW";
    break;
  case PropagationStatus::PROPAGATING:
    out << "PROPAGATING";
    break;
  case PropagationStatus::PROPAGATED:
    out << "PROPAGATED";
    break;
  case PropagationStatus::PROPAGATE_FAIL:
    out << "PROPAGATE_FAIL";
    break;
  default:
    out << "undefined status";
    break;
  }
}

PropagatedEntry::PropagatedEntry()
  : m_propagationStatus(PropagationStatus::NEW)
{
}

PropagatedEntry::PropagatedEntry(const PropagatedEntry& other)
  : m_signingIdentity(other.m_signingIdentity)
  , m_propagationStatus(other.m_propagationStatus)
{
  BOOST_ASSERT(!other.isPropagated() && !other.isPropagateFail());
}

PropagatedEntry&
PropagatedEntry::setSigningIdentity(const Name& identity)
{
  m_signingIdentity = identity;
  return *this;
}

const Name&
PropagatedEntry::getSigningIdentity() const
{
  return m_signingIdentity;
}

void
PropagatedEntry::startPropagation()
{
  m_propagationStatus = PropagationStatus::PROPAGATING;
}

void
PropagatedEntry::succeed(const scheduler::EventId& event)
{
  m_propagationStatus = PropagationStatus::PROPAGATED;
  m_rePropagateEvent = event;
}

void
PropagatedEntry::fail(const scheduler::EventId& event)
{
  m_propagationStatus = PropagationStatus::PROPAGATE_FAIL;
  m_rePropagateEvent = event;
}

void
PropagatedEntry::initialize()
{
  m_propagationStatus = PropagationStatus::NEW;
  m_rePropagateEvent.cancel();
}

bool
PropagatedEntry::isNew() const
{
  return PropagationStatus::NEW == m_propagationStatus;
}

bool
PropagatedEntry::isPropagating() const
{
  return PropagationStatus::PROPAGATING == m_propagationStatus;
}

bool
PropagatedEntry::isPropagated() const
{
  return PropagationStatus::PROPAGATED == m_propagationStatus;
}

bool
PropagatedEntry::isPropagateFail() const
{
  return PropagationStatus::PROPAGATE_FAIL == m_propagationStatus;
}

} // namespace rib
} // namespace nfd
