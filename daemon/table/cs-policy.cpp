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

#include "cs-policy.hpp"
#include "cs.hpp"

namespace nfd {
namespace cs {

Policy::Policy(const std::string& policyName)
  : m_policyName(policyName)
{
}

Policy::~Policy()
{
}

void
Policy::setLimit(size_t nMaxEntries)
{
  BOOST_ASSERT(nMaxEntries > 0);
  m_limit = nMaxEntries;

  this->evictEntries();
}

void
Policy::afterInsert(iterator i)
{
  BOOST_ASSERT(m_cs != nullptr);
  this->doAfterInsert(i);
}

void
Policy::afterRefresh(iterator i)
{
  BOOST_ASSERT(m_cs != nullptr);
  this->doAfterRefresh(i);
}

void
Policy::beforeErase(iterator i)
{
  BOOST_ASSERT(m_cs != nullptr);
  this->doBeforeErase(i);
}

void
Policy::beforeUse(iterator i)
{
  BOOST_ASSERT(m_cs != nullptr);
  this->doBeforeUse(i);
}

} // namespace cs
} // namespace nfd
