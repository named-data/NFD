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

#include "cs-entry-impl.hpp"

namespace nfd {
namespace cs {

EntryImpl::EntryImpl(const Name& name)
  : m_queryName(name)
{
  BOOST_ASSERT(this->isQuery());
}

EntryImpl::EntryImpl(shared_ptr<const Data> data, bool isUnsolicited)
{
  this->setData(data, isUnsolicited);
  BOOST_ASSERT(!this->isQuery());
}

bool
EntryImpl::isQuery() const
{
  return !this->hasData();
}

bool
EntryImpl::canStale() const
{
  BOOST_ASSERT(!this->isQuery());
  return this->getStaleTime() < time::steady_clock::TimePoint::max();
}

void
EntryImpl::unsetUnsolicited()
{
  BOOST_ASSERT(!this->isQuery());
  this->setData(this->getData(), false);
}

int
compareQueryWithData(const Name& queryName, const Data& data)
{
  bool queryIsFullName = !queryName.empty() && queryName[-1].isImplicitSha256Digest();

  int cmp = queryIsFullName ?
            queryName.compare(0, queryName.size() - 1, data.getName()) :
            queryName.compare(data.getName());

  if (cmp != 0) { // Name without digest differs
    return cmp;
  }

  if (queryIsFullName) { // Name without digest equals, compare digest
    return queryName[-1].compare(data.getFullName()[-1]);
  }
  else { // queryName is a proper prefix of Data fullName
    return -1;
  }
}

int
compareDataWithData(const Data& lhs, const Data& rhs)
{
  int cmp = lhs.getName().compare(rhs.getName());
  if (cmp != 0) {
    return cmp;
  }

  return lhs.getFullName()[-1].compare(rhs.getFullName()[-1]);
}

bool
EntryImpl::operator<(const EntryImpl& other) const
{
  if (this->isQuery()) {
    if (other.isQuery()) {
      return m_queryName < other.m_queryName;
    }
    else {
      return compareQueryWithData(m_queryName, other.getData()) < 0;
    }
  }
  else {
    if (other.isQuery()) {
      return compareQueryWithData(other.m_queryName, this->getData()) > 0;
    }
    else {
      return compareDataWithData(this->getData(), other.getData()) < 0;
    }
  }
}

} // namespace cs
} // namespace nfd
