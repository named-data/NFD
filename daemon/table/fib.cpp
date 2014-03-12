/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "fib.hpp"
#include "pit-entry.hpp"
#include "measurements-entry.hpp"

namespace nfd {

const shared_ptr<fib::Entry> Fib::s_emptyEntry = make_shared<fib::Entry>(Name());

Fib::Fib(NameTree& nameTree)
  : m_nameTree(nameTree)
  , m_nItems(0)
{
}

Fib::~Fib()
{
}

static inline bool
predicate_NameTreeEntry_hasFibEntry(const name_tree::Entry& entry)
{
  return static_cast<bool>(entry.getFibEntry());
}

std::pair<shared_ptr<fib::Entry>, bool>
Fib::insert(const Name& prefix)
{
  shared_ptr<name_tree::Entry> nameTreeEntry = m_nameTree.lookup(prefix);
  shared_ptr<fib::Entry> entry = nameTreeEntry->getFibEntry();
  if (static_cast<bool>(entry))
    return std::make_pair(entry, false);
  entry = make_shared<fib::Entry>(prefix);
  nameTreeEntry->setFibEntry(entry);
  ++m_nItems;
  return std::make_pair(entry, true);
}

shared_ptr<fib::Entry>
Fib::findLongestPrefixMatch(const Name& prefix) const
{
  shared_ptr<name_tree::Entry> nameTreeEntry =
    m_nameTree.findLongestPrefixMatch(prefix, &predicate_NameTreeEntry_hasFibEntry);
  if (static_cast<bool>(nameTreeEntry)) {
    return nameTreeEntry->getFibEntry();
  }
  return s_emptyEntry;
}

shared_ptr<fib::Entry>
Fib::findLongestPrefixMatch(shared_ptr<name_tree::Entry> nameTreeEntry) const
{
  shared_ptr<fib::Entry> entry = nameTreeEntry->getFibEntry();
  if (static_cast<bool>(entry))
    return entry;
  nameTreeEntry = m_nameTree.findLongestPrefixMatch(nameTreeEntry,
                                                    &predicate_NameTreeEntry_hasFibEntry);
  if (static_cast<bool>(nameTreeEntry)) {
    return nameTreeEntry->getFibEntry();
  }
  return s_emptyEntry;
}

shared_ptr<fib::Entry>
Fib::findLongestPrefixMatch(const pit::Entry& pitEntry) const
{
  shared_ptr<name_tree::Entry> nameTreeEntry = m_nameTree.get(pitEntry);

  BOOST_ASSERT(static_cast<bool>(nameTreeEntry));

  return findLongestPrefixMatch(nameTreeEntry);
}

shared_ptr<fib::Entry>
Fib::findLongestPrefixMatch(const measurements::Entry& measurementsEntry) const
{
  shared_ptr<name_tree::Entry> nameTreeEntry = m_nameTree.get(measurementsEntry);

  BOOST_ASSERT(static_cast<bool>(nameTreeEntry));

  return findLongestPrefixMatch(nameTreeEntry);
}

shared_ptr<fib::Entry>
Fib::findExactMatch(const Name& prefix) const
{
  shared_ptr<name_tree::Entry> nameTreeEntry = m_nameTree.findExactMatch(prefix);
  if (static_cast<bool>(nameTreeEntry))
    return nameTreeEntry->getFibEntry();
  return shared_ptr<fib::Entry>();
}

void
Fib::erase(const Name& prefix)
{
  shared_ptr<name_tree::Entry> nameTreeEntry = m_nameTree.findExactMatch(prefix);
  if (static_cast<bool>(nameTreeEntry))
  {
    nameTreeEntry->setFibEntry(shared_ptr<fib::Entry>());
    --m_nItems;
  }
}

void
Fib::erase(const fib::Entry& entry)
{
  shared_ptr<name_tree::Entry> nameTreeEntry = m_nameTree.findExactMatch(entry);
  if (static_cast<bool>(nameTreeEntry))
  {
    nameTreeEntry->setFibEntry(shared_ptr<fib::Entry>());
    --m_nItems;
  }
}

void
Fib::removeNextHopFromAllEntries(shared_ptr<Face> face)
{
  for (NameTree::const_iterator it = m_nameTree.fullEnumerate(
       &predicate_NameTreeEntry_hasFibEntry); it != m_nameTree.end(); ++it) {
    shared_ptr<fib::Entry> entry = it->getFibEntry();
    entry->removeNextHop(face);
    if (!entry->hasNextHops()) {
      this->erase(*entry);
    }
  }
}

Fib::const_iterator
Fib::begin() const
{
  return const_iterator(m_nameTree.fullEnumerate(&predicate_NameTreeEntry_hasFibEntry));
}

} // namespace nfd
