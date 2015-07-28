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

#include "cs-policy-lru.hpp"
#include "cs.hpp"
#include <ndn-cxx/util/signal.hpp>

namespace nfd {
namespace cs {
namespace lru {

const std::string LruPolicy::POLICY_NAME = "lru";

LruPolicy::LruPolicy()
  : Policy(POLICY_NAME)
{
}

void
LruPolicy::doAfterInsert(iterator i)
{
  this->insertToQueue(i, true);
  this->evictEntries();
}

void
LruPolicy::doAfterRefresh(iterator i)
{
  this->insertToQueue(i, false);
}

void
LruPolicy::doBeforeErase(iterator i)
{
  m_queue.get<1>().erase(i);
}

void
LruPolicy::doBeforeUse(iterator i)
{
  this->insertToQueue(i, false);
}

void
LruPolicy::evictEntries()
{
  BOOST_ASSERT(this->getCs() != nullptr);
  while (this->getCs()->size() > this->getLimit()) {
    BOOST_ASSERT(!m_queue.empty());
    iterator i = m_queue.front();
    m_queue.pop_front();
    this->emitSignal(beforeEvict, i);
  }
}

void
LruPolicy::insertToQueue(iterator i, bool isNewEntry)
{
  Queue::iterator it;
  bool isNew = false;
  // push_back only if iterator i does not exist
  std::tie(it, isNew) = m_queue.push_back(i);

  BOOST_ASSERT(isNew == isNewEntry);
  if (!isNewEntry) {
    m_queue.relocate(m_queue.end(), it);
  }
}

} // namespace lru
} // namespace cs
} // namespace nfd
