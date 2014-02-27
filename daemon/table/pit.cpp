/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

/**
 *  KNOWN ISSUES
 *  
 *  - To remove a PIT entry, we need to first perform a lookup on NameTree
 *  to locate its NameTree Entry, and then call NameTreeEntry->deletePitEntry()
 *  function. Alternatively, we could store a pointer at each PIT-Entry, which
 *  would speed up this procedure with the cost of additional memory space. Maybe
 *  this could also be part of the PIT/FIB/Measurement shortcut, where all of these
 *  entries have pointers to their NameTreeEntry. Which could be part of task
 *  #1202, shortcuts between FIB, PIT, Measurements.
 *  
 */

#include "pit.hpp"

namespace nfd {

Pit::Pit(NameTree& nameTree) : m_nameTree(nameTree), m_nItems(0)
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
  // - first lookup() the Interest Name in the NameTree, which will creates all
  // the intermedia nodes, starting from the shortest prefix.
  // - if it is guaranteed that this Interest already has a NameTree Entry, we 
  // could use findExactMatch() instead.
  // - Alternatively, we could try to do findExactMatch() first, if not found, then 
  // do lookup().
  shared_ptr<name_tree::Entry> nameTreeEntry = m_nameTree.lookup(interest.getName());

  BOOST_ASSERT(static_cast<bool>(nameTreeEntry));

  std::vector<shared_ptr<pit::Entry> >& pitEntries = nameTreeEntry->getPitEntries();

  // then check if this Interest is already in the PIT entries
  std::vector<shared_ptr<pit::Entry> >::iterator it = std::find_if(pitEntries.begin(), pitEntries.end(), bind(&predicate_PitEntry_similar_Interest, _1, interest));

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

  shared_ptr<name_tree::Entry> nameTreeEntry;

  // NOTE: We are using findLongestPrefixMatch() here.
  // The reason is that findLongestPrefixMatch() starts with the full name
  // and then remove one component each time, which is the type of behavior we would like
  // to use here.
  // If it could be guranteed that the quering Data packet always has a Name Tree
  // Entry, we could also use findExactMatch(). 
  for (nameTreeEntry = m_nameTree.findLongestPrefixMatch(data.getName()); 
                                   static_cast<bool>(nameTreeEntry);
                                   nameTreeEntry = nameTreeEntry->getParent())
    {
      std::vector<shared_ptr<pit::Entry> >& pitEntries = nameTreeEntry->getPitEntries();
      for (size_t i = 0; i < pitEntries.size(); i++)
        {
          if (pitEntries[i]->getInterest().matchesName(data.getName()))
            result->push_back(pitEntries[i]);
        }
    }

  return result;
}

void
Pit::erase(shared_ptr<pit::Entry> pitEntry)
{
  // first get the NPE
  // If pit-entry.hpp stores a NameTree Entry for each PIT, we could also use the get() method
  // directly, saving one hash computation.
  shared_ptr<name_tree::Entry> nameTreeEntry = m_nameTree.findExactMatch(pitEntry->getName());

  BOOST_ASSERT(static_cast<bool>(nameTreeEntry));

  // erase this PIT entry
  if (static_cast<bool>(nameTreeEntry)) 
    {
      nameTreeEntry->erasePitEntry(pitEntry);
      m_nameTree.eraseEntryIfEmpty(nameTreeEntry);

      m_nItems--;
    }
}

} // namespace nfd
