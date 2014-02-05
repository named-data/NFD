/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "fib.hpp"
#include <algorithm>
#include <numeric>

namespace nfd {

Fib::Fib()
{
  std::pair<shared_ptr<fib::Entry>, bool> pair_rootEntry_dummy =
    this->insert(Name());
  m_rootEntry = pair_rootEntry_dummy.first;
}

Fib::~Fib()
{
}

static inline bool
predicate_FibEntry_eq_Name(const shared_ptr<fib::Entry>& entry,
  const Name& name)
{
  return entry->getPrefix().equals(name);
}

std::pair<shared_ptr<fib::Entry>, bool>
Fib::insert(const Name& prefix)
{
  std::list<shared_ptr<fib::Entry> >::iterator it = std::find_if(
    m_table.begin(), m_table.end(),
    bind(&predicate_FibEntry_eq_Name, _1, prefix));
  if (it != m_table.end()) return std::make_pair(*it, false);

  shared_ptr<fib::Entry> entry = make_shared<fib::Entry>(prefix);
  m_table.push_back(entry);
  return std::make_pair(entry, true);
}

static inline const shared_ptr<fib::Entry>&
accumulate_FibEntry_longestPrefixMatch(
  const shared_ptr<fib::Entry>& bestMatch,
  const shared_ptr<fib::Entry>& entry, const Name& name)
{
  if (!entry->getPrefix().isPrefixOf(name)) return bestMatch;
  if (bestMatch->getPrefix().size() < entry->getPrefix().size()) return entry;
  return bestMatch;
}

shared_ptr<fib::Entry>
Fib::findLongestPrefixMatch(const Name& prefix) const
{
  shared_ptr<fib::Entry> bestMatch =
    std::accumulate(m_table.begin(), m_table.end(), m_rootEntry,
    bind(&accumulate_FibEntry_longestPrefixMatch, _1, _2, prefix));
  return bestMatch;
}

shared_ptr<fib::Entry>
Fib::findExactMatch(const Name& prefix) const
{
  std::list<shared_ptr<fib::Entry> >::const_iterator it =
    std::find_if(m_table.begin(), m_table.end(),
                 bind(&predicate_FibEntry_eq_Name, _1, prefix));

  if (it != m_table.end())
    {
      return *it;
    }
  return shared_ptr<fib::Entry>();
}

void
Fib::remove(const Name& prefix)
{
  m_table.remove_if(bind(&predicate_FibEntry_eq_Name, _1, prefix));
}

static inline void
FibEntry_removeNextHop(shared_ptr<fib::Entry> entry,
  shared_ptr<Face> face)
{
  entry->removeNextHop(face);
}

void
Fib::removeNextHopFromAllEntries(shared_ptr<Face> face)
{
  std::for_each(m_table.begin(), m_table.end(),
    bind(&FibEntry_removeNextHop, _1, face));
}


} // namespace nfd
