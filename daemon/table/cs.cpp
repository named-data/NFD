/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 *
 * Author: Ilya Moiseenko <iliamo@ucla.edu>
 */

#include "cs.hpp"
#include "core/logger.hpp"

#include <ndn-cpp-dev/util/crypto.hpp>

#define SKIPLIST_MAX_LAYERS 32
#define SKIPLIST_PROBABILITY 50         // 50% ( p = 1/2 )

NFD_LOG_INIT("ContentStore");

namespace nfd {

Cs::Cs(int nMaxPackets)
  : m_nMaxPackets(nMaxPackets)
{
  srand (time::toUnixTimestamp(time::system_clock::now()).count());
  SkipListLayer* zeroLayer = new SkipListLayer();
  m_skipList.push_back(zeroLayer);
}

Cs::~Cs()
{
  /// \todo Fix memory leak
}

size_t
Cs::size() const
{
  return (*m_skipList.begin())->size(); // size of the first layer in a skip list
}

void
Cs::setLimit(size_t nMaxPackets)
{
  m_nMaxPackets = nMaxPackets;

  while (isFull())
    {
      evictItem();
    }
}

size_t
Cs::getLimit() const
{
  return m_nMaxPackets;
}

//Reference: "Skip Lists: A Probabilistic Alternative to Balanced Trees" by W.Pugh
std::pair< shared_ptr<cs::Entry>, bool>
Cs::insertToSkipList(const Data& data, bool isUnsolicited)
{
  NFD_LOG_TRACE("insertToSkipList() " << data.getName() << ", "
                << "skipList size " << size());

  shared_ptr<cs::Entry> entry = make_shared<cs::Entry>(data, isUnsolicited);

  bool insertInFront = false;
  bool isIterated = false;
  SkipList::reverse_iterator topLayer = m_skipList.rbegin();
  SkipListLayer::iterator updateTable[SKIPLIST_MAX_LAYERS];
  SkipListLayer::iterator head = (*topLayer)->begin();

  if ( !(*topLayer)->empty() )
    {
      //start from the upper layer towards bottom
      int layer = m_skipList.size() - 1;
      for (SkipList::reverse_iterator rit = topLayer; rit != m_skipList.rend(); ++rit)
        {
          //if we didn't do any iterations on the higher layers, start from the begin() again
          if ( !isIterated )
            head = (*rit)->begin();

          updateTable[layer] = head;

          if (head != (*rit)->end())
            {
              // it can happen when begin() contains the element in front of which we need to insert
              if ( !isIterated && ((*head)->getName() >= entry->getName()) )
                {
                  --updateTable[layer];
                  insertInFront = true;
                }
              else
                {
                  SkipListLayer::iterator it = head;

                  while ((*it)->getName() < entry->getName())
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
      isNameIdentical = (*head)->getName() == entry->getName();
    }

  //check if this is a duplicate packet
  if (isNameIdentical)
    {
      NFD_LOG_TRACE("Duplicate name (with digest)");

      (*head)->setData(data, entry->getDigest()); //updates stale time

      return std::make_pair(*head, false);
    }

  NFD_LOG_TRACE("Not a duplicate");

  size_t randomLayer = pickRandomLayer();

  while ( m_skipList.size() < randomLayer + 1)
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

  printSkipList();

  return std::make_pair(entry, true);
}

bool
Cs::insert(const Data& data, bool isUnsolicited)
{
  NFD_LOG_TRACE("insert() " << data.getName());

  if (isFull())
    {
      evictItem();
    }

  //pointer and insertion status
  std::pair< shared_ptr<cs::Entry>, bool> entry = insertToSkipList(data, isUnsolicited);

  //new entry
  if (entry.first && (entry.second == true))
    {
      m_contentByArrival.push(entry.first);
      m_contentByStaleness.push(entry.first);

      if (entry.first->isUnsolicited())
        m_unsolicitedContent.push(entry.first);

      return true;
    }

  return false;
}

size_t
Cs::pickRandomLayer() const
{
  int layer = -1;
  int randomValue;

  do
    {
      layer++;
      randomValue = rand() % 100 + 1;
    }
  while ( (randomValue < SKIPLIST_PROBABILITY) && (layer < SKIPLIST_MAX_LAYERS) );

  return static_cast<size_t>(layer);
}

bool
Cs::isFull() const
{
  if (size() >= m_nMaxPackets) //size of the first layer vs. max size
    return true;

  return false;
}

bool
Cs::eraseFromSkipList(shared_ptr<cs::Entry> entry)
{
  NFD_LOG_TRACE("eraseFromSkipList() "  << entry->getName());
  NFD_LOG_TRACE("SkipList size " << size());

  bool isErased = false;

  int layer = m_skipList.size() - 1;
  for (SkipList::reverse_iterator rit = m_skipList.rbegin(); rit != m_skipList.rend(); ++rit)
    {
      const std::map<int, std::list< shared_ptr<cs::Entry> >::iterator>& iterators = entry->getIterators();
      std::map<int, std::list< shared_ptr<cs::Entry> >::iterator>::const_iterator it = iterators.find(layer);
      if (it != iterators.end())
        {
          (*rit)->erase(it->second);
          entry->removeIterator(layer);
          isErased = true;
        }

      layer--;
    }

  printSkipList();

  //remove layers that do not contain any elements (starting from the second layer)
  for (SkipList::iterator it = (++m_skipList.begin()); it != m_skipList.end();)
    {
      if ((*it)->empty())
        {
          it = m_skipList.erase(it);
        }
      else
        ++it;
    }

  return isErased;
}

bool
Cs::evictItem()
{
  NFD_LOG_TRACE("evictItem()");

  //because there is a possibility that this item is in a queue, but no longer in skiplist
  while ( !m_unsolicitedContent.empty() )
    {
      NFD_LOG_TRACE("Evict from unsolicited queue");

      shared_ptr<cs::Entry> entry = m_unsolicitedContent.front();
      m_unsolicitedContent.pop();
      bool isErased = eraseFromSkipList(entry);

      if (isErased)
        return true;
    }

  //because there is a possibility that this item is in a queue, but no longer in skiplist
  int nIterations = size() * 0.01;  // 1% of the Content Store
  while ( !m_contentByStaleness.empty() )
    {
      NFD_LOG_TRACE("Evict from staleness queue");

      shared_ptr<cs::Entry> entry = m_contentByStaleness.top();

      //because stale time could be updated by the duplicate packet
      if (entry->getStaleTime() < time::steady_clock::now())
        {
          m_contentByStaleness.pop();
          bool isErased = eraseFromSkipList(entry);

          if (isErased)
            return true;
        }
      else if ( (entry->getStaleTime() > time::steady_clock::now()) && entry->wasRefreshedByDuplicate() )
        {
          m_contentByStaleness.pop();
          m_contentByStaleness.push(entry); // place in a right order

          nIterations--;
          // if 1% of the CS are non-expired refreshed CS entries (allocated sequentially),
          // then stop to prevent too many iterations
          if ( nIterations <= 0 )
            break;
        }
      else //no further item will be expired, stop
        {
          break;
        }
    }

  //because there is a possibility that this item is in a queue, but no longer in skiplist
  while ( !m_contentByArrival.empty() )
    {
      NFD_LOG_TRACE("Evict from arrival queue");

      shared_ptr<cs::Entry> entry = m_contentByArrival.front();
      m_contentByArrival.pop();
      bool isErased = eraseFromSkipList(entry);

      if (isErased)
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

  if ( !(*topLayer)->empty() )
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
              if ( !isIterated && (interest.getName().isPrefixOf((*head)->getName())) )
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

                  while ( (*it)->getName() < interest.getName() )
                    {
                      NFD_LOG_TRACE((*it)->getName() << " < " << interest.getName());
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
              if ( isIterated )
                return selectChild(interest, head);
            }

          layer--;
        }
    }

  return 0;
}

// because skip list is a probabilistic data structure and the way it is traversed,
// there is no guarantee that startingPoint is an element preceding the leftmost child
const Data*
Cs::selectChild(const Interest& interest, SkipListLayer::iterator startingPoint) const
{
  BOOST_ASSERT( startingPoint != (*m_skipList.begin())->end() );

  if (startingPoint != (*m_skipList.begin())->begin())
    {
      BOOST_ASSERT( (*startingPoint)->getName() < interest.getName() );
    }

  NFD_LOG_TRACE("selectChild() " << interest.getChildSelector() << " " << (*startingPoint)->getName());

  bool hasLeftmostSelector = (interest.getChildSelector() <= 0);
  bool hasRightmostSelector = !hasLeftmostSelector;

  if (hasLeftmostSelector)
    {
      bool doesInterestContainDigest = recognizeInterestWithDigest(interest, *startingPoint);
      bool isInPrefix = false;

      if (doesInterestContainDigest)
        {
          isInPrefix = interest.getName().getPrefix(-1).isPrefixOf((*startingPoint)->getName());
        }
      else
        {
          isInPrefix = interest.getName().isPrefixOf((*startingPoint)->getName());
        }

      if (isInPrefix)
        {
          if (doesComplyWithSelectors(interest, *startingPoint))
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
              doesInterestContainDigest = recognizeInterestWithDigest(interest, *rightmostCandidate);

              if (doesInterestContainDigest)
                {
                  isInPrefix = interest.getName().getPrefix(-1).isPrefixOf((*rightmostCandidate)->getName());
                }
              else
                {
                  isInPrefix = interest.getName().isPrefixOf((*rightmostCandidate)->getName());
                }
            }

          if (isInPrefix)
            {
              if (doesComplyWithSelectors(interest, *rightmostCandidate))
                {
                  if (hasLeftmostSelector)
                    {
                      return &(*rightmostCandidate)->getData();
                    }

                  if (hasRightmostSelector)
                    {
                      if (doesInterestContainDigest)
                        {
                          // get prefix which is one component longer than Interest name (without digest)
                          const Name& childPrefix = (*rightmostCandidate)->getName().getPrefix(interest.getName().size());
                          NFD_LOG_TRACE("Child prefix" << childPrefix);

                          if ( currentChildPrefix.empty() || (childPrefix != currentChildPrefix) )
                            {
                              currentChildPrefix = childPrefix;
                              rightmost = rightmostCandidate;
                            }
                        }
                      else
                        {
                          // get prefix which is one component longer than Interest name
                          const Name& childPrefix = (*rightmostCandidate)->getName().getPrefix(interest.getName().size() + 1);
                          NFD_LOG_TRACE("Child prefix" << childPrefix);

                          if ( currentChildPrefix.empty() || (childPrefix != currentChildPrefix) )
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
          isInPrefix = interest.getName().getPrefix(-1).isPrefixOf((*startingPoint)->getName());
        }
      else
        {
          isInPrefix = interest.getName().isPrefixOf((*startingPoint)->getName());
        }

      if (isInPrefix)
        {
          if (doesComplyWithSelectors(interest, *startingPoint))
            {
              return &(*startingPoint)->getData();
            }
        }
    }

  return 0;
}

bool
Cs::doesComplyWithSelectors(const Interest& interest, shared_ptr<cs::Entry> entry) const
{
  NFD_LOG_TRACE("doesComplyWithSelectors()");

  /// \todo The following detection is not correct
  ///       1. If data name ends with 32-octet component doesn't mean that this component is digest
  ///       2. Only min/max selectors (both 0) can be specified, all other selectors do not make sense
  ///          for interests with digest (though not sure if we need to enforce this)
  bool doesInterestContainDigest = recognizeInterestWithDigest(interest, entry);
  if ( doesInterestContainDigest )
    {
      const ndn::name::Component& last = interest.getName().get(-1);
      const ndn::ConstBufferPtr& digest = entry->getDigest();

      BOOST_ASSERT(digest->size() == last.value_size());
      BOOST_ASSERT(digest->size() == ndn::crypto::SHA256_DIGEST_SIZE);

      if (std::memcmp(digest->buf(), last.value(), ndn::crypto::SHA256_DIGEST_SIZE) != 0)
        {
          NFD_LOG_TRACE("violates implicit digest");
          return false;
        }
    }

  if ( !doesInterestContainDigest )
    {
      if (interest.getMinSuffixComponents() >= 0)
        {
          size_t minDataNameLength = interest.getName().size() + interest.getMinSuffixComponents();

          bool isSatisfied = (minDataNameLength <= entry->getName().size());
          if ( !isSatisfied )
            {
              NFD_LOG_TRACE("violates minComponents");
              return false;
            }
        }

      if (interest.getMaxSuffixComponents() >= 0)
        {
          size_t maxDataNameLength = interest.getName().size() + interest.getMaxSuffixComponents();

          bool isSatisfied = (maxDataNameLength >= entry->getName().size());
          if ( !isSatisfied )
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

  if ( doesInterestContainDigest )
    {
      const ndn::name::Component& lastComponent = entry->getName().get(-1);

      if ( !lastComponent.empty() )
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
      if (entry->getName().size() >= interest.getName().size() + 1)
        {
          const ndn::name::Component& nextComponent = entry->getName().get(interest.getName().size());

          if ( !nextComponent.empty() )
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
Cs::recognizeInterestWithDigest(const Interest& interest, shared_ptr<cs::Entry> entry) const
{
  // only when min selector is not specified or specified with value of 0
  // and Interest's name length is exactly the length of the name of CS entry
  if (interest.getMinSuffixComponents() <= 0 &&
      interest.getName().size() == (entry->getName().size()))
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

  if ( !(*topLayer)->empty() )
    {
      //start from the upper layer towards bottom
      int layer = m_skipList.size() - 1;
      for (SkipList::reverse_iterator rit = topLayer; rit != m_skipList.rend(); ++rit)
        {
          //if we didn't do any iterations on the higher layers, start from the begin() again
          if ( !isIterated )
            head = (*rit)->begin();

          updateTable[layer] = head;

          if (head != (*rit)->end())
            {
              // it can happen when begin() contains the element we want to remove
              if ( !isIterated && ((*head)->getName() == exactName) )
                {
                  eraseFromSkipList(*head);
                  return;
                }
              else
                {
                  SkipListLayer::iterator it = head;

                  while ((*it)->getName() < exactName)
                    {
                      head = it;
                      updateTable[layer] = it;
                      isIterated = true;

                      ++it;
                      if ( it == (*rit)->end() )
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
      NFD_LOG_TRACE("Identical? " << (*head)->getName());
      isNameIdentical = (*head)->getName() == exactName;
    }

  if (isNameIdentical)
    {
      NFD_LOG_TRACE("Found target " << (*head)->getName());
      eraseFromSkipList(*head);
    }

  printSkipList();
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
          NFD_LOG_TRACE("Layer " << layer << " " << (*it)->getName());
        }
      layer--;
    }
}

} //namespace nfd
