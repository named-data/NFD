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
 *
 * \author Ilya Moiseenko <http://ilyamoiseenko.com/>
 * \author Junxiao Shi <http://www.cs.arizona.edu/people/shijunxiao/>
 * \author Alexander Afanasyev <http://lasr.cs.ucla.edu/afanasyev/index.html>
 */

#include "cs.hpp"
#include "core/logger.hpp"
#include "core/random.hpp"

#include <ndn-cxx/util/crypto.hpp>
#include <ndn-cxx/security/signature-sha256-with-rsa.hpp>

#include <boost/random/bernoulli_distribution.hpp>
#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>
#include <type_traits>

/// max skip list layers
static const size_t SKIPLIST_MAX_LAYERS = 32;
/// probability for an entry in layer N to appear also in layer N+1
static const double SKIPLIST_PROBABILITY = 0.25;

NFD_LOG_INIT("ContentStore");

namespace nfd {

// http://en.cppreference.com/w/cpp/concept/ForwardIterator
BOOST_CONCEPT_ASSERT((boost::ForwardIterator<Cs::const_iterator>));
// boost::ForwardIterator follows SGI standard http://www.sgi.com/tech/stl/ForwardIterator.html,
// which doesn't require DefaultConstructible
#ifdef HAVE_IS_DEFAULT_CONSTRUCTIBLE
static_assert(std::is_default_constructible<Cs::const_iterator>::value,
              "Cs::const_iterator must be default-constructible");
#else
BOOST_CONCEPT_ASSERT((boost::DefaultConstructible<Cs::const_iterator>));
#endif // HAVE_IS_DEFAULT_CONSTRUCTIBLE

Cs::Cs(size_t nMaxPackets)
  : m_nMaxPackets(nMaxPackets)
  , m_nPackets(0)
{
  SkipListLayer* zeroLayer = new SkipListLayer();
  m_skipList.push_back(zeroLayer);

  for (size_t i = 0; i < m_nMaxPackets; i++)
    m_freeCsEntries.push(new cs::skip_list::Entry());
}

Cs::~Cs()
{
  // evict all items from CS
  while (evictItem())
    ;

  BOOST_ASSERT(m_freeCsEntries.size() == m_nMaxPackets);

  while (!m_freeCsEntries.empty())
    {
      delete m_freeCsEntries.front();
      m_freeCsEntries.pop();
    }
}

size_t
Cs::size() const
{
  return m_nPackets; // size of the first layer in a skip list
}

void
Cs::setLimit(size_t nMaxPackets)
{
  size_t oldNMaxPackets = m_nMaxPackets;
  m_nMaxPackets = nMaxPackets;

  while (size() > m_nMaxPackets) {
    evictItem();
  }

  if (m_nMaxPackets >= oldNMaxPackets) {
    for (size_t i = oldNMaxPackets; i < m_nMaxPackets; i++) {
      m_freeCsEntries.push(new cs::skip_list::Entry());
    }
  }
  else {
    for (size_t i = oldNMaxPackets; i > m_nMaxPackets; i--) {
      delete m_freeCsEntries.front();
      m_freeCsEntries.pop();
    }
  }
}

size_t
Cs::getLimit() const
{
  return m_nMaxPackets;
}

//Reference: "Skip Lists: A Probabilistic Alternative to Balanced Trees" by W.Pugh
std::pair<cs::skip_list::Entry*, bool>
Cs::insertToSkipList(const Data& data, bool isUnsolicited)
{
  NFD_LOG_TRACE("insertToSkipList() " << data.getFullName() << ", "
                << "skipList size " << size());

  BOOST_ASSERT(m_cleanupIndex.size() <= size());
  BOOST_ASSERT(m_freeCsEntries.size() > 0);

  // take entry for the memory pool
  cs::skip_list::Entry* entry = m_freeCsEntries.front();
  m_freeCsEntries.pop();
  m_nPackets++;
  entry->setData(data, isUnsolicited);

  bool insertInFront = false;
  bool isIterated = false;
  SkipList::reverse_iterator topLayer = m_skipList.rbegin();
  SkipListLayer::iterator updateTable[SKIPLIST_MAX_LAYERS];
  SkipListLayer::iterator head = (*topLayer)->begin();

  if (!(*topLayer)->empty())
    {
      //start from the upper layer towards bottom
      int layer = m_skipList.size() - 1;
      for (SkipList::reverse_iterator rit = topLayer; rit != m_skipList.rend(); ++rit)
        {
          //if we didn't do any iterations on the higher layers, start from the begin() again
          if (!isIterated)
            head = (*rit)->begin();

          updateTable[layer] = head;

          if (head != (*rit)->end())
            {
              // it can happen when begin() contains the element in front of which we need to insert
              if (!isIterated && ((*head)->getFullName() >= entry->getFullName()))
                {
                  --updateTable[layer];
                  insertInFront = true;
                }
              else
                {
                  SkipListLayer::iterator it = head;

                  while ((*it)->getFullName() < entry->getFullName())
                    {
                      head = it;
                      updateTable[layer] = it;
                      isIterated = true;

                      ++it;
                      if (it == (*rit)->end())
                        break;
                    }
                }
            }

          if (layer > 0)
            head = (*head)->getIterators().find(layer - 1)->second; // move HEAD to the lower layer

          layer--;
        }
    }
  else
    {
      updateTable[0] = (*topLayer)->begin(); //initialization
    }

  head = updateTable[0];
  ++head; // look at the next slot to check if it contains a duplicate

  bool isCsEmpty = (size() == 0);
  bool isInBoundaries = (head != (*m_skipList.begin())->end());
  bool isNameIdentical = false;
  if (!isCsEmpty && isInBoundaries)
    {
      isNameIdentical = (*head)->getFullName() == entry->getFullName();
    }

  //check if this is a duplicate packet
  if (isNameIdentical)
    {
      NFD_LOG_TRACE("Duplicate name (with digest)");

      (*head)->setData(data, isUnsolicited); //updates stale time

      // new entry not needed, returning to the pool
      entry->release();
      m_freeCsEntries.push(entry);
      m_nPackets--;

      return std::make_pair(*head, false);
    }

  NFD_LOG_TRACE("Not a duplicate");

  size_t randomLayer = pickRandomLayer();

  while (m_skipList.size() < randomLayer + 1)
    {
      SkipListLayer* newLayer = new SkipListLayer();
      m_skipList.push_back(newLayer);

      updateTable[(m_skipList.size() - 1)] = newLayer->begin();
    }

  size_t layer = 0;
  for (SkipList::iterator i = m_skipList.begin();
       i != m_skipList.end() && layer <= randomLayer; ++i)
    {
      if (updateTable[layer] == (*i)->end() && !insertInFront)
        {
          (*i)->push_back(entry);
          SkipListLayer::iterator last = (*i)->end();
          --last;
          entry->setIterator(layer, last);

          NFD_LOG_TRACE("pushback " << &(*last));
        }
      else if (updateTable[layer] == (*i)->end() && insertInFront)
        {
          (*i)->push_front(entry);
          entry->setIterator(layer, (*i)->begin());

          NFD_LOG_TRACE("pushfront ");
        }
      else
        {
          NFD_LOG_TRACE("insertafter");
          ++updateTable[layer]; // insert after
          SkipListLayer::iterator position = (*i)->insert(updateTable[layer], entry);
          entry->setIterator(layer, position); // save iterator where item was inserted
        }
      layer++;
    }

  return std::make_pair(entry, true);
}

bool
Cs::insert(const Data& data, bool isUnsolicited)
{
  NFD_LOG_TRACE("insert() " << data.getFullName());

  if (isFull())
    {
      evictItem();
    }

  //pointer and insertion status
  std::pair<cs::skip_list::Entry*, bool> entry = insertToSkipList(data, isUnsolicited);

  //new entry
  if (static_cast<bool>(entry.first) && (entry.second == true))
    {
      m_cleanupIndex.push_back(entry.first);
      return true;
    }

  return false;
}

size_t
Cs::pickRandomLayer() const
{
  static boost::random::bernoulli_distribution<> dist(SKIPLIST_PROBABILITY);
  // TODO rewrite using geometry_distribution
  size_t layer;
  for (layer = 0; layer < SKIPLIST_MAX_LAYERS; ++layer) {
    if (!dist(getGlobalRng())) {
      break;
    }
  }
  return layer;
}

bool
Cs::isFull() const
{
  if (size() >= m_nMaxPackets) //size of the first layer vs. max size
    return true;

  return false;
}

bool
Cs::eraseFromSkipList(cs::skip_list::Entry* entry)
{
  NFD_LOG_TRACE("eraseFromSkipList() "  << entry->getFullName());
  NFD_LOG_TRACE("SkipList size " << size());

  bool isErased = false;

  const std::map<int, std::list<cs::skip_list::Entry*>::iterator>& iterators = entry->getIterators();

  if (!iterators.empty())
    {
      int layer = 0;

      for (SkipList::iterator it = m_skipList.begin(); it != m_skipList.end(); )
        {
          std::map<int, std::list<cs::skip_list::Entry*>::iterator>::const_iterator i = iterators.find(layer);

          if (i != iterators.end())
            {
              (*it)->erase(i->second);
              entry->removeIterator(layer);
              isErased = true;

              //remove layers that do not contain any elements (starting from the second layer)
              if ((layer != 0) && (*it)->empty())
                {
                  delete *it;
                  it = m_skipList.erase(it);
                }
              else
                ++it;

              layer++;
            }
          else
            break;
      }
    }

  //delete entry;
  if (isErased)
  {
    entry->release();
    m_freeCsEntries.push(entry);
    m_nPackets--;
  }

  return isErased;
}

bool
Cs::evictItem()
{
  NFD_LOG_TRACE("evictItem()");

  if (!m_cleanupIndex.get<unsolicited>().empty()) {
    CleanupIndex::index<unsolicited>::type::const_iterator firstSolicited =
      m_cleanupIndex.get<unsolicited>().upper_bound(false);

    if (firstSolicited != m_cleanupIndex.get<unsolicited>().begin()) {
      --firstSolicited;
      BOOST_ASSERT((*firstSolicited)->isUnsolicited());
      NFD_LOG_TRACE("Evict from unsolicited queue");

      eraseFromSkipList(*firstSolicited);
      m_cleanupIndex.get<unsolicited>().erase(firstSolicited);
      return true;
    }
    else {
      BOOST_ASSERT(!(*m_cleanupIndex.get<unsolicited>().begin())->isUnsolicited());
    }
  }

  if (!m_cleanupIndex.get<byStaleness>().empty() &&
      (*m_cleanupIndex.get<byStaleness>().begin())->getStaleTime() < time::steady_clock::now())
  {
    NFD_LOG_TRACE("Evict from staleness queue");

    eraseFromSkipList(*m_cleanupIndex.get<byStaleness>().begin());
    m_cleanupIndex.get<byStaleness>().erase(m_cleanupIndex.get<byStaleness>().begin());
    return true;
  }

  if (!m_cleanupIndex.get<byArrival>().empty())
  {
    NFD_LOG_TRACE("Evict from arrival queue");

    eraseFromSkipList(*m_cleanupIndex.get<byArrival>().begin());
    m_cleanupIndex.get<byArrival>().erase(m_cleanupIndex.get<byArrival>().begin());
    return true;
  }

  return false;
}

const Data*
Cs::find(const Interest& interest) const
{
  NFD_LOG_TRACE("find() " << interest.getName());

  bool isIterated = false;
  SkipList::const_reverse_iterator topLayer = m_skipList.rbegin();
  SkipListLayer::iterator head = (*topLayer)->begin();

  if (!(*topLayer)->empty())
    {
      //start from the upper layer towards bottom
      int layer = m_skipList.size() - 1;
      for (SkipList::const_reverse_iterator rit = topLayer; rit != m_skipList.rend(); ++rit)
        {
          //if we didn't do any iterations on the higher layers, start from the begin() again
          if (!isIterated)
            head = (*rit)->begin();

          if (head != (*rit)->end())
            {
              // it happens when begin() contains the element we want to find
              if (!isIterated && (interest.getName().isPrefixOf((*head)->getFullName())))
                {
                  if (layer > 0)
                    {
                      layer--;
                      continue; // try lower layer
                    }
                  else
                    {
                      isIterated = true;
                    }
                }
              else
                {
                  SkipListLayer::iterator it = head;

                  while ((*it)->getFullName() < interest.getName())
                    {
                      NFD_LOG_TRACE((*it)->getFullName() << " < " << interest.getName());
                      head = it;
                      isIterated = true;

                      ++it;
                      if (it == (*rit)->end())
                        break;
                    }
                }
            }

          if (layer > 0)
            {
              head = (*head)->getIterators().find(layer - 1)->second; // move HEAD to the lower layer
            }
          else //if we reached the first layer
            {
              if (isIterated)
                return selectChild(interest, head);
            }

          layer--;
        }
    }

  return 0;
}

const Data*
Cs::selectChild(const Interest& interest, SkipListLayer::iterator startingPoint) const
{
  BOOST_ASSERT(startingPoint != (*m_skipList.begin())->end());

  if (startingPoint != (*m_skipList.begin())->begin())
    {
      BOOST_ASSERT((*startingPoint)->getFullName() < interest.getName());
    }

  NFD_LOG_TRACE("selectChild() " << interest.getChildSelector() << " "
                << (*startingPoint)->getFullName());

  bool hasLeftmostSelector = (interest.getChildSelector() <= 0);
  bool hasRightmostSelector = !hasLeftmostSelector;

  if (hasLeftmostSelector)
    {
      bool doesInterestContainDigest = recognizeInterestWithDigest(interest, *startingPoint);
      bool isInPrefix = false;

      if (doesInterestContainDigest)
        {
          isInPrefix = interest.getName().getPrefix(-1).isPrefixOf((*startingPoint)->getFullName());
        }
      else
        {
          isInPrefix = interest.getName().isPrefixOf((*startingPoint)->getFullName());
        }

      if (isInPrefix)
        {
          if (doesComplyWithSelectors(interest, *startingPoint, doesInterestContainDigest))
            {
              return &(*startingPoint)->getData();
            }
        }
    }

  //iterate to the right
  SkipListLayer::iterator rightmost = startingPoint;
  if (startingPoint != (*m_skipList.begin())->end())
    {
      SkipListLayer::iterator rightmostCandidate = startingPoint;
      Name currentChildPrefix("");

      while (true)
        {
          ++rightmostCandidate;

          bool isInBoundaries = (rightmostCandidate != (*m_skipList.begin())->end());
          bool isInPrefix = false;
          bool doesInterestContainDigest = false;
          if (isInBoundaries)
            {
              doesInterestContainDigest = recognizeInterestWithDigest(interest,
                                                                      *rightmostCandidate);

              if (doesInterestContainDigest)
                {
                  isInPrefix = interest.getName().getPrefix(-1)
                                 .isPrefixOf((*rightmostCandidate)->getFullName());
                }
              else
                {
                  isInPrefix = interest.getName().isPrefixOf((*rightmostCandidate)->getFullName());
                }
            }

          if (isInPrefix)
            {
              if (doesComplyWithSelectors(interest, *rightmostCandidate, doesInterestContainDigest))
                {
                  if (hasLeftmostSelector)
                    {
                      return &(*rightmostCandidate)->getData();
                    }

                  if (hasRightmostSelector)
                    {
                      if (doesInterestContainDigest)
                        {
                          // get prefix which is one component longer than Interest name
                          // (without digest)
                          const Name& childPrefix = (*rightmostCandidate)->getFullName()
                                                      .getPrefix(interest.getName().size());
                          NFD_LOG_TRACE("Child prefix" << childPrefix);

                          if (currentChildPrefix.empty() || (childPrefix != currentChildPrefix))
                            {
                              currentChildPrefix = childPrefix;
                              rightmost = rightmostCandidate;
                            }
                        }
                      else
                        {
                          // get prefix which is one component longer than Interest name
                          const Name& childPrefix = (*rightmostCandidate)->getFullName()
                                                      .getPrefix(interest.getName().size() + 1);
                          NFD_LOG_TRACE("Child prefix" << childPrefix);

                          if (currentChildPrefix.empty() || (childPrefix != currentChildPrefix))
                            {
                              currentChildPrefix = childPrefix;
                              rightmost = rightmostCandidate;
                            }
                        }
                    }
                }
            }
          else
            break;
        }
    }

  if (rightmost != startingPoint)
    {
      return &(*rightmost)->getData();
    }

  if (hasRightmostSelector) // if rightmost was not found, try starting point
    {
      bool doesInterestContainDigest = recognizeInterestWithDigest(interest, *startingPoint);
      bool isInPrefix = false;

      if (doesInterestContainDigest)
        {
          isInPrefix = interest.getName().getPrefix(-1).isPrefixOf((*startingPoint)->getFullName());
        }
      else
        {
          isInPrefix = interest.getName().isPrefixOf((*startingPoint)->getFullName());
        }

      if (isInPrefix)
        {
          if (doesComplyWithSelectors(interest, *startingPoint, doesInterestContainDigest))
            {
              return &(*startingPoint)->getData();
            }
        }
    }

  return 0;
}

bool
Cs::doesComplyWithSelectors(const Interest& interest,
                            cs::skip_list::Entry* entry,
                            bool doesInterestContainDigest) const
{
  NFD_LOG_TRACE("doesComplyWithSelectors()");

  /// \todo The following detection is not correct
  ///       1. If Interest name ends with 32-octet component doesn't mean that this component is
  ///          digest
  ///       2. Only min/max selectors (both 0) can be specified, all other selectors do not
  ///          make sense for interests with digest (though not sure if we need to enforce this)

  if (doesInterestContainDigest)
    {
      if (interest.getName().get(-1) != entry->getFullName().get(-1))
        {
          NFD_LOG_TRACE("violates implicit digest");
          return false;
        }
    }

  if (!doesInterestContainDigest)
    {
      if (interest.getMinSuffixComponents() >= 0)
        {
          size_t minDataNameLength = interest.getName().size() + interest.getMinSuffixComponents();

          bool isSatisfied = (minDataNameLength <= entry->getFullName().size());
          if (!isSatisfied)
            {
              NFD_LOG_TRACE("violates minComponents");
              return false;
            }
        }

      if (interest.getMaxSuffixComponents() >= 0)
        {
          size_t maxDataNameLength = interest.getName().size() + interest.getMaxSuffixComponents();

          bool isSatisfied = (maxDataNameLength >= entry->getFullName().size());
          if (!isSatisfied)
            {
              NFD_LOG_TRACE("violates maxComponents");
              return false;
            }
        }
    }

  if (interest.getMustBeFresh() && entry->getStaleTime() < time::steady_clock::now())
    {
      NFD_LOG_TRACE("violates mustBeFresh");
      return false;
    }

  if (!interest.getPublisherPublicKeyLocator().empty())
    {
      if (entry->getData().getSignature().getType() == ndn::Signature::Sha256WithRsa)
        {
          ndn::SignatureSha256WithRsa rsaSignature(entry->getData().getSignature());
          if (rsaSignature.getKeyLocator() != interest.getPublisherPublicKeyLocator())
            {
              NFD_LOG_TRACE("violates publisher key selector");
              return false;
            }
        }
      else
        {
          NFD_LOG_TRACE("violates publisher key selector");
          return false;
        }
    }

  if (doesInterestContainDigest)
    {
      const ndn::name::Component& lastComponent = entry->getFullName().get(-1);

      if (!lastComponent.empty())
        {
          if (interest.getExclude().isExcluded(lastComponent))
            {
              NFD_LOG_TRACE("violates exclusion");
              return false;
            }
        }
    }
  else
    {
      if (entry->getFullName().size() >= interest.getName().size() + 1)
        {
          const ndn::name::Component& nextComponent = entry->getFullName()
                                                        .get(interest.getName().size());
          if (!nextComponent.empty())
            {
              if (interest.getExclude().isExcluded(nextComponent))
                {
                  NFD_LOG_TRACE("violates exclusion");
                  return false;
                }
            }
        }
    }

  NFD_LOG_TRACE("complies");
  return true;
}

bool
Cs::recognizeInterestWithDigest(const Interest& interest, cs::skip_list::Entry* entry) const
{
  // only when min selector is not specified or specified with value of 0
  // and Interest's name length is exactly the length of the name of CS entry
  if (interest.getMinSuffixComponents() <= 0 &&
      interest.getName().size() == (entry->getFullName().size()))
    {
      const ndn::name::Component& last = interest.getName().get(-1);
      if (last.value_size() == ndn::crypto::SHA256_DIGEST_SIZE)
        {
          NFD_LOG_TRACE("digest recognized");
          return true;
        }
    }

  return false;
}

void
Cs::erase(const Name& exactName)
{
  NFD_LOG_TRACE("insert() " << exactName << ", "
                << "skipList size " << size());

  bool isIterated = false;
  SkipListLayer::iterator updateTable[SKIPLIST_MAX_LAYERS];
  SkipList::reverse_iterator topLayer = m_skipList.rbegin();
  SkipListLayer::iterator head = (*topLayer)->begin();

  if (!(*topLayer)->empty())
    {
      //start from the upper layer towards bottom
      int layer = m_skipList.size() - 1;
      for (SkipList::reverse_iterator rit = topLayer; rit != m_skipList.rend(); ++rit)
        {
          //if we didn't do any iterations on the higher layers, start from the begin() again
          if (!isIterated)
            head = (*rit)->begin();

          updateTable[layer] = head;

          if (head != (*rit)->end())
            {
              // it can happen when begin() contains the element we want to remove
              if (!isIterated && ((*head)->getFullName() == exactName))
                {
                  cs::skip_list::Entry* entryToDelete = *head;
                  NFD_LOG_TRACE("Found target " << entryToDelete->getFullName());
                  eraseFromSkipList(entryToDelete);
                  // head can become invalid after eraseFromSkipList
                  m_cleanupIndex.remove(entryToDelete);
                  return;
                }
              else
                {
                  SkipListLayer::iterator it = head;

                  while ((*it)->getFullName() < exactName)
                    {
                      head = it;
                      updateTable[layer] = it;
                      isIterated = true;

                      ++it;
                      if (it == (*rit)->end())
                        break;
                    }
                }
            }

          if (layer > 0)
            head = (*head)->getIterators().find(layer - 1)->second; // move HEAD to the lower layer

          layer--;
        }
    }
  else
    {
      return;
    }

  head = updateTable[0];
  ++head; // look at the next slot to check if it contains the item we want to remove

  bool isCsEmpty = (size() == 0);
  bool isInBoundaries = (head != (*m_skipList.begin())->end());
  bool isNameIdentical = false;
  if (!isCsEmpty && isInBoundaries)
    {
      NFD_LOG_TRACE("Identical? " << (*head)->getFullName());
      isNameIdentical = (*head)->getFullName() == exactName;
    }

  if (isNameIdentical)
    {
      cs::skip_list::Entry* entryToDelete = *head;
      NFD_LOG_TRACE("Found target " << entryToDelete->getFullName());
      eraseFromSkipList(entryToDelete);
      // head can become invalid after eraseFromSkipList
      m_cleanupIndex.remove(entryToDelete);
    }
}

void
Cs::printSkipList() const
{
  NFD_LOG_TRACE("print()");
  //start from the upper layer towards bottom
  int layer = m_skipList.size() - 1;
  for (SkipList::const_reverse_iterator rit = m_skipList.rbegin(); rit != m_skipList.rend(); ++rit)
    {
      for (SkipListLayer::iterator it = (*rit)->begin(); it != (*rit)->end(); ++it)
        {
          NFD_LOG_TRACE("Layer " << layer << " " << (*it)->getFullName());
        }
      layer--;
    }
}

} //namespace nfd
