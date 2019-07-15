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

#include "cs-entry.hpp"

namespace nfd {
namespace cs {

Entry::Entry(shared_ptr<const Data> data, bool isUnsolicited)
  : m_data(std::move(data))
  , m_isUnsolicited(isUnsolicited)
{
  updateFreshUntil();
}

bool
Entry::isFresh() const
{
  return m_freshUntil >= time::steady_clock::now();
}

void
Entry::updateFreshUntil()
{
  m_freshUntil = time::steady_clock::now() + m_data->getFreshnessPeriod();
}

bool
Entry::canSatisfy(const Interest& interest) const
{
  if (!interest.matchesData(*m_data)) {
    return false;
  }

  if (interest.getMustBeFresh() && !this->isFresh()) {
    return false;
  }

  return true;
}

static int
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

static int
compareDataWithData(const Data& lhs, const Data& rhs)
{
  int cmp = lhs.getName().compare(rhs.getName());
  if (cmp != 0) {
    return cmp;
  }

  return lhs.getFullName()[-1].compare(rhs.getFullName()[-1]);
}

bool
operator<(const Entry& entry, const Name& queryName)
{
  return compareQueryWithData(queryName, entry.getData()) > 0;
}

bool
operator<(const Name& queryName, const Entry& entry)
{
  return compareQueryWithData(queryName, entry.getData()) < 0;
}

bool
operator<(const Entry& lhs, const Entry& rhs)
{
  return compareDataWithData(lhs.getData(), rhs.getData()) < 0;
}

} // namespace cs
} // namespace nfd
