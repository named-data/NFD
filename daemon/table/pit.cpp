/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "pit.hpp"
#include <algorithm>

namespace nfd {

Pit::Pit()
{
}

Pit::~Pit()
{
}

static inline bool
operator==(const Exclude& a, const Exclude& b)
{
  const Block& aBlock = a.wireEncode();
  const Block& bBlock = b.wireEncode();
  return aBlock.size() == bBlock.size() &&
         0 == memcmp(aBlock.wire(), bBlock.wire(), aBlock.size());
}

static inline bool
predicate_PitEntry_similar_Interest(shared_ptr<pit::Entry> entry,
                                    const Interest& interest)
{
  const Interest& pi = entry->getInterest();
  return pi.getName().equals(interest.getName()) &&
         pi.getMinSuffixComponents() == interest.getMinSuffixComponents() &&
         pi.getMaxSuffixComponents() == interest.getMaxSuffixComponents() &&
         // TODO PublisherPublicKeyLocator (ndn-cpp-dev #1157)
         pi.getExclude() == interest.getExclude() &&
         pi.getChildSelector() == interest.getChildSelector() &&
         pi.getMustBeFresh() == interest.getMustBeFresh();
}

std::pair<shared_ptr<pit::Entry>, bool>
Pit::insert(const Interest& interest)
{
  std::list<shared_ptr<pit::Entry> >::iterator it = std::find_if(
    m_table.begin(), m_table.end(),
    bind(&predicate_PitEntry_similar_Interest, _1, interest));
  if (it != m_table.end()) return std::make_pair(*it, false);
  
  shared_ptr<pit::Entry> entry = make_shared<pit::Entry>(interest);
  m_table.push_back(entry);
  return std::make_pair(entry, true);
}

shared_ptr<pit::DataMatchResult>
Pit::findAllDataMatches(const Data& data) const
{
  shared_ptr<pit::DataMatchResult> result = make_shared<pit::DataMatchResult>();
  for (std::list<shared_ptr<pit::Entry> >::const_iterator it = m_table.begin();
       it != m_table.end(); ++it) {
    shared_ptr<pit::Entry> entry = *it;
    if (entry->getInterest().matchesName(data.getName())) {
      result->push_back(entry);
    }
  }
  return result;
}

void
Pit::remove(shared_ptr<pit::Entry> pitEntry)
{
  m_table.remove(pitEntry);
}

} // namespace nfd
