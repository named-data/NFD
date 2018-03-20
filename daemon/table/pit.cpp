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

#include "pit.hpp"

namespace nfd {
namespace pit {

static inline bool
nteHasPitEntries(const name_tree::Entry& nte)
{
  return nte.hasPitEntries();
}

Pit::Pit(NameTree& nameTree)
  : m_nameTree(nameTree)
  , m_nItems(0)
{
}

std::pair<shared_ptr<Entry>, bool>
Pit::findOrInsert(const Interest& interest, bool allowInsert)
{
  // determine which NameTree entry should the PIT entry be attached onto
  const Name& name = interest.getName();
  bool hasDigest = name.size() > 0 && name[-1].isImplicitSha256Digest();
  size_t nteDepth = name.size() - static_cast<int>(hasDigest);
  nteDepth = std::min(nteDepth, NameTree::getMaxDepth());

  // ensure NameTree entry exists
  name_tree::Entry* nte = nullptr;
  if (allowInsert) {
    nte = &m_nameTree.lookup(name, nteDepth);
  }
  else {
    nte = m_nameTree.findExactMatch(name, nteDepth);
    if (nte == nullptr) {
      return {nullptr, true};
    }
  }

  // check if PIT entry already exists
  const auto& pitEntries = nte->getPitEntries();
  auto it = std::find_if(pitEntries.begin(), pitEntries.end(),
    [&interest, nteDepth] (const shared_ptr<Entry>& entry) {
      // NameTree guarantees first nteDepth components are equal
      return entry->canMatch(interest, nteDepth);
    });
  if (it != pitEntries.end()) {
    return {*it, false};
  }

  if (!allowInsert) {
    BOOST_ASSERT(!nte->isEmpty()); // nte shouldn't be created in this call
    return {nullptr, true};
  }

  auto entry = make_shared<Entry>(interest);
  nte->insertPitEntry(entry);
  ++m_nItems;
  return {entry, true};
}

DataMatchResult
Pit::findAllDataMatches(const Data& data) const
{
  auto&& ntMatches = m_nameTree.findAllMatches(data.getName(), &nteHasPitEntries);

  DataMatchResult matches;
  for (const name_tree::Entry& nte : ntMatches) {
    for (const shared_ptr<Entry>& pitEntry : nte.getPitEntries()) {
      if (pitEntry->getInterest().matchesData(data))
        matches.emplace_back(pitEntry);
    }
  }

  return matches;
}

void
Pit::erase(Entry* entry, bool canDeleteNte)
{
  name_tree::Entry* nte = m_nameTree.getEntry(*entry);
  BOOST_ASSERT(nte != nullptr);

  nte->erasePitEntry(entry);
  if (canDeleteNte) {
    m_nameTree.eraseIfEmpty(nte);
  }
  --m_nItems;
}

void
Pit::deleteInOutRecords(Entry* entry, const Face& face)
{
  BOOST_ASSERT(entry != nullptr);

  entry->deleteInRecord(face);
  entry->deleteOutRecord(face);

  /// \todo decide whether to delete PIT entry if there's no more in/out-record left
}

Pit::const_iterator
Pit::begin() const
{
  return const_iterator(m_nameTree.fullEnumerate(&nteHasPitEntries).begin());
}

} // namespace pit
} // namespace nfd
