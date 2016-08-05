/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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
#include "core/city-hash.hpp"

#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>
#include <type_traits>

namespace nfd {
namespace name_tree {

NFD_LOG_INIT("NameTree");

// http://en.cppreference.com/w/cpp/concept/ForwardIterator
BOOST_CONCEPT_ASSERT((boost::ForwardIterator<NameTree::const_iterator>));
// boost::ForwardIterator follows SGI standard http://www.sgi.com/tech/stl/ForwardIterator.html,
// which doesn't require DefaultConstructible
#ifdef HAVE_IS_DEFAULT_CONSTRUCTIBLE
static_assert(std::is_default_constructible<NameTree::const_iterator>::value,
              "NameTree::const_iterator must be default-constructible");
#else
BOOST_CONCEPT_ASSERT((boost::DefaultConstructible<NameTree::const_iterator>));
#endif // HAVE_IS_DEFAULT_CONSTRUCTIBLE

class Hash32
{
public:
  static size_t
  compute(const char* buffer, size_t length)
  {
    return static_cast<size_t>(CityHash32(buffer, length));
  }
};

class Hash64
{
public:
  static size_t
  compute(const char* buffer, size_t length)
  {
    return static_cast<size_t>(CityHash64(buffer, length));
  }
};

/// @cond NoDocumentation
typedef boost::mpl::if_c<sizeof(size_t) >= 8, Hash64, Hash32>::type CityHash;
/// @endcond

// Interface of different hash functions
size_t
computeHash(const Name& prefix)
{
  prefix.wireEncode();  // guarantees prefix's wire buffer is not empty

  size_t hashValue = 0;
  size_t hashUpdate = 0;

  for (Name::const_iterator it = prefix.begin(); it != prefix.end(); ++it) {
    const char* wireFormat = reinterpret_cast<const char*>( it->wire() );
    hashUpdate = CityHash::compute(wireFormat, it->size());
    hashValue ^= hashUpdate;
  }

  return hashValue;
}

std::vector<size_t>
computeHashSet(const Name& prefix)
{
  prefix.wireEncode(); // guarantees prefix's wire buffer is not empty

  size_t hashValue = 0;
  size_t hashUpdate = 0;

  std::vector<size_t> hashValueSet;
  hashValueSet.push_back(hashValue);

  for (Name::const_iterator it = prefix.begin(); it != prefix.end(); ++it) {
    const char* wireFormat = reinterpret_cast<const char*>( it->wire() );
    hashUpdate = CityHash::compute(wireFormat, it->size());
    hashValue ^= hashUpdate;
    hashValueSet.push_back(hashValue);
  }

  return hashValueSet;
}

NameTree::NameTree(size_t nBuckets)
  : m_nItems(0)
  , m_nBuckets(nBuckets)
  , m_minNBuckets(nBuckets)
  , m_enlargeLoadFactor(0.5) // more than 50% buckets loaded
  , m_enlargeFactor(2)       // double the hash table size
  , m_shrinkLoadFactor(0.1)  // less than 10% buckets loaded
  , m_shrinkFactor(0.5)      // reduce the number of buckets by half
{
  m_enlargeThreshold = static_cast<size_t>(m_enlargeLoadFactor * static_cast<double>(m_nBuckets));
  m_shrinkThreshold = static_cast<size_t>(m_shrinkLoadFactor * static_cast<double>(m_nBuckets));

  // array of node pointers
  m_buckets = new Node*[m_nBuckets];
  // Initialize the pointer array
  for (size_t i = 0; i < m_nBuckets; ++i) {
    m_buckets[i] = nullptr;
  }
}

NameTree::~NameTree()
{
  for (size_t i = 0; i < m_nBuckets; ++i) {
    if (m_buckets[i] != nullptr) {
      delete m_buckets[i];
    }
  }

  delete[] m_buckets;
}

// insert() is a private function, and called by only lookup()
std::pair<shared_ptr<Entry>, bool>
NameTree::insert(const Name& prefix)
{
  NFD_LOG_TRACE("insert " << prefix);

  size_t hashValue = computeHash(prefix);
  size_t loc = hashValue % m_nBuckets;

  NFD_LOG_TRACE("Name " << prefix << " hash value = " << hashValue << "  location = " << loc);

  // Check if this Name has been stored
  Node* node = m_buckets[loc];
  Node* nodePrev = node;

  for (node = m_buckets[loc]; node != nullptr; node = node->m_next) {
    if (node->m_entry != nullptr) {
      if (prefix == node->m_entry->m_prefix) {
         return {node->m_entry, false}; // false: old entry
      }
    }
    nodePrev = node;
  }

  NFD_LOG_TRACE("Did not find " << prefix << ", need to insert it to the table");

  // If no bucket is empty occupied, we need to create a new node, and it is
  // linked from nodePrev
  node = new Node();
  node->m_prev = nodePrev;

  if (nodePrev == nullptr) {
    m_buckets[loc] = node;
  }
  else{
    nodePrev->m_next = node;
  }

  // Create a new Entry
  auto entry = make_shared<Entry>(prefix);
  entry->setHash(hashValue);
  node->m_entry = entry; // link the Entry to its Node
  entry->m_node = node; // link the node to Entry. Used in eraseEntryIfEmpty.

  return {entry, true}; // true: new entry
}

// Name Prefix Lookup. Create Name Tree Entry if not found
shared_ptr<Entry>
NameTree::lookup(const Name& prefix)
{
  NFD_LOG_TRACE("lookup " << prefix);

  shared_ptr<Entry> entry;
  shared_ptr<Entry> parent;

  for (size_t i = 0; i <= prefix.size(); ++i) {
    Name temp = prefix.getPrefix(i);

    // insert() will create the entry if it does not exist.
    bool isNew = false;
    std::tie(entry, isNew) = insert(temp);

    if (isNew) {
      ++m_nItems; // Increase the counter
      entry->m_parent = parent;

      if (parent != nullptr) {
        parent->m_children.push_back(entry);
      }
    }

    if (m_nItems > m_enlargeThreshold) {
      resize(m_enlargeFactor * m_nBuckets);
    }

    parent = entry;
  }
  return entry;
}

shared_ptr<Entry>
NameTree::lookup(const fib::Entry& fibEntry) const
{
  shared_ptr<Entry> nte = this->getEntry(fibEntry);
  BOOST_ASSERT(nte == nullptr || nte->getFibEntry() == &fibEntry);
  return nte;
}

shared_ptr<Entry>
NameTree::lookup(const pit::Entry& pitEntry)
{
  shared_ptr<Entry> nte = this->getEntry(pitEntry);
  if (nte == nullptr) {
    return nullptr;
  }

  if (nte->getPrefix().size() == pitEntry.getName().size()) {
    return nte;
  }

  BOOST_ASSERT(pitEntry.getName().at(-1).isImplicitSha256Digest());
  BOOST_ASSERT(nte->getPrefix() == pitEntry.getName().getPrefix(-1));
  return this->lookup(pitEntry.getName());
}

shared_ptr<Entry>
NameTree::lookup(const measurements::Entry& measurementsEntry) const
{
  shared_ptr<Entry> nte = this->getEntry(measurementsEntry);
  BOOST_ASSERT(nte == nullptr || nte->getMeasurementsEntry() == &measurementsEntry);
  return nte;
}

shared_ptr<Entry>
NameTree::lookup(const strategy_choice::Entry& strategyChoiceEntry) const
{
  shared_ptr<Entry> nte = this->getEntry(strategyChoiceEntry);
  BOOST_ASSERT(nte == nullptr || nte->getStrategyChoiceEntry() == &strategyChoiceEntry);
  return nte;
}

// return {false: this entry is not empty, true: this entry is empty and erased}
bool
NameTree::eraseEntryIfEmpty(shared_ptr<Entry> entry)
{
  BOOST_ASSERT(entry != nullptr);

  NFD_LOG_TRACE("eraseEntryIfEmpty " << entry->getPrefix());

  // first check if this Entry can be erased
  if (entry->isEmpty()) {
    // update child-related info in the parent
    shared_ptr<Entry> parent = entry->getParent();

    if (parent != nullptr) {
      std::vector<shared_ptr<Entry>>& parentChildrenList = parent->getChildren();

      bool isFound = false;
      size_t size = parentChildrenList.size();
      for (size_t i = 0; i < size; ++i) {
        if (parentChildrenList[i] == entry) {
          parentChildrenList[i] = parentChildrenList[size - 1];
          parentChildrenList.pop_back();
          isFound = true;
          break;
        }
      }

      BOOST_VERIFY(isFound == true);
    }

    // remove this Entry and its Name Tree Node
    Node* node = entry->m_node;
    Node* nodePrev = node->m_prev;

    // configure the previous node
    if (nodePrev != nullptr) {
      // link the previous node to the next node
      nodePrev->m_next = node->m_next;
    }
    else {
      m_buckets[entry->getHash() % m_nBuckets] = node->m_next;
    }

    // link the previous node with the next node (skip the erased one)
    if (node->m_next != nullptr) {
      node->m_next->m_prev = nodePrev;
      node->m_next = 0;
    }

    BOOST_ASSERT(node->m_next == nullptr);

    --m_nItems;
    delete node;

    if (parent != nullptr) {
      eraseEntryIfEmpty(parent);
    }

    size_t newNBuckets = static_cast<size_t>(m_shrinkFactor * static_cast<double>(m_nBuckets));

    if (newNBuckets >= m_minNBuckets && m_nItems < m_shrinkThreshold) {
      resize(newNBuckets);
    }

    return true;

  }

  return false;
}

// Exact Match
shared_ptr<Entry>
NameTree::findExactMatch(const Name& prefix) const
{
  NFD_LOG_TRACE("findExactMatch " << prefix);

  size_t hashValue = computeHash(prefix);
  size_t loc = hashValue % m_nBuckets;

  NFD_LOG_TRACE("Name " << prefix << " hash value = " << hashValue << "  location = " << loc);

  shared_ptr<Entry> entry;
  Node* node = nullptr;

  for (node = m_buckets[loc]; node != nullptr; node = node->m_next) {
    entry = node->m_entry;
    if (entry != nullptr) {
      if (hashValue == entry->getHash() && prefix == entry->getPrefix()) {
        return entry;
      }
    }
  }

  // if not found, the default value of entry (null pointer) will be returned
  entry.reset();
  return entry;
}

// Longest Prefix Match
shared_ptr<Entry>
NameTree::findLongestPrefixMatch(const Name& prefix, const EntrySelector& entrySelector) const
{
  NFD_LOG_TRACE("findLongestPrefixMatch " << prefix);

  shared_ptr<Entry> entry;
  std::vector<size_t> hashValueSet = computeHashSet(prefix);

  size_t hashValue = 0;
  size_t loc = 0;

  for (int i = static_cast<int>(prefix.size()); i >= 0; --i) {
    hashValue = hashValueSet[i];
    loc = hashValue % m_nBuckets;

    Node* node = nullptr;
    for (node = m_buckets[loc]; node != nullptr; node = node->m_next) {
      entry = node->m_entry;
      if (entry != nullptr) {
        // isPrefixOf() is used to avoid making a copy of the name
        if (hashValue == entry->getHash() &&
            entry->getPrefix().isPrefixOf(prefix) &&
            entrySelector(*entry)) {
          return entry;
        }
      }
    }
  }

  return nullptr;
}

shared_ptr<Entry>
NameTree::findLongestPrefixMatch(shared_ptr<Entry> entry,
                                 const EntrySelector& entrySelector) const
{
  while (entry != nullptr) {
    if (entrySelector(*entry)) {
      return entry;
    }
    entry = entry->getParent();
  }
  return nullptr;
}

shared_ptr<Entry>
NameTree::findLongestPrefixMatch(const pit::Entry& pitEntry) const
{
  shared_ptr<Entry> nte = this->getEntry(pitEntry);
  BOOST_ASSERT(nte != nullptr);
  if (nte->getPrefix().size() == pitEntry.getName().size()) {
    return nte;
  }

  BOOST_ASSERT(pitEntry.getName().at(-1).isImplicitSha256Digest());
  BOOST_ASSERT(nte->getPrefix() == pitEntry.getName().getPrefix(-1));
  shared_ptr<Entry> exact = this->findExactMatch(pitEntry.getName());
  return exact == nullptr ? nte : exact;
}

boost::iterator_range<NameTree::const_iterator>
NameTree::findAllMatches(const Name& prefix,
                         const EntrySelector& entrySelector) const
{
  NFD_LOG_TRACE("NameTree::findAllMatches" << prefix);

  // As we are using Name Prefix Hash Table, and the current LPM() is
  // implemented as starting from full name, and reduce the number of
  // components by 1 each time, we could use it here.
  // For trie-like design, it could be more efficient by walking down the
  // trie from the root node.

  shared_ptr<Entry> entry = findLongestPrefixMatch(prefix, entrySelector);
  return {Iterator(make_shared<PrefixMatchImpl>(*this, entrySelector), entry), end()};
}

boost::iterator_range<NameTree::const_iterator>
NameTree::fullEnumerate(const EntrySelector& entrySelector) const
{
  NFD_LOG_TRACE("fullEnumerate");

  return {Iterator(make_shared<FullEnumerationImpl>(*this, entrySelector), nullptr), end()};
}

boost::iterator_range<NameTree::const_iterator>
NameTree::partialEnumerate(const Name& prefix,
                           const EntrySubTreeSelector& entrySubTreeSelector) const
{
  // the first step is to process the root node
  shared_ptr<Entry> entry = findExactMatch(prefix);
  return {Iterator(make_shared<PartialEnumerationImpl>(*this, entrySubTreeSelector), entry), end()};
}

// Hash Table Resize
void
NameTree::resize(size_t newNBuckets)
{
  NFD_LOG_TRACE("resize");

  Node** newBuckets = new Node*[newNBuckets];
  size_t count = 0;

  // referenced ccnx hashtb.c hashtb_rehash()
  Node** pp = nullptr;
  Node* p = nullptr;
  Node* pre = nullptr;
  Node* q = nullptr; // record p->m_next

  for (size_t i = 0; i < newNBuckets; ++i) {
    newBuckets[i] = nullptr;
  }

  for (size_t i = 0; i < m_nBuckets; ++i) {
    for (p = m_buckets[i]; p != nullptr; p = q) {
      ++count;
      q = p->m_next;
      BOOST_ASSERT(p->m_entry != nullptr);
      uint32_t h = p->m_entry->m_hash;
      uint32_t b = h % newNBuckets;
      pre = nullptr;
      for (pp = &newBuckets[b]; *pp != nullptr; pp = &((*pp)->m_next)) {
        pre = *pp;
      }
      p->m_prev = pre;
      p->m_next = *pp; // Actually *pp always == nullptr in this case
      *pp = p;
    }
  }

  BOOST_ASSERT(count == m_nItems);

  Node** oldBuckets = m_buckets;
  m_buckets = newBuckets;
  delete[] oldBuckets;

  m_nBuckets = newNBuckets;
  m_enlargeThreshold = static_cast<size_t>(m_enlargeLoadFactor * static_cast<double>(m_nBuckets));
  m_shrinkThreshold = static_cast<size_t>(m_shrinkLoadFactor * static_cast<double>(m_nBuckets));
}

// For debugging
void
NameTree::dump(std::ostream& output) const
{
  NFD_LOG_TRACE("dump()");

  Node* node = nullptr;
  shared_ptr<Entry> entry;

  for (size_t i = 0; i < m_nBuckets; ++i) {
    for (node = m_buckets[i]; node != nullptr; node = node->m_next) {
      entry = node->m_entry;

      // if the Entry exist, dump its information
      if (entry != nullptr) {
        output << "Bucket" << i << '\t' << entry->m_prefix.toUri() << '\n';
        output << "\t\tHash " << entry->m_hash << '\n';

        if (entry->m_parent != nullptr) {
          output << "\t\tparent->" << entry->m_parent->m_prefix.toUri();
        }
        else {
          output << "\t\tROOT";
        }
        output << '\n';

        if (!entry->m_children.empty()) {
          output << "\t\tchildren = " << entry->m_children.size() << '\n';

          for (size_t j = 0; j < entry->m_children.size(); ++j) {
            output << "\t\t\tChild " << j << " " << entry->m_children[j]->getPrefix() << '\n';
          }
        }

      }

    }
  }

  output << "Bucket count = " << m_nBuckets << '\n';
  output << "Stored item = " << m_nItems << '\n';
  output << "--------------------------\n";
}

} // namespace name_tree
} // namespace nfd
