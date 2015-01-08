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

#include "cs-entry.hpp"

namespace nfd {
namespace cs {

Entry::Entry(const Name& name)
  : m_queryName(name)
{
  BOOST_ASSERT(this->isQuery());
}

Entry::Entry(shared_ptr<const Data> data, bool isUnsolicited)
  : queueType(Cs::QUEUE_NONE)
  , m_data(data)
  , m_isUnsolicited(isUnsolicited)
{
  BOOST_ASSERT(!this->isQuery());
}

bool
Entry::isQuery() const
{
  return m_data == nullptr;
}

shared_ptr<const Data>
Entry::getData() const
{
  BOOST_ASSERT(!this->isQuery());
  return m_data;
}

const Name&
Entry::getName() const
{
  BOOST_ASSERT(!this->isQuery());
  return m_data->getName();
}

const Name&
Entry::getFullName() const
{
  BOOST_ASSERT(!this->isQuery());
  return m_data->getFullName();
}

bool
Entry::canStale() const
{
  BOOST_ASSERT(!this->isQuery());
  return m_data->getFreshnessPeriod() >= time::milliseconds::zero();
}

bool
Entry::isStale() const
{
  BOOST_ASSERT(!this->isQuery());
  return time::steady_clock::now() > m_staleAt;
}

void
Entry::refresh()
{
  BOOST_ASSERT(!this->isQuery());
  if (this->canStale()) {
    m_staleAt = time::steady_clock::now() + time::milliseconds(m_data->getFreshnessPeriod());
  }
  else {
    m_staleAt = time::steady_clock::TimePoint::max();
  }
}

bool
Entry::isUnsolicited() const
{
  BOOST_ASSERT(!this->isQuery());
  return m_isUnsolicited;
}

void
Entry::unsetUnsolicited()
{
  BOOST_ASSERT(!this->isQuery());
  m_isUnsolicited = false;
}

bool
Entry::canSatisfy(const Interest& interest) const
{
  BOOST_ASSERT(!this->isQuery());
  if (!interest.matchesData(*m_data)) {
    return false;
  }

  if (interest.getMustBeFresh() == static_cast<int>(true) && this->isStale()) {
    return false;
  }

  return true;
}

bool
Entry::operator<(const Entry& other) const
{
  if (this->isQuery()) {
    if (other.isQuery()) {
      return m_queryName < other.m_queryName;
    }
    else {
      return m_queryName < other.m_queryName;
    }
  }
  else {
    if (other.isQuery()) {
      return this->getFullName() < other.m_queryName;
    }
    else {
      return this->getFullName() < other.getFullName();
    }
  }
}

} // namespace cs
} // namespace nfd
