/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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

Pit::Pit(NameTree& nameTree)
  : m_nameTree(nameTree)
  , m_nItems(0)
{
}

Pit::~Pit()
{
}

static inline bool
predicate_NameTreeEntry_hasPitEntry(const name_tree::Entry& entry)
{
  return entry.hasPitEntries();
}

static inline bool
predicate_PitEntry_similar_Interest(const shared_ptr<pit::Entry>& entry,
                                    const Interest& interest)
{
  const Interest& pi = entry->getInterest();
  return pi.getName().equals(interest.getName()) &&
         pi.getMinSuffixComponents() == interest.getMinSuffixComponents() &&
         pi.getMaxSuffixComponents() == interest.getMaxSuffixComponents() &&
         pi.getPublisherPublicKeyLocator() == interest.getPublisherPublicKeyLocator() &&
         pi.getExclude() == interest.getExclude() &&
         pi.getChildSelector() == interest.getChildSelector() &&
         pi.getMustBeFresh() == interest.getMustBeFresh();
}

std::pair<shared_ptr<pit::Entry>, bool>
Pit::insert(const Interest& interest)
{
  // - first lookup() the Interest Name in the NameTree, which will creates all
  // the intermedia nodes, starting from the shortest prefix.
  // - if it is guaranteed that this Interest already has a NameTree Entry, we
  // could use findExactMatch() instead.
  // - Alternatively, we could try to do findExactMatch() first, if not found,
  // then do lookup().
  shared_ptr<name_tree::Entry> nameTreeEntry = m_nameTree.lookup(interest.getName());
  BOOST_ASSERT(static_cast<bool>(nameTreeEntry));

  const std::vector<shared_ptr<pit::Entry> >& pitEntries = nameTreeEntry->getPitEntries();

  // then check if this Interest is already in the PIT entries
  std::vector<shared_ptr<pit::Entry> >::const_iterator it =
    std::find_if(pitEntries.begin(), pitEntries.end(),
                 bind(&predicate_PitEntry_similar_Interest, _1, cref(interest)));

  if (it != pitEntries.end())
    {
      return std::make_pair(*it, false);
    }
  else
    {
      shared_ptr<pit::Entry> entry = make_shared<pit::Entry>(interest);
      nameTreeEntry->insertPitEntry(entry);

      // Increase m_nItmes only if we create a new PIT Entry
      m_nItems++;

      return std::make_pair(entry, true);
    }
}

shared_ptr<pit::DataMatchResult>
Pit::findAllDataMatches(const Data& data) const
{
  shared_ptr<pit::DataMatchResult> result = make_shared<pit::DataMatchResult>();

  for (NameTree::const_iterator it =
       m_nameTree.findAllMatches(data.getName(), &predicate_NameTreeEntry_hasPitEntry);
       it != m_nameTree.end(); it++)
    {
      const std::vector<shared_ptr<pit::Entry> >& pitEntries = it->getPitEntries();
      for (size_t i = 0; i < pitEntries.size(); i++)
        {
          if (pitEntries[i]->getInterest().matchesData(data))
            result->push_back(pitEntries[i]);
        }
    }

  return result;
}

void
Pit::erase(shared_ptr<pit::Entry> pitEntry)
{
  shared_ptr<name_tree::Entry> nameTreeEntry = m_nameTree.get(*pitEntry);
  BOOST_ASSERT(static_cast<bool>(nameTreeEntry));

  nameTreeEntry->erasePitEntry(pitEntry);
  m_nameTree.eraseEntryIfEmpty(nameTreeEntry);

  --m_nItems;
}

} // namespace nfd
