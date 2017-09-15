/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#include "name-tree.hpp"
#include "core/logger.hpp"

#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>
#include <type_traits>

namespace nfd {
namespace name_tree {

NFD_LOG_INIT("NameTree");

NameTree::NameTree(size_t nBuckets)
  : m_ht(HashtableOptions(nBuckets))
{
}

Entry&
NameTree::lookup(const Name& name, bool enforceMaxDepth)
{
  NFD_LOG_TRACE("lookup " << name);
  size_t depth = enforceMaxDepth ? std::min(name.size(), getMaxDepth()) : name.size();

  HashSequence hashes = computeHashes(name, depth);
  const Node* node = nullptr;
  Entry* parent = nullptr;

  for (size_t prefixLen = 0; prefixLen <= depth; ++prefixLen) {
    bool isNew = false;
    std::tie(node, isNew) = m_ht.insert(name, prefixLen, hashes);

    if (isNew && parent != nullptr) {
      node->entry.setParent(*parent);
    }
    parent = &node->entry;
  }
  return node->entry;
}

Entry&
NameTree::lookup(const fib::Entry& fibEntry)
{
  Entry* nte = this->getEntry(fibEntry);
  if (nte == nullptr) {
    // special case: Fib::s_emptyEntry is unattached
    BOOST_ASSERT(fibEntry.getPrefix().empty());
    return this->lookup(fibEntry.getPrefix());
  }

  BOOST_ASSERT(nte->getFibEntry() == &fibEntry);
  return *nte;
}

Entry&
NameTree::lookup(const pit::Entry& pitEntry)
{
  Entry* nte = this->getEntry(pitEntry);
  BOOST_ASSERT(nte != nullptr);

  BOOST_ASSERT(std::count_if(nte->getPitEntries().begin(), nte->getPitEntries().end(),
    [&pitEntry] (const shared_ptr<pit::Entry>& pitEntry1) {
      return pitEntry1.get() == &pitEntry;
    }) == 1);

  if (nte->getName().size() == pitEntry.getName().size()) {
    return *nte;
  }

  // special case: PIT entry whose Interest name ends with an implicit digest
  // are attached to the name tree entry with one-shorter-prefix.
  BOOST_ASSERT(pitEntry.getName().at(-1).isImplicitSha256Digest());
  BOOST_ASSERT(nte->getName() == pitEntry.getName().getPrefix(-1));
  return this->lookup(pitEntry.getName());
}

Entry&
NameTree::lookup(const measurements::Entry& measurementsEntry)
{
  Entry* nte = this->getEntry(measurementsEntry);
  BOOST_ASSERT(nte != nullptr);

  BOOST_ASSERT(nte->getMeasurementsEntry() == &measurementsEntry);
  return *nte;
}

Entry&
NameTree::lookup(const strategy_choice::Entry& strategyChoiceEntry)
{
  Entry* nte = this->getEntry(strategyChoiceEntry);
  BOOST_ASSERT(nte != nullptr);

  BOOST_ASSERT(nte->getStrategyChoiceEntry() == &strategyChoiceEntry);
  return *nte;
}

size_t
NameTree::eraseIfEmpty(Entry* entry, bool canEraseAncestors)
{
  BOOST_ASSERT(entry != nullptr);

  size_t nErased = 0;
  for (Entry* parent = nullptr; entry != nullptr && entry->isEmpty(); entry = parent) {
    parent = entry->getParent();

    if (parent != nullptr) {
      entry->unsetParent();
    }

    m_ht.erase(getNode(*entry));
    ++nErased;

    if (!canEraseAncestors) {
      break;
    }
  }

  if (nErased == 0) {
    NFD_LOG_TRACE("not-erase " << entry->getName());
  }
  return nErased;
}

Entry*
NameTree::findExactMatch(const Name& name, size_t prefixLen) const
{
  const Node* node = m_ht.find(name, std::min(name.size(), prefixLen));
  return node == nullptr ? nullptr : &node->entry;
}

Entry*
NameTree::findLongestPrefixMatch(const Name& name, const EntrySelector& entrySelector) const
{
  HashSequence hashes = computeHashes(name);

  for (ssize_t prefixLen = name.size(); prefixLen >= 0; --prefixLen) {
    const Node* node = m_ht.find(name, prefixLen, hashes);
    if (node != nullptr && entrySelector(node->entry)) {
      return &node->entry;
    }
  }

  return nullptr;
}

Entry*
NameTree::findLongestPrefixMatch(const Entry& entry1, const EntrySelector& entrySelector) const
{
  Entry* entry = const_cast<Entry*>(&entry1);
  while (entry != nullptr) {
    if (entrySelector(*entry)) {
      return entry;
    }
    entry = entry->getParent();
  }
  return nullptr;
}

Entry*
NameTree::findLongestPrefixMatch(const pit::Entry& pitEntry, const EntrySelector& entrySelector) const
{
  const Entry* nte = this->getEntry(pitEntry);
  BOOST_ASSERT(nte != nullptr);

  // PIT entry Interest name either exceeds depth limit or ends with an implicit digest: go deeper
  if (nte->getName().size() < pitEntry.getName().size()) {
    for (size_t prefixLen = nte->getName().size() + 1; prefixLen <= pitEntry.getName().size(); ++prefixLen) {
      const Entry* exact = this->findExactMatch(pitEntry.getName(), prefixLen);
      if (exact == nullptr) {
        break;
      }
      nte = exact;
    }
  }

  return this->findLongestPrefixMatch(*nte, entrySelector);
}

boost::iterator_range<NameTree::const_iterator>
NameTree::findAllMatches(const Name& name, const EntrySelector& entrySelector) const
{
  // As we are using Name Prefix Hash Table, and the current LPM() is
  // implemented as starting from full name, and reduce the number of
  // components by 1 each time, we could use it here.
  // For trie-like design, it could be more efficient by walking down the
  // trie from the root node.

  Entry* entry = this->findLongestPrefixMatch(name, entrySelector);
  return {Iterator(make_shared<PrefixMatchImpl>(*this, entrySelector), entry), end()};
}

boost::iterator_range<NameTree::const_iterator>
NameTree::fullEnumerate(const EntrySelector& entrySelector) const
{
  return {Iterator(make_shared<FullEnumerationImpl>(*this, entrySelector), nullptr), end()};
}

boost::iterator_range<NameTree::const_iterator>
NameTree::partialEnumerate(const Name& prefix,
                           const EntrySubTreeSelector& entrySubTreeSelector) const
{
  Entry* entry = this->findExactMatch(prefix);
  return {Iterator(make_shared<PartialEnumerationImpl>(*this, entrySubTreeSelector), entry), end()};
}

} // namespace name_tree
} // namespace nfd
